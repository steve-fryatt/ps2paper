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
 * \file: ps2paper.c
 *
 * Postscript 2 paper dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/fileswitch.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "ps2paper.h"


#define PS2PAPER_NAME_LEN 128
#define PS2PAPER_SOURCE_LEN 32
#define PS2PAPER_ICON_HEIGHT 48

/* Paper Window icons. */

#define PS2PAPER_ICON_PANE 0

#define PS2PAPER_PANE_NAME 0
#define PS2PAPER_PANE_X 1
#define PS2PAPER_PANE_Y 2
#define PS2PAPER_PANE_SOURCE 3
#define PS2PAPER_PANE_FILE 4
#define PS2PAPER_PANE_STATUS 5

#define PS2PAPER_ICON_CLOSE 1
#define PS2PAPER_ICON_UPDATE 2
#define PS2PAPER_ICON_INCH 3
#define PS2PAPER_ICON_MM 4
#define PS2PAPER_ICON_POINT 5
//#define PAPER_ICON_SIZE_X 3
//#define PAPER_ICON_SIZE_Y 4

enum ps2paper_status {
	PS2PAPER_STATUS_MISSING,				/**< There is no file for the paper size.	*/
	PS2PAPER_STATUS_UNKNOWN,				/**< There is a file, but it's not one of ours.	*/
	PS2PAPER_STATUS_CORRECT,				/**< There is a file, and it matches the paper.	*/
	PS2PAPER_STATUS_INCORRECT				/**< There is a file, but the size is wrong.	*/
};

enum ps2paper_units {
	PS2PAPER_UNITS_MM = 0,
	PS2PAPER_UNITS_INCH = 1,
	PS2PAPER_UNITS_POINT = 2
};

struct ps2paper_size {
	char			name[PS2PAPER_NAME_LEN];	/**< The Printers name for the paper		*/
	int			width;				/**< The Printers width of the paper		*/
	int			height;				/**< The Printers height of the paper		*/
	char			source[PS2PAPER_SOURCE_LEN];	/**< The name of the source file		*/
	char			ps2_file[PS2PAPER_NAME_LEN];	/**< The associated PS2 Paper file, or ""	*/
	enum ps2paper_status	ps2_file_status;		/**< Indicate the status of the Paper File.	*/

	struct ps2paper_size	*next;				/**< Link to the next paper size.		*/
};

static struct ps2paper_size	*paper_sizes = NULL;		/**< Linked list of paper sizes.		*/
static unsigned			paper_count = 0;		/**< Number of defined paper sizes.		*/
static struct ps2paper_size	**redraw_list = NULL;		/**< Redraw list for paper sizes.		*/

static wimp_w			ps2paper_window = NULL;		/**< The paper list window.			*/
static wimp_w			ps2paper_pane = NULL;		/**< The paper list pane.			*/
static wimp_window		*ps2paper_pane_def = NULL;	/**< The paper list pane defintion.		*/

static enum ps2paper_units	page_display_unit = PS2PAPER_UNITS_MM;

static void	ps2paper_close_window(void);
static void	ps2paper_pane_redraw_handler(wimp_draw *redraw);
static void	ps2paper_click_handler(wimp_pointer *pointer);


/**
 * Initialise the ps2paper dialogue.
 */

void ps2paper_initialise(void)
{
	ps2paper_window = templates_create_window("PsPaper");
	ihelp_add_window(ps2paper_window, "PsPaper", NULL);

	ps2paper_pane_def = templates_load_window("PsPaperPane");
	ps2paper_pane_def->icon_count = 0;
	ps2paper_pane = wimp_create_window(ps2paper_pane_def);
	ihelp_add_window(ps2paper_pane, "PsPaperPane", NULL /*convert_decode_queue_pane_help*/);

	event_add_window_redraw_event(ps2paper_pane, ps2paper_pane_redraw_handler);
	//event_add_window_mouse_event(ps2paper_pane convert_queue_pane_click_handler);

	paper_count = 5;

	event_add_window_mouse_event(ps2paper_window, ps2paper_click_handler);
	//event_add_window_key_event(paper_window, paper_keypress_handler);

	event_add_window_icon_radio(ps2paper_window, PS2PAPER_ICON_MM, FALSE);
	event_add_window_icon_radio(ps2paper_window, PS2PAPER_ICON_INCH, FALSE);
	event_add_window_icon_radio(ps2paper_window, PS2PAPER_ICON_POINT, FALSE);
}


/**
 * Open the PS2 Paper dialogue.
 *
 * \param *pointer	The pointer location at which to open the window.
 */

