/* Copyright 2016, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of PS2Paper:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file: list.h
 *
 * Paper list window implementation.
 */

#ifndef PS2PAPER_LIST
#define PS2PAPER_LIST

#include <stdio.h>

#include "oslib/osspriteop.h"


/**
 * Initialise the list window.
 *
 * \param *sprites		The application sprite area.
 */

void list_initialise(osspriteop_area *sprites);


/**
 * Open the List window centred on the screen.
 */

void list_open_window(void);


/**
 * Request the List window to rebuild its index from the paper definitions.
 */

void list_rescan_paper_definitions(void);

#endif
