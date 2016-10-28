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

#include "flex.h"

/* OSLib header files */

#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "list.h"

#include "columns.h"
#include "paper.h"

/* The page dimensions. */

#define LIST_TOOLBAR_HEIGHT 132						/**< The height of the toolbar in OS units.				*/
#define LIST_LINE_HEIGHT 56						/**< The height of a results line, in OS units.				*/
#define LIST_WINDOW_MARGIN 4						/**< The margin around the edge of the window, in OS units.		*/
#define LIST_LINE_OFFSET 4						/**< The offset from the base of a line to the base of the icon.	*/
#define LIST_ICON_INSET 4						/**< The left-hand inset for an icon containing a sprite.		*/
#define LIST_ICON_HEIGHT 52						/**< The height of an icon in the results window, in OS units.		*/

/* Memory allocation. */

#define LIST_ICON_BUFFER_LEN 128					/**< The scratch buffer used for formatting text for display.		*/
#define LIST_SELECT_MENU_LEN 150					/**< The amount of space allocated for the selection menu item.		*/

/* The main window icons. */

#define LIST_NAME_ICON 0
#define LIST_WIDTH_ICON 1
#define LIST_HEIGHT_ICON 2
#define LIST_SIZE_ICON 3
#define LIST_FILENAME_ICON 4
#define LIST_STATUS_ICON 5
#define LIST_SEPARATOR_ICON 6

/* The toolbar pane icons. */

#define LIST_REFRESH_ICON 0
#define LIST_SELECT_ICON 1

#define LIST_NAME_HEADING_ICON 2
#define LIST_WIDTH_HEADING_ICON 3
#define LIST_HEIGHT_HEADING_ICON 4
#define LIST_SIZE_HEADING_ICON 5
#define LIST_FILENAME_HEADING_ICON 6
#define LIST_STATUS_HEADING_ICON 7

#define LIST_INCH_ICON 8
#define LIST_MM_ICON 9
#define LIST_POINT_ICON 10

/* The menu entries. */

#define LIST_MENU_SELECTION 0
#define LIST_MENU_SELECT_ALL 1
#define LIST_MENU_CLEAR_SELECTION 2
#define LIST_MENU_DIMENSION_UNITS 3
#define LIST_MENU_REFRESH 4

#define LIST_SELECTION_MENU_WRITE 0
#define LIST_SELECTION_MENU_RUN 1

#define LIST_DIMENSION_MENU_MM 0
#define LIST_DIMENSION_MENU_INCH 1
#define LIST_DIMENSION_MENU_POINT 2

/* The number of columns in the window. */

#define LIST_COLUMN_COUNT 6

/* The column numbers. */

#define LIST_COLUMN_PAPER_NAME 0
#define LIST_COLUMN_PAPER_FILE 4

/* The column definitions. */

static struct columns_definition list_column_definitions[] = {
	{ LIST_NAME_ICON, LIST_NAME_HEADING_ICON, 436, LIST_LINE_OFFSET + LIST_ICON_INSET, LIST_LINE_OFFSET, -1, -1, COLUMNS_FLAGS_NONE },
	{ LIST_WIDTH_ICON, LIST_WIDTH_HEADING_ICON, 156, LIST_LINE_OFFSET, LIST_LINE_OFFSET, -1, -1, COLUMNS_FLAGS_NONE },
	{ LIST_HEIGHT_ICON, LIST_HEIGHT_HEADING_ICON, 156, LIST_LINE_OFFSET, LIST_LINE_OFFSET, -1, -1, COLUMNS_FLAGS_NONE },
	{ LIST_SIZE_ICON, LIST_SIZE_HEADING_ICON, 200, LIST_LINE_OFFSET, LIST_LINE_OFFSET, -1, -1, COLUMNS_FLAGS_NONE },
	{ LIST_FILENAME_ICON, LIST_FILENAME_HEADING_ICON, 360, LIST_LINE_OFFSET + LIST_ICON_INSET, LIST_LINE_OFFSET, -1, -1, COLUMNS_FLAGS_NONE },
	{ LIST_STATUS_ICON, LIST_STATUS_HEADING_ICON, 164, LIST_LINE_OFFSET, LIST_LINE_OFFSET, -1, -1, COLUMNS_FLAGS_NONE }
};

enum list_units {
	LIST_UNITS_MM = 0,
	LIST_UNITS_INCH = 1,
	LIST_UNITS_POINT = 2
};

enum list_line_type {
	LIST_LINE_TYPE_SEPARATOR,
	LIST_LINE_TYPE_PAPER
};

