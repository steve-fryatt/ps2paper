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

#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/ihelp.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "list.h"

#define LIST_TOOLBAR_HEIGHT 132						/**< The height of the toolbar in OS units.				*/
#define LIST_LINE_HEIGHT 56						/**< The height of a results line, in OS units.				*/
#define LIST_WINDOW_MARGIN 4						/**< The margin around the edge of the window, in OS units.		*/
#define LIST_LINE_OFFSET 4						/**< The offset from the base of a line to the base of the icon.	*/
#define LIST_ICON_HEIGHT 52						/**< The height of an icon in the results window, in OS units.		*/

#define LIST_NAME_ICON 0
#define LIST_WIDTH_ICON 1
#define LIST_HEIGHT_ICON 2
#define LIST_LOCATION_ICON 3
#define LIST_FILENAME_ICON 4
#define LIST_STATUS_ICON 5
#define LIST_UNKNOWN1_ICON 6
#define LIST_UNKNOWN2_ICON 7

static wimp_window	*list_window_def = NULL;	/**< The list window definition.	*/
static wimp_window	*list_pane_def = NULL;		/**> The list pane definition.		*/

static wimp_w		list_window = NULL;		/**< The list window handle.		*/
static wimp_w		list_pane = NULL;		/**< The list pane handle.		*/


static void list_redraw_handler(wimp_draw *redraw);


/* Line position calculations.
 *
 * NB: These can be called with lines < 0 to give lines off the top of the window!
 */

#define LINE_BASE(x) (-((x)+1) * LIST_LINE_HEIGHT - LIST_TOOLBAR_HEIGHT - LIST_WINDOW_MARGIN)
#define LINE_Y0(x) (LINE_BASE(x) + LIST_LINE_OFFSET)
#define LINE_Y1(x) (LINE_BASE(x) + LIST_LINE_OFFSET + LIST_ICON_HEIGHT)

/* Row calculations: taking a positive offset from the top of the window, return
 * the raw row number and the position within a row.
 */

#define ROW(y) (((-(y)) - LIST_TOOLBAR_HEIGHT - LIST_WINDOW_MARGIN) / LIST_LINE_HEIGHT)
#define ROW_Y_POS(y) (((-(y)) - LIST_TOOLBAR_HEIGHT - LIST_WINDOW_MARGIN) % LIST_LINE_HEIGHT)

/* Return true or false if a ROW_Y_POS() value is above or below the icon
 * area of the row.
 */

#define ROW_ABOVE(y) ((y) < (LIST_LINE_HEIGHT - (LIST_LINE_OFFSET + LIST_ICON_HEIGHT)))
#define ROW_BELOW(y) ((y) > (LIST_LINE_HEIGHT - LIST_LINE_OFFSET))


/**
 * Initialise the list window.
 *
 * \param *sprites		The application sprite area.
 */

