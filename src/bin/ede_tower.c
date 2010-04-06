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

#define EDE_MAX_TOWERS 500

#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif


typedef struct _Ede_Tower Ede_Tower;
struct _Ede_Tower {
   Ede_Tower_Type type;

   Evas_Object *o_base;
   Evas_Object *o_cannon;

   int row, col, rows, cols; // position, size
   int range, damage, reload;
};


// tower type names
static const char *_type_name[TOWER_TYPE_NUM] = {
   "unknow",
   "normal",
   "ghost",
   "powerup",
   "slowdown",
};
static const char *_type_long_name[TOWER_TYPE_NUM] = {
   "unknow",
   "Normal Tower",
   "Ghost Tower",
   "DamageUP Tower",
   "SlowDown Tower",
};


/* protos */
static void _tower_select(Ede_Tower *tower);

/* Local subsystem vars */
static Eina_List *towers = NULL;


/* Local subsystem callbacks */
static void
_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Ede_Tower *tower = data;
   
   _tower_select(tower);
}

/* Local subsystem functions */

static void
_tower_add_real(int row, int col, int w, int h, void *data)
{
   Ede_Tower_Type tt = (Ede_Tower_Type)data;
   Ede_Level *level = ede_level_current_get();
   Ede_Tower *tower;
   int x, y, i, j;
   char buf[64];

   D("%s tower at: %d %d", _type_name[tt], row, col);

   if (tt == TOWER_UNKNOW) return;

   // alloc & fill enemy struct
   tower = EDE_NEW(Ede_Tower);
   if (!tower) return;
   tower->type = tt;
   tower->row = row;
   tower->col = col;
   tower->cols = w;
   tower->rows = h;
   tower->damage = 10;
   tower->range = 300;
   tower->reload = 20;

   // add the base sprite
   ede_gui_cell_coords_get(row, col, &x, &y, EINA_FALSE);
   snprintf(buf, sizeof(buf), PACKAGE_DATA_DIR"/themes/tower_%s_base.png", _type_name[tt]);
   tower->o_base = evas_object_image_filled_add(ede_gui_canvas_get());
   evas_object_image_file_set(tower->o_base, buf, NULL);
   evas_object_event_callback_add(tower->o_base, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, tower);
   evas_object_resize(tower->o_base, CELL_W * w, CELL_H *h);
   evas_object_show(tower->o_base);
   evas_object_move(tower->o_base, x, y);

   // add the rotating sprite
   snprintf(buf, sizeof(buf), PACKAGE_DATA_DIR"/themes/tower_%s_cannon.png", _type_name[tt]);
   tower->o_cannon = evas_object_image_filled_add(ede_gui_canvas_get());
   evas_object_image_file_set(tower->o_cannon, buf, NULL);
   evas_object_pass_events_set(tower->o_cannon, EINA_TRUE);
   evas_object_resize(tower->o_cannon, CELL_W * w, CELL_H *h);
   evas_object_show(tower->o_cannon);
   evas_object_move(tower->o_cannon, x, y);

   // mark all the tower cells as unwalkable
   for (i = col; i < col + w; i++)
      for (j = row; j < row + h; j++)
         level->cells[j][i] = CELL_TOWER;

   // add to the towers list
   towers = eina_list_append(towers, tower);

   // select the tower
   _tower_select(tower);
}

static void
_tower_del(Ede_Tower *tower)
{
   //  TODO clear the level cells
   EDE_OBJECT_DEL(tower->o_base);
   EDE_OBJECT_DEL(tower->o_cannon);
   EDE_FREE(tower);
}

static void
_tower_step(Ede_Tower *tower, double time)
{
   static int angle = 0;
   
   //~ D("STEP %d [time %f]", e->id, time);
   //~ ede_gui_sprite_rotate(e->id + 1, angle);
   //~ angle += 2;
}

static void
_tower_select(Ede_Tower *tower)
{
   char buf[128];
   
   D(" ");
   snprintf(buf, sizeof(buf), "damage: %d<br>range: %d<br>reload: %d",
                 tower->damage, tower->range, tower->reload);
   ede_gui_tower_info_set(_type_long_name[tower->type], _type_name[tower->type], buf);
   ede_gui_selection_show_at(tower->row, tower->col, tower->rows, tower->cols, tower->range);
   ede_gui_selection_type_set(SELECTION_TOWER);
}




/* Externally accessible functions */

/**
 * TODO
 */
EAPI Eina_Bool
ede_tower_init(void)
{
   D(" ");

   return EINA_TRUE;
}

/**
 * TODO
 */
EAPI Eina_Bool
ede_tower_shutdown(void)
{
   Ede_Tower *tower;
   D(" ");
   
   EINA_LIST_FREE(towers, tower)
      _tower_del(tower);
   
   return EINA_TRUE;
}

/**
 * TODO
 */
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

/**
 * TODO
 */
EAPI void
ede_tower_one_step_all(double time)
{
   Ede_Tower *tower;
   Eina_List *l;

   EINA_LIST_FOREACH(towers, l, tower)
      _tower_step(tower, time);
}