enum list_line_flags {
	LIST_LINE_FLAGS_NONE = 0,
	LIST_LINE_FLAGS_SELECTED = 1
};

struct list_redraw {
	enum list_line_type	type;
	enum list_line_flags	flags;
	int			index;
	enum paper_source	source;
};

static wimp_window		*list_window_def = NULL;		/**< The list window definition.			*/
static wimp_window		*list_pane_def = NULL;			/**< The list pane definition.				*/

static wimp_w			list_window = NULL;			/**< The list window handle.				*/
static wimp_w			list_pane = NULL;			/**< The list pane handle.				*/

static wimp_menu		*list_window_menu = NULL;		/**< The list window menu.				*/
static wimp_menu		*list_window_selection_menu = NULL;	/**< The list window selection submenu.			*/
static wimp_menu		*list_window_dimension_menu = NULL;	/**< The list window display unit menu.			*/

static struct columns_block	*list_columns = NULL;			/**< The column handler for the list window columns.	*/

static enum list_units		list_display_units;			/**< The units used to display paper sizes.		*/

static struct list_redraw	*list_index = NULL;			/**< The window redraw index.				*/
static size_t			list_index_count = 0;			/**< The number of entries in the redraw index.		*/

static int			list_selection_count = 0;		/**< The number of selected lines.			*/
static int			list_selection_row = -1;		/**< The currently selected row, or -1.			*/
static osbool			list_selection_from_menu = FALSE;	/**< TRUE if the selection came from the menu opening.	*/

static void list_click_handler(wimp_pointer *pointer);
static void list_toolbar_click_handler(wimp_pointer *pointer);
static void list_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void list_menu_warning(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void list_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void list_menu_close(wimp_w w, wimp_menu *menu);
static void list_redraw_handler(wimp_draw *redraw);
static void list_add_paper_source_to_index(enum paper_source source, size_t index_lines, struct paper_size *paper, size_t paper_lines);
static void list_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);
static int list_calculate_window_click_column(os_coord *pos, wimp_window_state *state);
static int list_calculate_window_click_row(os_coord *pos, wimp_window_state *state);
static void list_double_click_select(int row, int column);
static void list_select_click_select(int row, int column);
static void list_select_click_adjust(int row, int column);
static void list_select_all(void);
static void list_select_none(void);
static void list_write_selected_files(void);
static void list_launch_selected_files(void);
static void list_set_dimensions(enum list_units units);


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
	int		width;
	os_error	*error;

	/* Set up the List Window menu definition. */

	list_window_menu = templates_get_menu("ListWindowMenu");
	list_window_selection_menu = templates_get_menu("ListWindowSelectionMenu");
	list_window_dimension_menu = templates_get_menu("ListWindowDimensionMenu");
	ihelp_add_menu(list_window_menu, "ListMenu");

	/* Load the List Window and List Window Pane definitions. */

	list_window_def = templates_load_window("Paper");
	list_window_def->sprite_area = sprites;
	list_window_def->icon_count = 0;

	list_pane_def = templates_load_window("PaperTB");
	list_pane_def->sprite_area = sprites;

	/* Initialise the window columns, and adjust the icons to match. */

	list_columns = columns_create_window(list_window_def, list_pane_def, list_column_definitions, LIST_COLUMN_COUNT);
	if (list_columns == NULL)
		error_msgs_report_fatal("ColNoMem");

	columns_adjust_icons(list_columns);

	/* Set the main window width to match the defined columns. */

	width = columns_get_full_width(list_columns);
	list_window_def->extent.x1 = width - list_window_def->extent.x0;
	list_window_def->visible.x1 = width + list_window_def->visible.x0;

	/* Position the toolbar pane to fit. */

	windows_place_as_toolbar(list_window_def, list_pane_def, LIST_TOOLBAR_HEIGHT - 4);

	/* Set the left- and right-hand edges of the section icon to suit the window size. */

	list_window_def->icons[LIST_SEPARATOR_ICON].extent.x0 = list_window_def->extent.x0;
	list_window_def->icons[LIST_SEPARATOR_ICON].extent.x1 = list_window_def->extent.x1;

	/* Create the two windows and register them with Interactive Help. */

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

	ihelp_add_window(list_window, "List", list_decode_window_help);
	ihelp_add_window(list_pane, "ListTB", NULL);

	/* Set up the window event handlers. */

	event_add_window_menu(list_window, list_window_menu);
	event_add_window_redraw_event(list_window, list_redraw_handler);
	event_add_window_mouse_event(list_window, list_click_handler);
	event_add_window_menu_prepare(list_window, list_menu_prepare);
