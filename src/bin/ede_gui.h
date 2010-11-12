/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifndef EDE_GUI_H
#define EDE_GUI_H

#include <Evas.h>
#include "ede_enemy.h"


#define CELL_W 25
#define CELL_H 25

#define WIN_W 800
#define WIN_H 600

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

EAPI void        ede_gui_tower_info_set(const char *name, const char *icon, const char *text);
EAPI void        ede_gui_wave_info_set(int tot, int cur, int num, const char *type);
EAPI void        ede_gui_wave_timer_update(int secs);

EAPI void        ede_gui_upgrade_box_hide_all(void);
EAPI void        ede_gui_upgrade_box_set(int pos, const char *name);


EAPI void      ede_gui_cell_overlay_add(Ede_Cell_Overlay overlay, int row, int col);
EAPI void      ede_gui_cell_overlay_text_set(int row, int col, int val, int pos);

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
