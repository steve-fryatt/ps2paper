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

enum columns_flags {
	COLUMNS_FLAGS_NONE = 0
};

struct columns_definition {
	wimp_i			column_icon;
	wimp_i			heading_icon;
	int			width;
	int			min_width;
	int			max_width;
	enum columns_flags	flags;
};

struct columns_block *columns_create_window(wimp_window *window_def, wimp_window *toolbar_def, struct columns_definition columns[], size_t column_count);
void columns_set_window_handle(struct columns_block *handle, wimp_w window);
void columns_set_toolbar_handle(struct columns_block *handle, wimp_w toolbar);
void columns_adjust_icons(struct columns_block *handle);

#endif
