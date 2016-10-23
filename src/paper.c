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
#include "oslib/osfile.h"
#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
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
static osbool			paper_update_files(void);
static enum paper_status	paper_read_pagesize(struct paper_size *paper, char *file);
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

	debug_printf("Requesting new allocation of %d spaces", new_allocation);

	if (flex_extend((flex_ptr) &paper_sizes, new_allocation * sizeof(struct paper_size)) == 0)
		return FALSE;

	paper_allocation = new_allocation;

	debug_printf("Allocation successful");

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
			debug_printf("Found paper definition: %s, %d x %d", paper_name, paper_width, paper_height);

			paper_allocate_definition_space(paper_count + 1);

			if (paper_count < paper_allocation) {
				paper_definition = paper_sizes + paper_count;

				strncpy(paper_definition->name, paper_name, PAPER_NAME_LEN);
				paper_definition->source = source;
				paper_definition->width = paper_width;
				paper_definition->height = paper_height;

				for (i = 0; i < PAPER_FILE_LEN && paper_name[i] != '\0' && paper_name[i] != ' '; i++)
					paper_definition->ps2_file[i] = paper_name[i];

				paper_definition->ps2_file[i] = '\0';
				paper_definition->ps2_file_status = PAPER_STATUS_MISSING;

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

static osbool paper_update_files(void)
{
	struct paper_size	*paper;
	int			var_len;
	char			file_path[1024];


	*file_path = '\0';

	os_read_var_val_size("Choices$Write", 0, os_VARTYPE_STRING, &var_len, NULL);

	if (var_len == 0)
		return FALSE;

	snprintf(file_path, sizeof(file_path), "<Choices$Write>.Printers");
	if (osfile_read_no_path(file_path, NULL, NULL, NULL, NULL) == fileswitch_NOT_FOUND)
		osfile_create_dir(file_path, 0);

	snprintf(file_path, sizeof(file_path), "<Choices$Write>.Printers.ps");
	if (osfile_read_no_path(file_path, NULL, NULL, NULL, NULL) == fileswitch_NOT_FOUND)
		osfile_create_dir(file_path, 0);

	snprintf(file_path, sizeof(file_path), "<Choices$Write>.Printers.ps.Paper");
	if (osfile_read_no_path(file_path, NULL, NULL, NULL, NULL) == fileswitch_NOT_FOUND)
		osfile_create_dir(file_path, 0);

//	paper = paper_sizes;

//	while (paper != NULL) {
//		if (paper->ps2_file_status == PAPER_STATUS_MISSING || paper->ps2_file_status == PAPER_STATUS_INCORRECT)
//			paper_write_pagesize(paper, file_path);

//		paper = paper->next;
//	}

	return TRUE;
}

static enum paper_status paper_read_pagesize(struct paper_size *paper, char *file)
{
	FILE	*in;
	char	line[1024];
	double	width, height;

	if (file == NULL)
		return PAPER_STATUS_UNKNOWN;

	in = fopen(file, "r");
	if (in == NULL)
		return PAPER_STATUS_UNKNOWN;

	if (fgets(line, sizeof(line), in) == NULL) {
		fclose(in);
		return PAPER_STATUS_UNKNOWN;
	}

	if (strcmp(line, "% Created by PrintPDF\n") != 0) {
		fclose(in);
		return PAPER_STATUS_UNKNOWN;
	}

	if (fgets(line, sizeof(line), in) == NULL) {
		fclose(in);
		return PAPER_STATUS_UNKNOWN;
	}

	if (fgets(line, sizeof(line), in) == NULL) {
		fclose(in);
		return PAPER_STATUS_UNKNOWN;
	}

	sscanf(line, "<< /PageSize [ %lf %lf ] >> setpagedevice \n", &width, &height);

	fclose(in);

	if (width * 1000.0 != paper->width || height * 1000.0 != paper->height)
		return PAPER_STATUS_INCORRECT;

	return PAPER_STATUS_CORRECT;
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

	fprintf(out, "%% Created by PrintPDF\n");
	fprintf(out, "%%%%BeginFeature: PageSize %s\n", paper->name);
	fprintf(out, "<< /PageSize [ %.3f %.3f ] >> setpagedevice\n", (double) paper->width / 1000.0, (double) paper->height / 1000.0);
	fprintf(out, "%%%%EndFeature\n");

	fclose(out);
	osfile_set_type (filename, (bits) 0xff5);

	return TRUE;
}
