/* Copyright 2013-2016, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of PS2Paper:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: paper.c
 *
 * Paper definition storage implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/fileswitch.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/string.h"

/* Application header files */

#include "paper.h"

#include "list.h"

/**
 * The maximum length of a paper definition file line.
 */

#define PAPER_MAX_LINE_LEN 1024

/**
 * The number of paper spaces that we allocate on each change in flex heap.
 */

#define PAPER_STORAGE_ALLOCATION 4

static struct paper_size	*paper_sizes = NULL;		/**< Linked list of paper sizes.				*/
static size_t			paper_allocation = 0;		/**< The number of spaces allocated for paper definitions.	*/
static size_t			paper_count = 0;		/**< Number of defined paper sizes.				*/

static void			paper_clear_definitions(void);
static osbool			paper_allocate_definition_space(unsigned new_allocation);
static osbool			paper_read_def_file(char *file, enum paper_source source);
static void			paper_scan_sizes(void);
static enum paper_file_status	paper_read_pagesize(struct paper_size *paper, char *file);
static osbool			paper_write_pagesize(struct paper_size *paper, char *file_path);



/**
 * Initialise the paper definitions list.
 */

void paper_initialise(void)
{
	if (flex_alloc((flex_ptr) &paper_sizes, 4) == 0)
		paper_sizes = NULL;

	paper_clear_definitions();
	paper_read_definitions();
}


/**
 * Reset the paper definitions, then read them back in from the source
 * files in Printers.
 */

void paper_read_definitions(void)
{
	paper_clear_definitions();

	paper_read_def_file("Printers:PaperRO", PAPER_SOURCE_MASTER);
	paper_read_def_file("PrinterChoices:PaperRW", PAPER_SOURCE_USER);
	paper_read_def_file("Printers:ps.Resources.PaperRO", PAPER_SOURCE_DEVICE);

	paper_scan_sizes();

	list_rescan_paper_definitions();
}


size_t paper_get_definition_count(void)
{
	return paper_count;
}

struct paper_size *paper_get_definitions(void)
{
	return paper_sizes;
}

void paper_launch_file(int definition)
{
	char		buffer[PAPER_MAX_LINE_LEN];
	os_error	*error;

	if (definition < 0 || definition >= paper_count || paper_sizes[definition].ps2_file_status == PAPER_FILE_STATUS_MISSING)
		return;

	snprintf(buffer, PAPER_MAX_LINE_LEN, "%%Filer_Run -Shift Printers:ps.Paper.%s", paper_sizes[definition].ps2_file);
	buffer[PAPER_MAX_LINE_LEN - 1] = '\0';
	error = xos_cli(buffer);
	if (error != NULL)
		error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
}

void paper_write_file(int definition)
{
	if (definition < 0 || definition >= paper_count || paper_sizes[definition].ps2_file_status == PAPER_FILE_STATUS_CORRECT)
		return;

	if ((paper_sizes[definition].ps2_file_status == PAPER_FILE_STATUS_UNKNOWN) &&
			(error_msgs_param_report_question("Overwrite", "OverwriteB", paper_sizes[definition].ps2_file, NULL, NULL, NULL) == 2))
		return;

	paper_write_pagesize(paper_sizes + definition, "<Choices$Write>.Printers.ps.Paper");

	return;
}



/**
 * Clear the paper definitions and release the memory used to hold them.
 * Calling this function will also initialise the flex block and
 * associated variables.
 */

static void paper_clear_definitions(void)
{
	paper_count = 0;
	paper_allocation = 0;

	if (paper_sizes == NULL)
		return;

	if (flex_extend((flex_ptr) &paper_sizes, 4) == 0)
		paper_sizes = NULL;
}


/**
 * Claim additional storage space for paper definitions.
 * 
 * \param new_allocation	The number of storage places required.
 * \return			TRUE if successful; FALSE if allocation failed.
 */

static osbool paper_allocate_definition_space(unsigned new_allocation)
{
	if (paper_sizes == NULL)
		return FALSE;

	if (new_allocation <= paper_allocation)
		return TRUE;

	new_allocation = ((new_allocation / PAPER_STORAGE_ALLOCATION) + 1) * PAPER_STORAGE_ALLOCATION;

	if (flex_extend((flex_ptr) &paper_sizes, new_allocation * sizeof(struct paper_size)) == 0)
		return FALSE;

	paper_allocation = new_allocation;

	return TRUE;
}


