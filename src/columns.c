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
 * \file: columns.c
 *
 * Column implementation.
 */

/* ANSI C header files */

//#include <string.h>
#include <stdlib.h>
//#include <stdio.h>

/* Acorn C header files */

//#include "flex.h"

/* OSLib header files */

//#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

//#include "sflib/errors.h"
//#include "sflib/event.h"
//#include "sflib/icons.h"
//#include "sflib/ihelp.h"
//#include "sflib/msgs.h"
//#include "sflib/templates.h"
//#include "sflib/windows.h"

/* Application header files */

#include "columns.h"


/**
 * Column block structure, defining one column window instance.
 */

struct columns_block {
	wimp_window			*window_def;		/**< The definition of the main window.					*/
	wimp_window			*toolbar_def;		/**< The definition of the toolbar pane.				*/

	wimp_w				window;			/**< The handle of the main window, or NULL if it doesn't exist.	*/
	wimp_w				toolbar;		/**< The handle of the toolbar pane, or NULL if it doesn't exist.	*/

	size_t				column_count;		/**< The number of columns defined in the window.			*/
	struct columns_definition	*columns;		/**< An array of column definitions for the window.			*/
	int				*column_locations;	/**< An array of left-hand positions of the columns in the window.	*/
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

struct columns_block *columns_create_window(wimp_window *window_def, wimp_window *toolbar_def, struct columns_definition columns[], size_t column_count)
{
	struct columns_block	*new;

	new = malloc(sizeof(struct columns_block));
	if (new == NULL)
		return NULL;

	new->window_def = window_def;
	new->window = NULL;

	new->toolbar_def = toolbar_def;
	new->toolbar = NULL;

	new->columns = columns;
	new->column_locations = malloc(sizeof(int) * column_count);
	new->column_count = column_count;

	return new;
}


/**
 * Set, update or clear the handle of the main window holding the columns.
 * 
 * \param *handle		The handle of the column instance to update.
 * \param window		The new window handle, or NULL if the window has been deleted.
 */

void columns_set_window_handle(struct columns_block *handle, wimp_w window)
{
	if (handle == NULL)
		return;

	handle->window = window;
}


/**
 * Set, update or clear the handle of the toolbar pane window holding the column headings.
 * 
 * \param *handle		The handle of the column instance to update.
 * \param window		The new window handle, or NULL if the window has been deleted.
 */

void columns_set_toolbar_handle(struct columns_block *handle, wimp_w toolbar)
{
	if (handle == NULL)
		return;

	handle->toolbar = toolbar;
}


/**
 * Update the icon positions in the windows.
 * 
 * \param *handle		The handle of the column instance to update.
 */

void columns_adjust_icons(struct columns_block *handle)
{
	int	column, xpos;
	wimp_i	icon;

	if (handle == NULL)
		return;

	/* \TODO -- This won't handle grouped columns, or already-created windows at present. */

	xpos = 0;

	for (column = 0; column < handle->column_count; column++) {
		debug_printf("Starting to process column %d", column);
		icon = handle->columns[column].column_icon;
		debug_printf("Main table icon: %d", icon);
		handle->window_def->icons[icon].extent.x0 = xpos + handle->columns[column].left_margin;
		handle->window_def->icons[icon].extent.x1 = xpos + handle->columns[column].width - handle->columns[column].right_margin;

		icon = handle->columns[column].heading_icon;
		debug_printf("Toolbar heading icon: %d", icon);
		handle->toolbar_def->icons[icon].extent.x0 = xpos;
		handle->toolbar_def->icons[icon].extent.x1 = xpos + handle->columns[column].width;

		handle->column_locations[column] = xpos;

		xpos += handle->columns[column].width;
	}
}


/**
 * Identify which column the supplied X coordinate falls into.
 * 
 * \param *handle		The handle of the column instance to update.
 * \param xpos			The X position within the window.
 * \return			The identified column number, or -1.
 */

int columns_find_pointer(struct columns_block *handle, int xpos)
{
	int column;

	if (handle == NULL)
		return -1;

	for (column = 0; column < handle->column_count && xpos > (handle->column_locations[column] + handle->columns[column].width); column++);

	if (column >= handle->column_count)
		return -1;

	if ((xpos > (handle->column_locations[column] + handle->columns[column].width - handle->columns[column].right_margin)) ||
			(xpos < (handle->column_locations[column] + handle->columns[column].left_margin)))
		return -1;

	return column;
}
