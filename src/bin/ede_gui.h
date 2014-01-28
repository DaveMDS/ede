/*
 *  Ede - EFL Defender Environment
 *  Copyright (C) 2010-2014 Davide Andreoli <dave@gurumeditation.it>
 *
 *  This file is part of Ede.
 *
 *  Ede is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ede is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ede.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EDE_GUI_H
#define EDE_GUI_H

#include <Evas.h>
#include "ede_enemy.h"
#include "ede_tower.h"


#define EDE_THEME_GENERATION "2"

#define CELL_W 25
#define CELL_H 25

#define LAYER_TOWER 1
#define LAYER_WALKER 2
#define LAYER_SELECTION 3
#define LAYER_FLYER 4
#define LAYER_BULLET 5
#define LAYER_MENU 6


typedef enum {
   OVERLAY_NONE,
   OVERLAY_IMAGE_WALL,
   OVERLAY_BORDER_RED,
   OVERLAY_BORDER_GREEN,
   OVERLAY_BORDER_BLUE,
   OVERLAY_COLOR_RED,
   OVERLAY_COLOR_GREEN,
   OVERLAY_COLOR_BLUE,
   OVERLAY_0,
   OVERLAY_1,
   OVERLAY_2,
   OVERLAY_3,
   OVERLAY_4,
   OVERLAY_5,
   OVERLAY_6,
   OVERLAY_7
}Ede_Cell_Overlay; //REMOVE

typedef enum {
   SELECTION_UNKNOW,
   SELECTION_FREE,
   SELECTION_WRONG,
   SELECTION_BLOCKING,
   SELECTION_TOWER
} Ede_Selection_Type;

EAPI Eina_Bool ede_gui_init(void);
EAPI Eina_Bool ede_gui_shutdown(void);

EAPI Evas       *ede_gui_canvas_get(void);
EAPI const char *ede_gui_theme_get(void);
EAPI Eina_Bool   ede_gui_level_init(int rows, int cols, const char *towers);
EAPI void        ede_gui_level_clear(void);

EAPI void        ede_gui_lives_set(int lives);
EAPI void        ede_gui_bucks_set(int bucks);
EAPI void        ede_gui_score_set(int score);

EAPI void        ede_gui_menu_show(const char *title);
EAPI void        ede_gui_menu_hide(void);
EAPI void        ede_gui_menu_item_add(const char *label1, const char *label2,
                                       void (*selected_cb)(void *data), void *data);

EAPI void        ede_gui_level_selector_show(void);
EAPI void        ede_gui_level_selector_hide(void);
EAPI void        ede_gui_level_selector_item_add(const char *label,
                                                 void (*selected_cb)(void *data),
                                                 void *data);

EAPI void        ede_gui_tower_info_set(const char *name, const char *icon, const char *info);
EAPI void        ede_gui_wave_info_set(int tot, int cur, int num, const char *type);
EAPI void        ede_gui_wave_timer_update(int secs);

EAPI void        ede_gui_upgrade_box_append(Ede_Tower_Class_Param *param, Ede_Tower_Class_Param_Upgrade *up);
EAPI void        ede_gui_upgrade_box_clear(void);


EAPI void      ede_gui_cell_overlay_add(Ede_Cell_Overlay overlay, int row, int col);
EAPI void      ede_gui_cell_overlay_text_set(int row, int col, int val, int pos);

EAPI Evas_Object *ede_gui_image_load(const char *image);
EAPI Eina_Bool ede_gui_cell_coords_get(int row, int col, int *x, int *y, Eina_Bool center);
EAPI Eina_Bool ede_gui_cell_get_at_coords(int x, int y, int *row, int *col);

EAPI void      ede_gui_selection_show_at(int row, int col, int rows, int cols, int radius);
EAPI void      ede_gui_selection_type_set(Ede_Selection_Type type);
EAPI void      ede_gui_selection_hide(void);

EAPI void      ede_gui_request_area(int w, int h, void (*done_cb)(int row, int col, int w, int h, void *data), void *data);
EAPI void      ede_gui_request_area_end(void);

EAPI void      ede_gui_tower_button_add(const char *tower_class_id);
EAPI void      ede_gui_tower_button_box_clear(void);

EAPI void      ede_gui_debug_text_set(const char *text);


#endif /* EDE_GUI_H */
