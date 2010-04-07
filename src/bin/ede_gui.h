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
EAPI Eina_Bool   ede_gui_level_init(int rows, int cols);
EAPI void        ede_gui_level_clear(void);

EAPI void        ede_gui_tower_info_set(const char *name, const char *icon, const char *text);

EAPI void      ede_gui_cell_overlay_add(Ede_Cell_Overlay overlay, int row, int col);
EAPI void      ede_gui_cell_overlay_text_set(int row, int col, int val, int pos);

EAPI Eina_Bool ede_gui_cell_coords_get(int row, int col, int *x, int *y, Eina_Bool center);
EAPI Eina_Bool ede_gui_cell_get_at_coords(int x, int y, int *row, int *col);

EAPI void      ede_gui_selection_show_at(int row, int col, int rows, int cols, int radius);
EAPI void      ede_gui_selection_type_set(Ede_Selection_Type type);
EAPI void      ede_gui_selection_hide(void);

EAPI void      ede_gui_request_area(int w, int h, void (*done_cb)(int row, int col, int w, int h, void *data), void *data);



#endif /* EDE_GUI_H */