//	event_add_window_menu_warning(list_window, list_menu_warning);
	event_add_window_menu_selection(list_window, list_menu_selection);
	event_add_window_menu_close(list_window, list_menu_close);

	event_add_window_menu(list_pane, list_window_menu);
	event_add_window_mouse_event(list_pane, list_toolbar_click_handler);
//	event_add_window_key_event(list_pane, preset_edit_keypress_handler);
	event_add_window_menu_prepare(list_pane, list_menu_prepare);
//	event_add_window_menu_warning(list_pane, list_menu_warning);
	event_add_window_menu_selection(list_pane, list_menu_selection);
	event_add_window_menu_close(list_pane, list_menu_close);
	event_add_window_icon_radio(list_pane, LIST_INCH_ICON, FALSE);
	event_add_window_icon_radio(list_pane, LIST_MM_ICON, FALSE);
	event_add_window_icon_radio(list_pane, LIST_POINT_ICON, FALSE);

	/* Default the display units to millimeters. */

	list_set_dimensions(LIST_UNITS_MM);

	/* Allocate a token amount of memory to initialise the flex block. */

	if (flex_alloc((flex_ptr) &list_index, 4) == 0)
		list_index = NULL;
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
 * Process mouse clicks in the list window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void list_click_handler(wimp_pointer *pointer)
{
	wimp_window_state	state;
	int			row, column;


	state.w = pointer->w;
	if (xwimp_get_window_state(&state) != NULL)
		return;

	row = list_calculate_window_click_row(&(pointer->pos), &state);
	column = list_calculate_window_click_column(&(pointer->pos), &state);

	switch(pointer->buttons) {
	case wimp_SINGLE_SELECT:
		list_select_click_select(row, column);
		break;

	case wimp_SINGLE_ADJUST:
		list_select_click_adjust(row, column);
		break;

	case wimp_DOUBLE_SELECT:
		list_double_click_select(row, column);
		break;
	}
}


/**
 * Process mouse clicks in the toolbar.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void list_toolbar_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch ((int) pointer->i) {
	case LIST_REFRESH_ICON:
		paper_read_definitions();
		windows_redraw(list_window);
		break;
	case LIST_SELECT_ICON:
		if (pointer->buttons == wimp_CLICK_SELECT)
			list_select_all();
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			list_select_none();
		break;
	case LIST_MM_ICON:
		list_set_dimensions(LIST_UNITS_MM);
		break;

	case LIST_INCH_ICON:
		list_set_dimensions(LIST_UNITS_INCH);
		break;

	case LIST_POINT_ICON:
		list_set_dimensions(LIST_UNITS_POINT);
		break;
	}
}


/**
 * Prepare the list window menu for (re)-opening.
 *
 * \param  w			The handle of the menu's parent window.
 * \param  *menu		Pointer to the menu being opened.
 * \param  *pointer		Pointer to the Wimp Pointer event block.
 */

static void list_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct paper_size	*paper;
	wimp_window_state	state;
	int			row;


	if (pointer != NULL) {
		state.w = pointer->w;
		if (xwimp_get_window_state(&state) != NULL)
			return;

		row = list_calculate_window_click_row(&(pointer->pos), &state);
		if (list_selection_count == 0) {
			list_select_click_select(row, LIST_COLUMN_PAPER_NAME);
			list_selection_from_menu = TRUE;
		} else {
			list_selection_from_menu = FALSE;
		}
	}


	if (list_selection_count == 1) {
		paper = paper_get_definitions();
		msgs_param_lookup("MenuPaper", menus_get_indirected_text_addr(list_window_menu, LIST_MENU_SELECTION), LIST_SELECT_MENU_LEN,
				paper[list_index[list_selection_row].index].name, NULL, NULL, NULL);
	} else {
		msgs_lookup("MenuSelection", menus_get_indirected_text_addr(list_window_menu, LIST_MENU_SELECTION), LIST_SELECT_MENU_LEN);
	}

	menus_tick_entry(list_window_dimension_menu, LIST_DIMENSION_MENU_MM, list_display_units == LIST_UNITS_MM);
	menus_tick_entry(list_window_dimension_menu, LIST_DIMENSION_MENU_INCH, list_display_units == LIST_UNITS_INCH);
	menus_tick_entry(list_window_dimension_menu, LIST_DIMENSION_MENU_POINT, list_display_units == LIST_UNITS_POINT);

	menus_shade_entry(list_window_menu, LIST_MENU_SELECTION, list_selection_count == 0);
	menus_shade_entry(list_window_menu, LIST_MENU_CLEAR_SELECTION, list_selection_count == 0);
	menus_shade_entry(list_window_selection_menu, LIST_SELECTION_MENU_WRITE, list_selection_count == 0);
	menus_shade_entry(list_window_selection_menu, LIST_SELECTION_MENU_RUN, list_selection_count == 0);
}


