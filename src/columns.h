/* Copyright 2016, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: columns.h
 *
 * Column interface.
 */

#ifndef PS2PAPER_COLUMNS
#define PS2PAPER_COLUMNS

#include <stdlib.h>
#include "oslib/wimp.h"

/**
 * An instance of the columns handler.
 */

struct columns_block;

/**
 * Flags relating to the column definition.
 */

enum columns_flags {
	COLUMNS_FLAGS_NONE = 0				/**< No flags are set.		*/
};

/**
 * A column, to be defined by the client.
 */

struct columns_definition {
	wimp_i			column_icon;		/**< The handle of the main window icon relating to the column.		*/
	wimp_i			heading_icon;		/**< The handle of the toolbar pane icon holding the column heading.	*/

	int			width;			/**< The initial width of the column, in OS units.			*/
	int			left_margin;		/**< The left-hand margin of the column, in OS units.			*/
	int			right_margin;		/**< The right-hand margin of the column, in OS units.			*/
	int			min_width;		/**< The minimum width of the colum, in OS units (if draggable).	*/
	int			max_width;		/**< The maximum width of the colum, in OS units (if draggable).	*/

	enum columns_flags	flags;			/**< Flags relating to the column.					*/
};


/**
 * Create a new column definition instance, and return a handle for the instance
 * data.
 * 
 * \param *window_def		The definition of the window to take the columns.
 * \param *toolbar_def		The definition of the toolbar to hold the column headings.
 * \param columns[]		Definitions of the columns in the window.
 * \param column_count		The number of columns in the definition.
 * \return			The handle of the new window column instance, or NULL on failure.
 */

struct columns_block *columns_create_window(wimp_window *window_def, wimp_window *toolbar_def, struct columns_definition columns[], size_t column_count);


/**
 * Set, update or clear the handle of the main window holding the columns.
 * 
 * \param *handle		The handle of the column instance to update.
 * \param window		The new window handle, or NULL if the window has been deleted.
 */

void columns_set_window_handle(struct columns_block *handle, wimp_w window);


/**
 * Set, update or clear the handle of the toolbar pane window holding the column headings.
 * 
 * \param *handle		The handle of the column instance to update.
 * \param window		The new window handle, or NULL if the window has been deleted.
 */

void columns_set_toolbar_handle(struct columns_block *handle, wimp_w toolbar);


/**
 * Update the icon positions in the windows.
 * 
 * \param *handle		The handle of the column instance to update.
 */

void columns_adjust_icons(struct columns_block *handle);


/**
 * Identify which column the supplied X coordinate falls into.
 * 
 * \param *handle		The handle of the column instance to update.
 * \param xpos			The X position within the window.
 * \return			The identified column number, or -1.
 */

int columns_find_pointer(struct columns_block *handle, int xpos);

#endif