/**
 * Process the contents of a Printers paper file, reading the paper definitions
 * and adding them to the list of sizes.
 *
 * \param *file			The name of the file to be read in.
 * \param source		The type of file being read.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool paper_read_def_file(char *file, enum paper_source source)
{
	FILE			*in;
	char			line[PAPER_MAX_LINE_LEN], *clean, *data, paper_name[PAPER_NAME_LEN];
	int			i;
	unsigned		paper_width, paper_height;
	struct paper_size	*paper_definition;
	os_error		*error;
	fileswitch_object_type	type;


	if (file == NULL || *file == '\0')
		return FALSE;

	in = fopen(file, "r");

	if (in == NULL)
		return FALSE;

	*paper_name = '\0';
	paper_width = 0;
	paper_height = 0;

	while (fgets(line, PAPER_MAX_LINE_LEN, in) != NULL) {
		string_ctrl_zero_terminate(line);
		clean = string_strip_surrounding_whitespace(line);

		if (*clean == '\0' || *clean == '#')
			continue;

		if (strstr(clean, "pn:") == clean) {
			data = string_strip_surrounding_whitespace(clean + 3);
			strncpy(paper_name, data, PAPER_NAME_LEN);
		} else if (strstr(clean, "pw:") == clean) {
			data = string_strip_surrounding_whitespace(clean + 3);
			paper_width = atoi(data);
		} else if (strstr(clean, "ph:") == clean) {
			data = string_strip_surrounding_whitespace(clean + 3);
			paper_height = atoi(data);
		}

		if (paper_name != '\0' && paper_width != 0 && paper_height != 0) {
			paper_allocate_definition_space(paper_count + 1);

			if (paper_count < paper_allocation) {
				paper_definition = paper_sizes + paper_count;

				strncpy(paper_definition->name, paper_name, PAPER_NAME_LEN);
				paper_definition->source = source;
				paper_definition->width = paper_width;
				paper_definition->height = paper_height;
				paper_definition->size_status = PAPER_SIZE_STATUS_UNKNOWN;

				for (i = 0; i < PAPER_FILE_LEN && paper_name[i] != '\0' && paper_name[i] != ' '; i++)
					paper_definition->ps2_file[i] = paper_name[i];

				paper_definition->ps2_file[i] = '\0';
				paper_definition->ps2_file_status = PAPER_FILE_STATUS_MISSING;

				string_tolower(paper_definition->ps2_file);

				if (paper_definition->ps2_file[0] != '\0') {
					snprintf(line, PAPER_MAX_LINE_LEN, "Printers:ps.Paper.%s", paper_definition->ps2_file);
					line[PAPER_MAX_LINE_LEN - 1] = '\0';
					error = xosfile_read_no_path(line, &type, NULL, NULL, NULL, NULL);

					if (error == NULL && type == fileswitch_IS_FILE)
						paper_definition->ps2_file_status = paper_read_pagesize(paper_definition, line);
				}

				paper_count++;
			}

			*paper_name = '\0';
			paper_width = 0;
			paper_height = 0;
		}
	}

	fclose(in);

	return TRUE;
}


/**
 * Run a scan of the paper definitions to set up the paper size status values.
 */

static void paper_scan_sizes(void)
{
	int	paper, test;
	osbool	ambiguous;

	if (paper_sizes == NULL)
		return;

	for (paper = 0; paper < paper_count; paper++) {
		/* Has this paper already been tested in an earlier scan? */

		if (paper_sizes[paper].size_status != PAPER_SIZE_STATUS_UNKNOWN)
			continue;

		/* Scan through the remaining paper definitions to check for matching PS2 filenames. */

		ambiguous = FALSE;

		for (test = paper + 1; test < paper_count; test++) {
			/* Do the PS2 filenames match or not? */

			if (strcmp(paper_sizes[paper].ps2_file, paper_sizes[test].ps2_file) != 0)
				continue;

			/* If they do, do the paper sizes agree or not? */

			if (paper_sizes[paper].width == paper_sizes[test].width && paper_sizes[paper].height == paper_sizes[test].height)
				continue;

			/* If they don't, the same filename is being used for different sizes of paper. */

			ambiguous = TRUE;
			break;
		}

		/* Update the status of this paper definition. */

		paper_sizes[paper].size_status = (ambiguous) ? PAPER_SIZE_STATUS_AMBIGUOUS : PAPER_SIZE_STATUS_OK;

		/* Update the status of any other definitions using the same PS2 filename. */

		for (test = paper + 1; test < paper_count; test++) {
			if (strcmp(paper_sizes[paper].ps2_file, paper_sizes[test].ps2_file) == 0)
				paper_sizes[test].size_status = (ambiguous) ? PAPER_SIZE_STATUS_AMBIGUOUS : PAPER_SIZE_STATUS_OK;
		}
	}
}