/**
 * Process submenu warning events for the list window menu.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void list_menu_warning(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
/*	switch (warning->selection.items[0]) {
	case RESULTS_MENU_SAVE:
		switch (warning->selection.items[1]) {
		case RESULTS_MENU_SAVE_RESULTS:
			saveas_prepare_dialogue(results_save_results);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		case RESULTS_MENU_SAVE_PATH_NAMES:
			saveas_prepare_dialogue(results_save_paths);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		case RESULTS_MENU_SAVE_SEARCH_OPTIONS:
			saveas_prepare_dialogue(results_save_options);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		}
		break;

	case RESULTS_MENU_OBJECT_INFO:
		results_object_info_prepare(handle);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	} */
}


/**
 * Handle selections from the list window menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param  *selection		Pointer to the Wimp menu selction block.
 */

static void list_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	wimp_pointer		pointer;


	wimp_get_pointer_info(&pointer);

	switch(selection->items[0]) {
//	case RESULTS_MENU_DISPLAY:
//		switch(selection->items[1]) {
//		case RESULTS_MENU_DISPLAY_PATH_ONLY:
//			results_set_display_mode(handle, FALSE);
//			break;

//		case RESULTS_MENU_DISPLAY_FULL_INFO:
//			results_set_display_mode(handle, TRUE);
//			break;
//		}
//		break;

	case LIST_MENU_SELECTION:
		switch(selection->items[1]) {
		case LIST_SELECTION_MENU_WRITE:
			list_write_selected_files();
			break;

		case LIST_SELECTION_MENU_RUN:
			list_launch_selected_files();
			break;
		}
		break;

	case LIST_MENU_SELECT_ALL:
		list_select_all();
		list_selection_from_menu = FALSE;
		break;

	case LIST_MENU_CLEAR_SELECTION:
		list_select_none();
		list_selection_from_menu = FALSE;
		break;

	case LIST_MENU_DIMENSION_UNITS:
		switch(selection->items[1]) {
		case LIST_DIMENSION_MENU_MM:
			list_set_dimensions(LIST_UNITS_MM);
			break;

		case LIST_DIMENSION_MENU_INCH:
			list_set_dimensions(LIST_UNITS_INCH);
			break;
			
		case LIST_DIMENSION_MENU_POINT:
			list_set_dimensions(LIST_UNITS_POINT);
			break;
		}
		break;

	case LIST_MENU_REFRESH:
		paper_read_definitions();
		windows_redraw(list_window);
		break;

//	case RESULTS_MENU_OPEN_PARENT:
//		if (handle->selection_count == 1)
//			results_open_parent(handle, handle->selection_row);
//		break;

//	case RESULTS_MENU_COPY_NAMES:
//		results_clipboard_copy_filenames(handle);
//		break;

//	case RESULTS_MENU_MODIFY_SEARCH:
//		file_create_dialogue(&pointer, NULL, NULL, file_get_dialogue(handle->file));
//		break;

//	case RESULTS_MENU_ADD_TO_HOTLIST:
//		hotlist_add_dialogue(file_get_dialogue(handle->file));
//		break;

//	case RESULTS_MENU_STOP_SEARCH:
//		file_stop_search(handle->file);
//		break;
	}
}


/**
 * Handle the closure of the list window menu.
 *
 * \param  w			The handle of the menu's parent window.
 * \param  *menu		Pointer to the menu being closee.
 */

static void list_menu_close(wimp_w w, wimp_menu *menu)
{
	if (list_selection_from_menu == FALSE)
		return;

	list_select_none();
	list_selection_from_menu = FALSE;
}


/**
 * Callback to handle redraw events on the list window.
 *
 * \param  *redraw		The Wimp redraw event block.
 */