void list_initialise(osspriteop_area *sprites)
{
	os_error	*error;

	list_window_def = templates_load_window("Paper");
	list_pane_def = templates_load_window("PaperTB");

	list_pane_def->sprite_area = sprites;

	windows_place_as_toolbar(list_window_def, list_pane_def, LIST_TOOLBAR_HEIGHT - 4);

	error = xwimp_create_window(list_window_def, &list_window);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	error = xwimp_create_window(list_pane_def, &list_pane);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	ihelp_add_window(list_window, "List", NULL);
	ihelp_add_window(list_pane, "ListTB", NULL);

	event_add_window_redraw_event(list_window, list_redraw_handler);


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


/**
 * Set the number of visible lines in the list window.
 *
 * \param lines			The number of lines to be displayed.
 */

void list_set_lines(int lines)
{
	int			visible_extent, new_extent, new_scroll;
	wimp_window_state	state;
	os_box			extent;


	state.w = list_window;
	wimp_get_window_state(&state);

	visible_extent = state.yscroll + (state.visible.y0 - state.visible.y1);

	new_extent = -LIST_ICON_HEIGHT * lines;

	if (new_extent > (state.visible.y0 - state.visible.y1))
		new_extent = state.visible.y0 - state.visible.y1;

	if (new_extent > visible_extent) {
		/* Calculate the required new scroll offset.  If this is greater than zero, the current window is too
		 * big and will need shrinking down.  Otherwise, just set the new scroll offset.
		 */

		new_scroll = new_extent - (state.visible.y0 - state.visible.y1);

		if (new_scroll > 0) {
			state.visible.y0 += new_scroll;
			state.yscroll = 0;
		} else {
			state.yscroll = new_scroll;
		}

		wimp_open_window((wimp_open *) &state);
	}

	extent.x0 = 0;
	extent.y1 = 0;
	extent.x1 = state.visible.x1 - state.visible.x0;
	extent.y0 = new_extent;

	wimp_set_extent(list_window, &extent);
}








/**
 * Callback to handle redraw events on the list window.
 *
 * \param  *redraw		The Wimp redraw event block.
 */

static void list_redraw_handler(wimp_draw *redraw)
{
	int			ox, oy, top, bottom, y, i;
	osbool			more;
	wimp_icon		*icon;
	char			validation[255];

//	handle = (struct results_window *) event_get_window_user_data(redraw->w);

//	if (handle == NULL)
//		return;


	icon = list_window_def->icons;

	/* Set up the validation string buffer for text+sprite icons. */

	*validation = 'S';
	icon[LIST_UNKNOWN1_ICON].data.indirected_text.validation = validation;
	icon[LIST_UNKNOWN2_ICON].data.indirected_text.validation = validation;

	/* Redraw the window. */

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
	//	icon[RESULTS_ICON_FILE].extent.x1 = handle->format_width - RESULTS_WINDOW_MARGIN;
	//	icon[RESULTS_ICON_SIZE].extent.x1 = handle->format_width - RESULTS_WINDOW_MARGIN;

		top = (oy - redraw->clip.y1 - LIST_TOOLBAR_HEIGHT) / LIST_LINE_HEIGHT;
		if (top < 0)
			top = 0;

		bottom = ((LIST_LINE_HEIGHT * 1.5) + oy - redraw->clip.y0 - LIST_TOOLBAR_HEIGHT) / LIST_LINE_HEIGHT;
	//	if (bottom > handle->display_lines)
	//		bottom = handle->display_lines;

		for (y = top; y < bottom; y++) {
			icon[LIST_NAME_ICON].extent.y0 = LINE_Y0(y);
			icon[LIST_NAME_ICON].extent.y1 = LINE_Y1(y);

			wimp_plot_icon(&(icon[LIST_NAME_ICON]));

			icon[LIST_WIDTH_ICON].extent.y0 = LINE_Y0(y);
			icon[LIST_WIDTH_ICON].extent.y1 = LINE_Y1(y);

			wimp_plot_icon(&(icon[LIST_WIDTH_ICON]));

			icon[LIST_HEIGHT_ICON].extent.y0 = LINE_Y0(y);
			icon[LIST_HEIGHT_ICON].extent.y1 = LINE_Y1(y);

			wimp_plot_icon(&(icon[LIST_HEIGHT_ICON]));

			icon[LIST_LOCATION_ICON].extent.y0 = LINE_Y0(y);
			icon[LIST_LOCATION_ICON].extent.y1 = LINE_Y1(y);

			wimp_plot_icon(&(icon[LIST_LOCATION_ICON]));

			icon[LIST_FILENAME_ICON].extent.y0 = LINE_Y0(y);
			icon[LIST_FILENAME_ICON].extent.y1 = LINE_Y1(y);

			wimp_plot_icon(&(icon[LIST_FILENAME_ICON]));

			icon[LIST_STATUS_ICON].extent.y0 = LINE_Y0(y);
			icon[LIST_STATUS_ICON].extent.y1 = LINE_Y1(y);

			wimp_plot_icon(&(icon[LIST_STATUS_ICON]));



	//		i = handle->redraw[y].index;

	//		if (handle->redraw[i].format_width != handle->format_width)
	//			results_reformat_line(handle, i, truncation, truncation_len);

	/*		switch (handle->redraw[i].type) {
			case RESULTS_LINE_FILENAME:
			case RESULTS_LINE_ERROR_FILENAME:
				icon[RESULTS_ICON_FILE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_FILE].extent.y1 = LINE_Y1(y);

				objdb_get_info(handle->objects, handle->redraw[i].file, file, &object);
				fileicon_get_object_icon(file, &typeinfo);

				if (typeinfo.small != TEXTDUMP_NULL) {
					strcpy(validation + 1, fileicon_get_base() + typeinfo.small);
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_HALF_SIZE;
				} else if (typeinfo.large != TEXTDUMP_NULL) {
					strcpy(validation + 1, fileicon_get_base() + typeinfo.large);
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_HALF_SIZE;
				} else {
					strcpy(validation + 1, "small_xxx");
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_HALF_SIZE;
				}

				if ((object.status == OBJDB_STATUS_UNCHANGED || object.status == OBJDB_STATUS_CHANGED) &&
						handle->redraw[i].type != RESULTS_LINE_ERROR_FILENAME)
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SHADED;
				else
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_SHADED;

				if (truncation != NULL)
					objdb_get_name(handle->objects, handle->redraw[i].file, truncation + 3, truncation_len - 3);

				if (truncation == NULL) {
					icon[RESULTS_ICON_FILE].data.indirected_text.text = "Redraw Error";
				} else if (handle->redraw[i].truncate > 0) {
					truncation[handle->redraw[i].truncate] = '.';
					truncation[handle->redraw[i].truncate + 1] = '.';
					truncation[handle->redraw[i].truncate + 2] = '.';
					icon[RESULTS_ICON_FILE].data.indirected_text.text = truncation + handle->redraw[i].truncate;
				} else {
					icon[RESULTS_ICON_FILE].data.indirected_text.text = truncation + 3;
				}

				icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_FILE].flags |= (handle->redraw[i].colour << wimp_ICON_FG_COLOUR_SHIFT);

				if (handle->redraw[i].flags & RESULTS_FLAG_SELECTED)
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_SELECTED;
				else
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SELECTED;

				wimp_plot_icon(&(icon[RESULTS_ICON_FILE]));
				break;


			case RESULTS_LINE_FILEINFO:
				icon[RESULTS_ICON_TYPE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_TYPE].extent.y1 = LINE_Y1(y);

				objdb_get_info(handle->objects, handle->redraw[i].file, file, &object);
				fileicon_get_object_icon(file, &typeinfo);

				if (typeinfo.name != TEXTDUMP_NULL) {
					icon[RESULTS_ICON_TYPE].data.indirected_text.text = fileicon_get_base() + typeinfo.name;
				} else {
					icon[RESULTS_ICON_TYPE].data.indirected_text.text = "";
				}

				if (file == NULL)
					break;

				icon[RESULTS_ICON_SIZE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_SIZE].extent.y1 = LINE_Y1(y);
				icon[RESULTS_ICON_ATTRIBUTES].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_ATTRIBUTES].extent.y1 = LINE_Y1(y);
				icon[RESULTS_ICON_DATE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_DATE].extent.y1 = LINE_Y1(y);

				if (size != NULL && xos_convert_file_size(file->size, size, 32, NULL) != NULL)
					*size = '\0';

				if (attributes != NULL)
					results_create_attributes_string(file->attr, attributes, 32);

				if (date != NULL)
					results_create_address_string(file->load_addr, file->exec_addr, date, 100);

				icon[RESULTS_ICON_SIZE].data.indirected_text.text = (size != NULL) ? size : "Redraw Error";
				icon[RESULTS_ICON_ATTRIBUTES].data.indirected_text.text = (attributes != NULL) ? attributes : "Redraw Error";
				icon[RESULTS_ICON_DATE].data.indirected_text.text = (date != NULL) ? date : "Redraw Error";


				if (object.status == OBJDB_STATUS_UNCHANGED) {
					icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_SHADED;
					icon[RESULTS_ICON_TYPE].flags &= ~wimp_ICON_SHADED;
					icon[RESULTS_ICON_ATTRIBUTES].flags &= ~wimp_ICON_SHADED;
					icon[RESULTS_ICON_DATE].flags &= ~wimp_ICON_SHADED;
				} else {
					icon[RESULTS_ICON_SIZE].flags |= wimp_ICON_SHADED;
					icon[RESULTS_ICON_TYPE].flags |= wimp_ICON_SHADED;
					icon[RESULTS_ICON_ATTRIBUTES].flags |= wimp_ICON_SHADED;
					icon[RESULTS_ICON_DATE].flags |= wimp_ICON_SHADED;
				}

				icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_SIZE].flags |= (handle->redraw[i].colour << wimp_ICON_FG_COLOUR_SHIFT);
				icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_SELECTED;

				wimp_plot_icon(&(icon[RESULTS_ICON_SIZE]));
				wimp_plot_icon(&(icon[RESULTS_ICON_TYPE]));
				wimp_plot_icon(&(icon[RESULTS_ICON_ATTRIBUTES]));
				wimp_plot_icon(&(icon[RESULTS_ICON_DATE]));
				break;


			case RESULTS_LINE_CONTENTS:
				icon[RESULTS_ICON_SIZE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_SIZE].extent.y1 = LINE_Y1(y);

	*/			/* We're about to copy a pointer to the icon text from the flex heap,
				 * so take care not to shift the heap until wimp_plot_icon().
				 */

	/*			if (truncation != NULL && handle->redraw[i].truncate > 0) {
					strcpy(truncation + 3, textdump_get_base(handle->text) + handle->redraw[i].text + handle->redraw[i].truncate);
					icon[RESULTS_ICON_SIZE].data.indirected_text.text = truncation;
				} else {
					icon[RESULTS_ICON_SIZE].data.indirected_text.text = textdump_get_base(handle->text) + handle->redraw[i].text;
				}
				icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_SIZE].flags |= (handle->redraw[i].colour << wimp_ICON_FG_COLOUR_SHIFT);

				if (handle->redraw[i].flags & RESULTS_FLAG_SELECTED)
					icon[RESULTS_ICON_SIZE].flags |= wimp_ICON_SELECTED;
				else
					icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_SELECTED;

				icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_SHADED;

				wimp_plot_icon(&(icon[RESULTS_ICON_SIZE]));
				break;


			case RESULTS_LINE_TEXT:
				icon[RESULTS_ICON_FILE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_FILE].extent.y1 = LINE_Y1(y);

				fileicon_get_special_icon(handle->redraw[i].sprite, &typeinfo);

				if (typeinfo.small != TEXTDUMP_NULL) {
					strcpy(validation + 1, fileicon_get_base() + typeinfo.small);
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_HALF_SIZE;
				} else if (typeinfo.large != TEXTDUMP_NULL) {
					strcpy(validation + 1, fileicon_get_base() + typeinfo.large);
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_HALF_SIZE;
				} else {
					strcpy(validation + 1, "small_xxx");
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_HALF_SIZE;
				}

	*/			/* We're about to copy a pointer to the icon text from the flex heap,
				 * so take care not to shift the heap until wimp_plot_icon().
				 */

	/*			if (truncation != NULL && handle->redraw[i].truncate > 0) {
					strcpy(truncation + 3, textdump_get_base(handle->text) + handle->redraw[i].text + handle->redraw[i].truncate);
					icon[RESULTS_ICON_FILE].data.indirected_text.text = truncation;
				} else {
					icon[RESULTS_ICON_FILE].data.indirected_text.text = textdump_get_base(handle->text) + handle->redraw[i].text;
				}
				icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_FILE].flags |= (handle->redraw[i].colour << wimp_ICON_FG_COLOUR_SHIFT);

				if (handle->redraw[i].flags & RESULTS_FLAG_SELECTED)
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_SELECTED;
				else
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SELECTED;

				icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SHADED;

				wimp_plot_icon(&(icon[RESULTS_ICON_FILE]));
				break;

			default:
				break;
			}
	*/
		}

		more = wimp_get_rectangle(redraw);
	}
}
