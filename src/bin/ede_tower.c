/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#include <stdio.h>
#include <Eina.h>
#include <Evas.h>

#include "ede.h"
#include "ede_tower.h"
#include "ede_gui.h"
#include "ede_level.h"
#include "ede_bullet.h"
#include "ede_utils.h"


#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif

#define TOWER_DEFAULT_DAMAGE 10
#define TOWER_DEFAULT_RANGE 200
#define TOWER_DEFAULT_RELOAD 1

typedef struct _Ede_Tower Ede_Tower;
struct _Ede_Tower {
   Ede_Tower_Type type;

   Evas_Object *o_base;
   Evas_Object *o_cannon;

   int row, col, rows, cols; // position & size. In cells
   int center_x, center_y;   // center position. In pixel
   int range, damage, reload;

   double reload_counter; // accumulator for reloading
};

// tower type names
static const char *_type_name[TOWER_TYPE_NUM] = {
   "unknow",
   "normal",
   "ghost",
   "powerup",
   "slowdown",
};
// tower type long names
static const char *_type_long_name[TOWER_TYPE_NUM] = {
   "unknow",
   "Normal Tower",
   "Ghost Tower",
   "DamageUP Tower",
   "SlowDown Tower",
};


/* Local subsystem vars */
static Eina_List *towers = NULL;
static Ede_Tower *selected_tower = NULL;


/* Local subsystem callbacks */


/* Local subsystem functions */

static void
_tower_add_real(int row, int col, int rows, int cols, void *data)
{
   Ede_Tower_Type tt = (Ede_Tower_Type)data;
   Ede_Level *level = ede_level_current_get();
   Ede_Tower *tower;
   int x, y, i, j;
   char buf[64];

   D("%s tower at: %d %d", _type_name[tt], row, col);

   if (tt == TOWER_UNKNOW) return;

   // alloc & fill tower structure
   tower = EDE_NEW(Ede_Tower);
   if (!tower) return;

   tower->type = tt;
   tower->row = row;
   tower->col = col;
   tower->cols = cols;
   tower->rows = rows;
   tower->damage = TOWER_DEFAULT_DAMAGE;
   tower->range = TOWER_DEFAULT_RANGE;
   tower->reload = TOWER_DEFAULT_RELOAD;
   ede_gui_cell_coords_get(row, col, &x, &y, EINA_FALSE);
   tower->center_x = x + (cols * CELL_W / 2);
   tower->center_y = y + (rows * CELL_H / 2);

   // add the base sprite
   snprintf(buf, sizeof(buf),
            PACKAGE_DATA_DIR"/themes/tower_%s_base.png", _type_name[tt]);
   tower->o_base = evas_object_image_filled_add(ede_gui_canvas_get());
   evas_object_image_file_set(tower->o_base, buf, NULL);
   evas_object_resize(tower->o_base, CELL_W * cols, CELL_H * rows);
   evas_object_show(tower->o_base);
   evas_object_move(tower->o_base, x, y);

   // add the rotating sprite
   snprintf(buf, sizeof(buf),
            PACKAGE_DATA_DIR"/themes/tower_%s_cannon.png", _type_name[tt]);
   tower->o_cannon = evas_object_image_filled_add(ede_gui_canvas_get());
   evas_object_image_file_set(tower->o_cannon, buf, NULL);
   evas_object_resize(tower->o_cannon, CELL_W * cols, CELL_H * rows);
   evas_object_show(tower->o_cannon);
   evas_object_move(tower->o_cannon, x, y);

   // mark all the tower cells as unwalkable
   for (i = col; i < col + cols; i++)
      for (j = row; j < row + rows; j++)
         level->cells[j][i] = CELL_TOWER;

   // tell the enemies that the grid has changed
   ede_enemy_path_recalc_all();

   // add to the towers list
   towers = eina_list_append(towers, tower);
}

