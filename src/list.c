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
 * \file: list.c
 *
 * Paper list window implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/ihelp.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "list.h"

#define LIST_TOOLBAR_HEIGHT 132

static wimp_w		list_window = NULL;		/**< The list window handle.		*/
static wimp_w		list_pane = NULL;		/**< The list pane handle.		*/

/**
 * Initialise the list window.
 *
 * \param *sprites		The application sprite area.
 */

void list_initialise(osspriteop_area *sprites)
{
	wimp_window	*window_def, *pane_def;
	os_error	*error;

	window_def = templates_load_window("Paper");
	pane_def = templates_load_window("PaperTB");

	pane_def->sprite_area = sprites;

	windows_place_as_toolbar(window_def, pane_def, LIST_TOOLBAR_HEIGHT - 4);

	error = xwimp_create_window(window_def, &list_window);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	error = xwimp_create_window(pane_def, &list_pane);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

//	free(window_def);
//	free(pane_def);

	ihelp_add_window(list_window, "List", NULL);
	ihelp_add_window(list_pane, "ListTB", NULL);


//	event_add_window_mouse_event(preset_edit_window, preset_edit_click_handler);
//	event_add_window_key_event(preset_edit_window, preset_edit_keypress_handler);
//	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETDATE, TRUE);
//	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETFROM, TRUE);
//	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETTO, TRUE);
//	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETREF, TRUE);
//	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETAMOUNT, TRUE);
//	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETDESC, TRUE);
}


/**
 * Open the List window centred on the screen.
 */

void list_open_window(void)
{
	windows_open_centred_on_screen(list_window);
	windows_open_nested_as_toolbar(list_pane, list_window, LIST_TOOLBAR_HEIGHT - 4);
}
