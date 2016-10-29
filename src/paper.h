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
 * \file: paper.h
 *
 * Paper definition storage interface.
 */

#ifndef PS2PAPER_PAPER
#define PS2PAPER_PAPER

/* Static constants */

/**
 * The maximum amount of space allocated for a paper definition name.
 */

#define PAPER_NAME_LEN 128

/**
 * The maximum amount of space allocated for a paper file name.
 */

#define PAPER_FILE_LEN 128

/* Data structures */

/**
 * The possible sources for a paper definition.
 */

enum paper_source {
	PAPER_SOURCE_NONE,					/**< There's no definition source.				*/
	PAPER_SOURCE_MASTER,					/**< The definition is in the Printers' master definitions.	*/
	PAPER_SOURCE_DEVICE,					/**< The definition is in the Printers' device definitions.	*/
	PAPER_SOURCE_USER					/**< The definition is in the User definitions.			*/
};

/**
 * The possible statuses for a paper size.
 */

enum paper_size_status {
	PAPER_SIZE_STATUS_UNKNOWN,				/**< The size hasn't yet been scanned.				*/
	PAPER_SIZE_STATUS_OK,					/**< The paper size is OK, and doesn't clash with any others.	*/
	PAPER_SIZE_STATUS_AMBIGUOUS				/**< The paper sise clashes with others of the same name.	*/
};

/**
 * The possible statuses for a paper definition.
 */

enum paper_file_status {
	PAPER_FILE_STATUS_MISSING,				/**< There is no file for the paper size.			*/
	PAPER_FILE_STATUS_UNKNOWN,				/**< There is a file, but it's not one of ours.			*/
	PAPER_FILE_STATUS_CORRECT,				/**< There is a file, and it matches the paper.			*/
	PAPER_FILE_STATUS_INCORRECT				/**< There is a file, but the size is wrong.			*/
};

/**
 * The definition of a paper size.
 */

struct paper_size {
	char			name[PAPER_NAME_LEN];		/**< The Printers name for the paper				*/
	int			width;				/**< The Printers width of the paper				*/
	int			height;				/**< The Printers height of the paper				*/
	enum paper_size_status	size_status;			/**< The status of the paper size				*/
	enum paper_source	source;				/**< The name of the source file				*/
	char			ps2_file[PAPER_FILE_LEN];	/**< The associated PS2 Paper file, or ""			*/
	enum paper_file_status	ps2_file_status;		/**< Indicate the status of the Paper File.			*/
};

/**
 * Initialise the paper definitions list.
 */

void paper_initialise(void);


/**
 * Reset the paper definitions, then read them back in from the source
 * files in Printers.
 */

void paper_read_definitions(void);

/**
 * Return the number of paper definitions which are currently stored.
 *
 * \return			The number of paper definitions.
 */

size_t paper_get_definition_count(void);

/**
 * Return a pointer to the paper definition array. This points into a flex
 * heap, so the pointer will not remain valid if anything causes the heap
 * to shift.
 * 
 * \return			Pointer to the first entry in the array.
 */

struct paper_size *paper_get_definitions(void);

/**
 * Launch the snippet file relating to a paper definition, using a
 * *Filer_Run command.
 * 
 * \param definition		The index into the definitions of the
 *				definition to be launched.
 */

void paper_launch_file(int definition);

/**
 * Write a new snipped file for a paper definition.
 *
 * \param definition		The index into the definitions of the
 *				definition to be launched.
 */

void paper_write_file(int definition);

/**
 * Ensure that the Paper folder exists on Choices$Write, ready for writing
 * PS2 files to.
 *
 * \return			TRUE if successful; FALSE on failure.
 */

osbool paper_ensure_ps2_file_folder(void);

#endif