void ps2paper_open_window(wimp_pointer *pointer)
{
	icons_set_selected(ps2paper_window, PS2PAPER_ICON_MM, (page_display_unit == PS2PAPER_UNITS_MM) ? TRUE : FALSE);
	icons_set_selected(ps2paper_window, PS2PAPER_ICON_INCH, (page_display_unit == PS2PAPER_UNITS_INCH) ? TRUE : FALSE);
	icons_set_selected(ps2paper_window, PS2PAPER_ICON_POINT, (page_display_unit == PS2PAPER_UNITS_POINT) ? TRUE : FALSE);

	windows_open_with_pane_centred_at_pointer(ps2paper_window, ps2paper_pane, PS2PAPER_ICON_PANE, 40, pointer);
	ps2paper_read_definitions();
}


/**
 * Close the PS2 Paper dialogue.
 */

static void ps2paper_close_window(void)
{
	wimp_close_window(ps2paper_window);
	ps2paper_clear_definitions();
}


/**
 * Process redraw requests for the PS2 Paper dialogue pane.
 *
 * \param *redraw		The redraw event block to handle.
 */

static void ps2paper_pane_redraw_handler(wimp_draw *redraw)
{
	int			ox, oy, top, base, y;
	osbool			more;
	wimp_icon		*icon;
	char			buffer[128], *status, *unit_format;
	double			unit_scale;

	/* Perform the redraw if a window was found. */

	if (redraw->w != ps2paper_pane)
		return;

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	icon = ps2paper_pane_def->icons;

	switch (page_display_unit) {
	case PS2PAPER_UNITS_MM:
		unit_scale = 2834.64567;
		unit_format = "%.1f";
		break;
	case PS2PAPER_UNITS_INCH:
		unit_scale = 72000.0;
		unit_format = "%.3f";
		break;
	case PS2PAPER_UNITS_POINT:
	default:
		unit_scale = 1000.0;
		unit_format = "%.1f";
		break;
	}

	while (more) {
		top = (oy - redraw->clip.y1) / PS2PAPER_ICON_HEIGHT;
		if (top < 0)
			top = 0;

		base = (PS2PAPER_ICON_HEIGHT + (PS2PAPER_ICON_HEIGHT / 2) + oy - redraw->clip.y0) / PS2PAPER_ICON_HEIGHT;

		for (y = top; y < paper_count && y <= base; y++) {
			icon[PS2PAPER_PANE_NAME].extent.y1 = -(y * PS2PAPER_ICON_HEIGHT);
			icon[PS2PAPER_PANE_NAME].extent.y0 = icon[PS2PAPER_PANE_NAME].extent.y1 - PS2PAPER_ICON_HEIGHT;
			icon[PS2PAPER_PANE_NAME].data.indirected_text_and_sprite.text = (redraw_list[y])->name;
			icon[PS2PAPER_PANE_NAME].data.indirected_text_and_sprite.size = PS2PAPER_NAME_LEN;

			wimp_plot_icon(&(icon[PS2PAPER_PANE_NAME]));

			snprintf(buffer, sizeof(buffer), unit_format, (double) (redraw_list[y])->width / unit_scale);

			icon[PS2PAPER_PANE_X].extent.y1 = -(y * PS2PAPER_ICON_HEIGHT);
			icon[PS2PAPER_PANE_X].extent.y0 = icon[PS2PAPER_PANE_X].extent.y1 - PS2PAPER_ICON_HEIGHT;
			icon[PS2PAPER_PANE_X].data.indirected_text_and_sprite.text = buffer;
			icon[PS2PAPER_PANE_X].data.indirected_text_and_sprite.size = sizeof(buffer);

			wimp_plot_icon(&(icon[PS2PAPER_PANE_X]));

			snprintf(buffer, sizeof(buffer), unit_format, (double) (redraw_list[y])->height / unit_scale);

			icon[PS2PAPER_PANE_Y].extent.y1 = -(y * PS2PAPER_ICON_HEIGHT);
			icon[PS2PAPER_PANE_Y].extent.y0 = icon[PS2PAPER_PANE_Y].extent.y1 - PS2PAPER_ICON_HEIGHT;
			icon[PS2PAPER_PANE_Y].data.indirected_text_and_sprite.text = buffer;
			icon[PS2PAPER_PANE_Y].data.indirected_text_and_sprite.size = sizeof(buffer);

			wimp_plot_icon(&(icon[PS2PAPER_PANE_Y]));

			icon[PS2PAPER_PANE_SOURCE].extent.y1 = -(y * PS2PAPER_ICON_HEIGHT);
			icon[PS2PAPER_PANE_SOURCE].extent.y0 = icon[PS2PAPER_PANE_SOURCE].extent.y1 - PS2PAPER_ICON_HEIGHT;
			icon[PS2PAPER_PANE_SOURCE].data.indirected_text_and_sprite.text = (redraw_list[y])->source;
			icon[PS2PAPER_PANE_SOURCE].data.indirected_text_and_sprite.size = PS2PAPER_SOURCE_LEN;

			wimp_plot_icon(&(icon[PS2PAPER_PANE_SOURCE]));

			icon[PS2PAPER_PANE_FILE].extent.y1 = -(y * PS2PAPER_ICON_HEIGHT);
			icon[PS2PAPER_PANE_FILE].extent.y0 = icon[PS2PAPER_PANE_FILE].extent.y1 - PS2PAPER_ICON_HEIGHT;
			icon[PS2PAPER_PANE_FILE].data.indirected_text_and_sprite.text = (redraw_list[y])->ps2_file;
			icon[PS2PAPER_PANE_FILE].data.indirected_text_and_sprite.size = PS2PAPER_NAME_LEN;

			if ((redraw_list[y])->ps2_file_status != PS2PAPER_STATUS_MISSING)
				wimp_plot_icon(&(icon[PS2PAPER_PANE_FILE]));

			switch((redraw_list[y])->ps2_file_status) {
			case PS2PAPER_STATUS_MISSING:
				status = "PaperStatMiss";
				break;
			case PS2PAPER_STATUS_UNKNOWN:
				status = "PaperStatUnkn";
				break;
			case PS2PAPER_STATUS_CORRECT:
				status = "PaperStatOK";
				break;
			case PS2PAPER_STATUS_INCORRECT:
				status = "PaperStatNOK";
				break;
			default:
				status = "";
				break;
			}

			msgs_lookup(status, buffer, sizeof(buffer));

			icon[PS2PAPER_PANE_STATUS].extent.y1 = -(y * PS2PAPER_ICON_HEIGHT);
			icon[PS2PAPER_PANE_STATUS].extent.y0 = icon[PS2PAPER_PANE_STATUS].extent.y1 - PS2PAPER_ICON_HEIGHT;
			icon[PS2PAPER_PANE_STATUS].data.indirected_text_and_sprite.text = buffer;
			icon[PS2PAPER_PANE_STATUS].data.indirected_text_and_sprite.size = sizeof(buffer);

			wimp_plot_icon(&(icon[PS2PAPER_PANE_STATUS]));


			/*icon[QUEUE_PANE_INCLUDE].extent.y1 = -(y * QUEUE_ICON_HEIGHT);
			icon[QUEUE_PANE_INCLUDE].extent.y0 = icon[QUEUE_PANE_INCLUDE].extent.y1 - QUEUE_ICON_HEIGHT;
			icon[QUEUE_PANE_INCLUDE].data.indirected_sprite.id =
					(osspriteop_id) (((queue_redraw_list[y])->include) ? "opton" : "optoff");
			icon[QUEUE_PANE_INCLUDE].data.indirected_sprite.area = (osspriteop_area *) 1;
			icon[QUEUE_PANE_INCLUDE].data.indirected_sprite.size = 12;

			wimp_plot_icon(&(icon[QUEUE_PANE_INCLUDE])); */


			/*icon[QUEUE_PANE_DELETE].extent.y1 = -(y * QUEUE_ICON_HEIGHT);
			icon[QUEUE_PANE_DELETE].extent.y0 = icon[QUEUE_PANE_DELETE].extent.y1 - QUEUE_ICON_HEIGHT;
			icon[QUEUE_PANE_DELETE].data.indirected_sprite.id =
					(osspriteop_id) (((queue_redraw_list[y])->object_type == DELETED) ? "del1" : "del0");
			icon[QUEUE_PANE_DELETE].data.indirected_sprite.area = main_wimp_sprites;
			icon[QUEUE_PANE_DELETE].data.indirected_sprite.size = 12;

			wimp_plot_icon(&(icon[QUEUE_PANE_DELETE])); */
		}

		more = wimp_get_rectangle (redraw);
	}
}


/**
 * Process mouse clicks in the PS2 Paper dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void ps2paper_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch ((int) pointer->i) {
	case PS2PAPER_ICON_MM:
		page_display_unit = PS2PAPER_UNITS_MM;
		windows_redraw(ps2paper_pane);
		break;

	case PS2PAPER_ICON_INCH:
		page_display_unit = PS2PAPER_UNITS_INCH;
		windows_redraw(ps2paper_pane);
		break;

	case PS2PAPER_ICON_POINT:
		page_display_unit = PS2PAPER_UNITS_POINT;
		windows_redraw(ps2paper_pane);
		break;

	case PS2PAPER_ICON_CLOSE:
		ps2paper_close_window();
		break;

	case PS2PAPER_ICON_UPDATE:
		ps2paper_update_files();
		ps2paper_close_window();
		break;
	}
}