static enum paper_file_status paper_read_pagesize(struct paper_size *paper, char *file)
{
	FILE	*in;
	char	line[1024];
	double	width, height;

	if (file == NULL)
		return PAPER_FILE_STATUS_UNKNOWN;

	in = fopen(file, "r");
	if (in == NULL)
		return PAPER_FILE_STATUS_UNKNOWN;

	if (fgets(line, sizeof(line), in) == NULL) {
		fclose(in);
		return PAPER_FILE_STATUS_UNKNOWN;
	}

	if (strcmp(line, "% Created by PS2Paper\n") != 0) {
		fclose(in);
		return PAPER_FILE_STATUS_UNKNOWN;
	}

	if (fgets(line, sizeof(line), in) == NULL) {
		fclose(in);
		return PAPER_FILE_STATUS_UNKNOWN;
	}

	if (fgets(line, sizeof(line), in) == NULL) {
		fclose(in);
		return PAPER_FILE_STATUS_UNKNOWN;
	}

	sscanf(line, "<< /PageSize [ %lf %lf ] >> setpagedevice \n", &width, &height);

	fclose(in);

	if (width * 1000.0 != paper->width || height * 1000.0 != paper->height)
		return PAPER_FILE_STATUS_INCORRECT;

	return PAPER_FILE_STATUS_CORRECT;
}


/**
 * Ensure that the Paper folder exists on Choices$Write, ready for writing
 * PS2 files to.
 *
 * \return			TRUE if successful; FALSE on failure.
 */

osbool paper_ensure_ps2_file_folder(void)
{
	int			var_len;
	char			file_path[PAPER_MAX_LINE_LEN];


	*file_path = '\0';

	os_read_var_val_size("Choices$Write", 0, os_VARTYPE_STRING, &var_len, NULL);

	if (var_len == 0)
		return FALSE;

	snprintf(file_path, PAPER_MAX_LINE_LEN, "<Choices$Write>.Printers");
	file_path[PAPER_MAX_LINE_LEN - 1] = '\0';
	if (osfile_read_no_path(file_path, NULL, NULL, NULL, NULL) == fileswitch_NOT_FOUND)
		osfile_create_dir(file_path, 0);

	snprintf(file_path, PAPER_MAX_LINE_LEN, "<Choices$Write>.Printers.ps");
	file_path[PAPER_MAX_LINE_LEN - 1] = '\0';
	if (osfile_read_no_path(file_path, NULL, NULL, NULL, NULL) == fileswitch_NOT_FOUND)
		osfile_create_dir(file_path, 0);

	snprintf(file_path, PAPER_MAX_LINE_LEN, "<Choices$Write>.Printers.ps.Paper");
	file_path[PAPER_MAX_LINE_LEN - 1] = '\0';
	if (osfile_read_no_path(file_path, NULL, NULL, NULL, NULL) == fileswitch_NOT_FOUND)
		osfile_create_dir(file_path, 0);

	return TRUE;
}

static osbool paper_write_pagesize(struct paper_size *paper, char *file_path)
{
	FILE	*out;
	char	filename[1024];

	if (paper == NULL)
		return FALSE;

	snprintf(filename, sizeof(filename), "%s.%s", file_path, paper->ps2_file);

	out = fopen(filename, "w");
	if (out == NULL)
		return FALSE;

	fprintf(out, "%% Created by PS2Paper\n");
	fprintf(out, "%%%%BeginFeature: PageSize %s\n", paper->name);
	fprintf(out, "<< /PageSize [ %.3f %.3f ] >> setpagedevice\n", (double) paper->width / 1000.0, (double) paper->height / 1000.0);
	fprintf(out, "%%%%EndFeature\n");

	fclose(out);
	osfile_set_type (filename, (bits) 0xff5);

	return TRUE;
}