static void
_tower_del(Ede_Tower *tower)
{
   Ede_Level *level;
   int i, j;

   // mark all the tower cells as unwalkable
   level = ede_level_current_get();
   for (i = tower->col; i < tower->col + tower->cols; i++)
      for (j = tower->row; j < tower->row + tower->rows; j++)
         level->cells[j][i] = CELL_EMPTY;

   // tell the enemies that the grid has changed
   ede_enemy_path_recalc_all();

   // hide the selection
   ede_gui_selection_hide();

   // free stuff
   towers = eina_list_remove(towers, tower);
   EDE_OBJECT_DEL(tower->o_base);
   EDE_OBJECT_DEL(tower->o_cannon);
   EDE_FREE(tower);
}

static void
_tower_select(Ede_Tower *tower)
{
   char buf[128];

   D(" ");
   selected_tower = tower;
   snprintf(buf, sizeof(buf), "damage: %d<br>range: %d<br>reload: %d",
                 tower->damage, tower->range, tower->reload);
   ede_gui_tower_info_set(_type_long_name[tower->type], _type_name[tower->type], buf);
   ede_gui_selection_show_at(tower->row, tower->col, tower->rows, tower->cols, tower->range);
   ede_gui_selection_type_set(SELECTION_TOWER);
}

static void
_tower_shoot_at(Ede_Tower *tower, Ede_Enemy *e)
{
   ede_bullet_add(tower->center_x, tower->center_y, e, 1, tower->damage);
   tower->reload_counter = (float)(tower->reload) / 10;
}

static void
_tower_step(Ede_Tower *tower, double time)
{
   Ede_Enemy *e;
   int angle = 0;
   int distance = 0;

   // check reload time
   tower->reload_counter -= time;
   if (tower->reload_counter > 0)
      return;

   // fire to the closest enemy (if in range)
   // TODO no need to get the nearest every reload, every 1 or 2 seconds is enoughts
   e = ede_enemy_nearest_get(tower->center_x, tower->center_y, &angle, &distance);
   if (distance < tower->range)
   {
      ede_util_obj_rotate(tower->o_cannon, angle);
      _tower_shoot_at(tower, e);
   }
}


/* Externally accessible functions */
EAPI Eina_Bool
ede_tower_init(void)
{
   D(" ");

   return EINA_TRUE;
}

EAPI Eina_Bool
ede_tower_shutdown(void)
{
   Ede_Tower *tower;
   D(" ");
   
   EINA_LIST_FREE(towers, tower)
      _tower_del(tower);
   
   return EINA_TRUE;
}

EAPI void
ede_tower_add(const char *type)
{
   int i;
   Ede_Tower_Type tt;
   
   D("%s", type);

   // set tower_type 
   tt = TOWER_UNKNOW;
   for (i = 0; i < TOWER_TYPE_NUM; i++)
      if (!strcmp(type, _type_name[i]))
         tt = TOWER_UNKNOW + i;

   ede_gui_request_area(2, 2, _tower_add_real, (void*)tt);
}

EAPI void
ede_tower_destroy_selected(void)
{
   if (selected_tower)
      _tower_del(selected_tower);
   selected_tower = NULL;
}

EAPI void
ede_tower_deselect(void)
{
   if (selected_tower)
   {
      selected_tower = NULL;
      ede_gui_selection_hide();
      ede_gui_tower_info_set(NULL, NULL, NULL);
   }
}

EAPI void
ede_tower_select_at(int row, int col)
{
   Ede_Tower *tower;
   Eina_List *l;

   D("%d %d", row, col);

   EINA_LIST_FOREACH(towers, l, tower)
   {
      if (row >= tower->row && row < tower->row + tower->rows &&
          col >= tower->col && col < tower->col + tower->cols)
      {
         _tower_select(tower);
         return;
      }
   }
}

EAPI void
ede_tower_one_step_all(double time)
{
   Ede_Tower *tower;
   Eina_List *l;

   //~ D("STEP [time %f]", time);
   EINA_LIST_FOREACH(towers, l, tower)
      _tower_step(tower, time);
}

EAPI void
ede_tower_debug_info_fill(Eina_Strbuf *t)
{
   char buf[1024];

   eina_strbuf_append(t, "<h3>towers:</h3><br>");
   snprintf(buf, sizeof(buf), "count %d<br>",eina_list_count(towers));
   eina_strbuf_append(t, buf);
   eina_strbuf_append(t, "<br>");
}