static void list_redraw_handler(wimp_draw *redraw)
{
	struct paper_size	*paper;
	int			oy, top, bottom, y;
	osbool			more;
	wimp_icon		*icon;
	char			buffer[LIST_ICON_BUFFER_LEN], *unit_format, *token;
	double			unit_scale;

	/* ** This is a pointer to a flex block. If anything is done to make the
	 * ** heap shift before the end of the redraw process, things will
	 * ** go very badly wrong.
	 */

	paper = paper_get_definitions();

	icon = list_window_def->icons;

	switch (list_display_units) {
	case LIST_UNITS_MM:
		unit_scale = 2834.64567;
		unit_format = "%.1f";
		break;
	case LIST_UNITS_INCH:
		unit_scale = 72000.0;
		unit_format = "%.3f";
		break;
	case LIST_UNITS_POINT:
	default:
		unit_scale = 1000.0;
		unit_format = "%.1f";
		break;
	}

	/* Set up the buffers for the icons. */

	icon[LIST_WIDTH_ICON].data.indirected_text_and_sprite.text = buffer;
	icon[LIST_WIDTH_ICON].data.indirected_text_and_sprite.size = LIST_ICON_BUFFER_LEN;

	icon[LIST_HEIGHT_ICON].data.indirected_text_and_sprite.text = buffer;
	icon[LIST_HEIGHT_ICON].data.indirected_text_and_sprite.size = LIST_ICON_BUFFER_LEN;

	icon[LIST_SIZE_ICON].data.indirected_text_and_sprite.text = buffer;
	icon[LIST_SIZE_ICON].data.indirected_text_and_sprite.size = LIST_ICON_BUFFER_LEN;

	icon[LIST_STATUS_ICON].data.indirected_text_and_sprite.text = buffer;
	icon[LIST_STATUS_ICON].data.indirected_text_and_sprite.size = LIST_ICON_BUFFER_LEN;

	icon[LIST_SEPARATOR_ICON].data.indirected_text_and_sprite.text = buffer;
	icon[LIST_SEPARATOR_ICON].data.indirected_text_and_sprite.size = LIST_ICON_BUFFER_LEN;


	/* Redraw the window. */

	more = wimp_redraw_window(redraw);

	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
		top = (oy - redraw->clip.y1 - LIST_TOOLBAR_HEIGHT) / LIST_LINE_HEIGHT;
		if (top < 0)
			top = 0;

		bottom = ((LIST_LINE_HEIGHT * 1.5) + oy - redraw->clip.y0 - LIST_TOOLBAR_HEIGHT) / LIST_LINE_HEIGHT;
		if (bottom > list_index_count)
			bottom = list_index_count;

		for (y = top; y < bottom; y++) {
			switch (list_index[y].type) {
			case LIST_LINE_TYPE_SEPARATOR:
				/* Plot the separator icon. */

				icon[LIST_SEPARATOR_ICON].extent.y0 = LINE_Y0(y);
				icon[LIST_SEPARATOR_ICON].extent.y1 = LINE_Y1(y);

				switch(list_index[y].source) {
				case PAPER_SOURCE_MASTER:
					token = "PaperFileM";
					break;
				case PAPER_SOURCE_DEVICE:
					token = "PaperFileD";
					break;
				case PAPER_SOURCE_USER:
					token = "PaperFileU";
					break;
				default:
					token = "";
					break;
				}

				msgs_lookup(token, buffer, LIST_ICON_BUFFER_LEN);
				buffer[LIST_ICON_BUFFER_LEN - 1] = '\0';

				wimp_plot_icon(&(icon[LIST_SEPARATOR_ICON]));
				break;
			
			case LIST_LINE_TYPE_PAPER:
				/* Plot the Paper Name icon. */

				icon[LIST_NAME_ICON].extent.y0 = LINE_Y0(y);
				icon[LIST_NAME_ICON].extent.y1 = LINE_Y1(y);
				icon[LIST_NAME_ICON].data.indirected_text_and_sprite.text = paper[list_index[y].index].name;
				icon[LIST_NAME_ICON].data.indirected_text_and_sprite.size = PAPER_NAME_LEN;
				if (list_index[y].flags & LIST_LINE_FLAGS_SELECTED)
					icon[LIST_NAME_ICON].flags |= wimp_ICON_SELECTED;
				else
					icon[LIST_NAME_ICON].flags &= ~wimp_ICON_SELECTED;

				wimp_plot_icon(&(icon[LIST_NAME_ICON]));

				/* Plot the Width icon. */

				icon[LIST_WIDTH_ICON].extent.y0 = LINE_Y0(y);
				icon[LIST_WIDTH_ICON].extent.y1 = LINE_Y1(y);

				snprintf(buffer, LIST_ICON_BUFFER_LEN, unit_format, (double) (paper[list_index[y].index].width / unit_scale));
				buffer[LIST_ICON_BUFFER_LEN - 1] = '\0';

				wimp_plot_icon(&(icon[LIST_WIDTH_ICON]));

				/* Plot the height icon. */

				icon[LIST_HEIGHT_ICON].extent.y0 = LINE_Y0(y);
				icon[LIST_HEIGHT_ICON].extent.y1 = LINE_Y1(y);

				snprintf(buffer, LIST_ICON_BUFFER_LEN, unit_format, (double) (paper[list_index[y].index].height / unit_scale));
				buffer[LIST_ICON_BUFFER_LEN - 1] = '\0';

				wimp_plot_icon(&(icon[LIST_HEIGHT_ICON]));

				/* Plot the size status icon. */

				icon[LIST_SIZE_ICON].extent.y0 = LINE_Y0(y);
				icon[LIST_SIZE_ICON].extent.y1 = LINE_Y1(y);

				switch(paper[list_index[y].index].size_status) {
				case PAPER_SIZE_STATUS_UNKNOWN:
					token = "SizeStatUnkn";
					break;
				case PAPER_SIZE_STATUS_OK:
					token = "SizeStatOK";
					break;
				case PAPER_SIZE_STATUS_AMBIGUOUS:
					token = "SizeStatAmb";
					break;
				default:
					token = "";
					break;
				}

				msgs_lookup(token, buffer, LIST_ICON_BUFFER_LEN);
				buffer[LIST_ICON_BUFFER_LEN - 1] = '\0';

				wimp_plot_icon(&(icon[LIST_SIZE_ICON]));

				/* Plot the PS filename icon. */

				icon[LIST_FILENAME_ICON].extent.y0 = LINE_Y0(y);
				icon[LIST_FILENAME_ICON].extent.y1 = LINE_Y1(y);
				icon[LIST_FILENAME_ICON].data.indirected_text_and_sprite.text = paper[list_index[y].index].ps2_file;
				icon[LIST_FILENAME_ICON].data.indirected_text_and_sprite.size = PAPER_FILE_LEN;

				if (paper[list_index[y].index].ps2_file_status == PAPER_FILE_STATUS_MISSING)
					icon[LIST_FILENAME_ICON].flags |= wimp_ICON_SHADED;
				else
					icon[LIST_FILENAME_ICON].flags &= ~wimp_ICON_SHADED;

				wimp_plot_icon(&(icon[LIST_FILENAME_ICON]));

				/* Plot the PS file status icon. */

				icon[LIST_STATUS_ICON].extent.y0 = LINE_Y0(y);
				icon[LIST_STATUS_ICON].extent.y1 = LINE_Y1(y);

				switch(paper[list_index[y].index].ps2_file_status) {
				case PAPER_FILE_STATUS_MISSING:
					token = "PaperStatMiss";
					break;
				case PAPER_FILE_STATUS_UNKNOWN:
					token = "PaperStatUnkn";
					break;
				case PAPER_FILE_STATUS_CORRECT:
					token = "PaperStatOK";
					break;
				case PAPER_FILE_STATUS_INCORRECT:
					token = "PaperStatNOK";
					break;
				default:
					token = "";
					break;
				}

				msgs_lookup(token, buffer, LIST_ICON_BUFFER_LEN);
				buffer[LIST_ICON_BUFFER_LEN - 1] = '\0';

				wimp_plot_icon(&(icon[LIST_STATUS_ICON]));
				break;
			
			default:
				break;
			}
		}

		more = wimp_get_rectangle(redraw);
	}
}


