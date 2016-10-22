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
 * The possible statuses for a paper definition.
 */

enum paper_status {
	PAPER_STATUS_MISSING,					/**< There is no file for the paper size.			*/
	PAPER_STATUS_UNKNOWN,					/**< There is a file, but it's not one of ours.			*/
	PAPER_STATUS_CORRECT,					/**< There is a file, and it matches the paper.			*/
	PAPER_STATUS_INCORRECT					/**< There is a file, but the size is wrong.			*/
};


struct paper_size {
	char			name[PAPER_NAME_LEN];		/**< The Printers name for the paper				*/
	int			width;				/**< The Printers width of the paper				*/
	int			height;				/**< The Printers height of the paper				*/
	enum paper_source	source;				/**< The name of the source file				*/
	char			ps2_file[PAPER_FILE_LEN];	/**< The associated PS2 Paper file, or ""			*/
	enum paper_status	ps2_file_status;		/**< Indicate the status of the Paper File.			*/
};

/**
 * Initialise the paper definitions list.
 */

void paper_initialise(void);

size_t paper_get_definition_count(void);

struct paper_size *paper_get_definitions(void);


#endif