/**
 * Request the List window to rebuild its index from the paper definitions.
 */

void list_rescan_paper_definitions(void)
{
	int			visible_extent, new_extent, new_scroll;
	size_t			paper_lines, index_size;
	struct paper_size	*paper;
	wimp_window_state	state;
	os_box			extent;

	paper_lines = paper_get_definition_count();
	index_size = paper_lines + 3;

	if (flex_extend((flex_ptr) &list_index, index_size * sizeof(struct list_redraw)) == 0)
		list_index = NULL;

	list_index_count = 0;
	list_selection_count = 0;
	list_selection_row = -1;
	list_selection_from_menu = FALSE;

	paper = paper_get_definitions();

	if (list_index != NULL) {
		list_add_paper_source_to_index(PAPER_SOURCE_MASTER, index_size, paper, paper_lines);
		list_add_paper_source_to_index(PAPER_SOURCE_DEVICE, index_size, paper, paper_lines);
		list_add_paper_source_to_index(PAPER_SOURCE_USER, index_size, paper, paper_lines);
	}

	state.w = list_window;
	wimp_get_window_state(&state);

	visible_extent = state.yscroll + (state.visible.y0 - state.visible.y1);

	new_extent = -((LIST_LINE_HEIGHT * index_size) + LIST_TOOLBAR_HEIGHT + (2 * LIST_WINDOW_MARGIN));

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
 * Add the paper definitions from a given source to the end of the paper
 * list index.
 * 
 * \param source		The target paper source to be added to the index.
 * \param index_lines		The number of index entries allocated for the process.
 * \param *paper		The paper list to process.
 * \param paper_lines		The number of paper definitions in the list.
 */

static void list_add_paper_source_to_index(enum paper_source source, size_t index_lines, struct paper_size *paper, size_t paper_lines)
{
	int i;

	if (list_index_count >= index_lines)
		return;

	list_index[list_index_count].type = LIST_LINE_TYPE_SEPARATOR;
	list_index[list_index_count].source = source;
	list_index[list_index_count].flags = LIST_LINE_FLAGS_NONE;

	list_index_count++;

	for (i = 0; i < paper_lines && list_index_count < index_lines; i++) {
		if (paper[i].source == source) {
			list_index[list_index_count].type = LIST_LINE_TYPE_PAPER;
			list_index[list_index_count].index = i;
			list_index[list_index_count].flags = LIST_LINE_FLAGS_NONE;
			list_index_count++;
		}
	}
}


/**
 * Turn a mouse position over the list window into an interactive
 * help token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void list_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int			row, column;
	wimp_window_state	window;

	*buffer = '\0';

	window.w = list_window;
	wimp_get_window_state(&window);

	row = list_calculate_window_click_row(&pos, &window);
	column = list_calculate_window_click_column(&pos, &window);

	if (row < 0 || row >= list_index_count)
		return;

	switch (list_index[row].type) {
	case LIST_LINE_TYPE_PAPER:
		sprintf(buffer, "Col%d", column);
		break;
	case LIST_LINE_TYPE_SEPARATOR:
		sprintf(buffer, "Separator");
		break;
	}
}


/**
 * Calculate the column that the mouse was clicked over in the list window.
 *
 * \param  *pointer		The Wimp pointer data.
 * \param  *state		The results window state.
 * \return			The column (from 0) or -1 if none.
 */

static int list_calculate_window_click_column(os_coord *pos, wimp_window_state *state)
{
	int	x;

	x = pos->x - state->visible.x0 + state->xscroll;

	return columns_find_pointer(list_columns, x);
}


/**
 * Calculate the row that the mouse was clicked over in the list window.
 *
 * \param  *pointer		The Wimp pointer data.
 * \param  *state		The results window state.
 * \return			The row (from 0) or -1 if none.
 */

static int list_calculate_window_click_row(os_coord *pos, wimp_window_state *state)
{
	int		y, row_y_pos, row;

	y = pos->y - state->visible.y1 + state->yscroll;

	row = ROW(y);
	row_y_pos = ROW_Y_POS(y);

	if (row >= list_index_count || ROW_ABOVE(row_y_pos) || ROW_BELOW(row_y_pos))
		row = -1;

	return row;
}


/**
 * Process a Select double-click over the list window.
 * 
 * \param row			The row under the double click, or -1.
 * \param column		The column under the double click, or -1.
 */

static void list_double_click_select(int row, int column)
{
	if ((row == -1) || (list_index[row].type != LIST_LINE_TYPE_PAPER) || (column != LIST_COLUMN_PAPER_FILE))
		return;

	paper_launch_file(list_index[row].index);
}

/**
 * Update the current selection based on a select click over a row of the
 * window.
 *
 * \param row			The row under the click, or -1.
 * \param column		The column under the click, or -1.
 */

static void list_select_click_select(int row, int column)
{
	wimp_window_state	window;

	/* If the click is on a selection, nothing changes. */

	if ((row != -1) && (row < list_index_count) && (column == LIST_COLUMN_PAPER_NAME) && (list_index[row].flags & LIST_LINE_FLAGS_SELECTED))
		return;

	/* Clear everything and then try to select the clicked line. */

	list_select_none();

	window.w = list_window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	if ((row == -1) || (column != LIST_COLUMN_PAPER_NAME) || (row >= list_index_count) || (list_index[row].type != LIST_LINE_TYPE_PAPER))
		return;

	list_index[row].flags |= LIST_LINE_FLAGS_SELECTED;
	list_selection_count++;
	if (list_selection_count == 1)
		list_selection_row = row;

	wimp_force_redraw(window.w, window.xscroll, LINE_BASE(row),
			window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(row));
}


/**
 * Update the current selection based on an adjust click over a row of the
 * window.
 *
 * \param row			The row under the click, or RESULTS_ROW_NONE.
 * \param column		The column under the click, or -1.
 */

static void list_select_click_adjust(int row, int column)
{
	int			i;
	wimp_window_state	window;

	if ((row == -1) || (column != LIST_COLUMN_PAPER_NAME) || (row >= list_index_count) || (list_index[row].type != LIST_LINE_TYPE_PAPER))
		return;

	window.w = list_window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	if (list_index[row].flags & LIST_LINE_FLAGS_SELECTED) {
		list_index[row].flags &= ~LIST_LINE_FLAGS_SELECTED;
		list_selection_count--;
		if (list_selection_count == 1) {
			for (i = 0; i < list_index_count; i++) {
				if (list_index[row].flags & LIST_LINE_FLAGS_SELECTED) {
					list_selection_row = i;
					break;
				}
			}
		}
	} else {
		list_index[row].flags |= LIST_LINE_FLAGS_SELECTED;
		list_selection_count++;
		if (list_selection_count == 1)
			list_selection_row = row;
	}

	wimp_force_redraw(window.w, window.xscroll, LINE_BASE(row),
			window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(row));
}


/**
 * Select all of the rows in the list window.
 */

static void list_select_all(void)
{
	int			i;
	wimp_window_state	window;

	if (list_selection_count == list_index_count)
		return;

	window.w = list_window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	for (i = 0; i < list_index_count; i++) {
		if (!(list_index[i].flags & LIST_LINE_FLAGS_SELECTED)) {
			list_index[i].flags |= LIST_LINE_FLAGS_SELECTED;

			list_selection_count++;
			if (list_selection_count == 1)
				list_selection_row = i;

			wimp_force_redraw(window.w, window.xscroll, LINE_BASE(i),
					window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(i));
		}
	}
}


/**
 * Clear the selection in the list window.
 */

static void list_select_none(void)
{
	int			i;
	wimp_window_state	window;

	if (list_selection_count == 0)
		return;

	window.w = list_window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	/* If there's just one row selected, we can avoid looping through the lot
	 * by just clearing that one line.
	 */

	if (list_selection_count == 1) {
		if (list_selection_row < list_index_count)
			list_index[list_selection_row].flags &= ~LIST_LINE_FLAGS_SELECTED;
		list_selection_count = 0;

		wimp_force_redraw(window.w, window.xscroll, LINE_BASE(list_selection_row),
				window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(list_selection_row));

		return;
	}

	/* If there is more than one row selected, we must loop through the lot
	 * to clear them all.
	 */

	for (i = 0; i < list_index_count; i++) {
		if (list_index[i].flags & LIST_LINE_FLAGS_SELECTED) {
			list_index[i].flags &= ~LIST_LINE_FLAGS_SELECTED;

			wimp_force_redraw(window.w, window.xscroll, LINE_BASE(i),
					window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(i));
		}
	}

	list_selection_count = 0;
}

static void list_write_selected_files(void)
{
	int	i;

	if (list_selection_count == 0)
		return;

	paper_ensure_ps2_file_folder();

	for (i = 0; i < list_index_count; i++) {
		if ((list_index[i].type == LIST_LINE_TYPE_PAPER) && (list_index[i].flags & LIST_LINE_FLAGS_SELECTED)) {
			paper_write_file(list_index[i].index);
		}
	}

	paper_read_definitions();
}

static void list_launch_selected_files(void)
{
	int	i;

	if (list_selection_count == 0)
		return;

	for (i = 0; i < list_index_count; i++) {
		if ((list_index[i].type == LIST_LINE_TYPE_PAPER) && (list_index[i].flags & LIST_LINE_FLAGS_SELECTED)) {
			paper_launch_file(list_index[i].index);
		}
	}
}

static void list_set_dimensions(enum list_units units)
{
	list_display_units = units;
	icons_set_radio_group_selected(list_pane, units, 3, LIST_MM_ICON, LIST_INCH_ICON, LIST_POINT_ICON);

	if (windows_get_open(list_window))
		windows_redraw(list_window);
}
