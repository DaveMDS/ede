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

#define TOWER_DEFAULT_DAMAGE 40
#define TOWER_DEFAULT_RANGE 250
#define TOWER_DEFAULT_RELOAD 1


/* structure to define a single tower */
typedef struct _Ede_Tower Ede_Tower;
struct _Ede_Tower {
   Ede_Tower_Class *class;

   Evas_Object *o_base;
   Evas_Object *o_cannon;

   int row, col, rows, cols; // position & size. In cells
   int center_x, center_y;   // center position. In pixel
   int range, damage, reload;

   double reload_counter; // accumulator for reloading
};


/* Local subsystem vars */
static Eina_List *tower_classes = NULL;
static Eina_List *alive_towers = NULL; // TODO rename to alive_towers
static Ede_Tower *selected_tower = NULL;


/* Local subsystem callbacks */


/* Local subsystem functions */

static void
_tower_add_real(int row, int col, int rows, int cols, void *data)
{
   Ede_Tower_Class *tc = data;
   Ede_Tower *tower;
   int x, y, i, j;

   if (!tc) return;

   D("%s tower at: %d %d", tc->id, row, col);

   // alloc & fill tower structure
   tower = EDE_NEW(Ede_Tower);
   if (!tower) return;

   tower->class = tc;
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
   tower->o_base = ede_gui_image_load(tc->image1);
   evas_object_pass_events_set(tower->o_base, EINA_TRUE);
   evas_object_layer_set(tower->o_base, LAYER_TOWER);
   evas_object_resize(tower->o_base, CELL_W * cols, CELL_H * rows);
   evas_object_move(tower->o_base, x, y);

   // add the rotating sprite
   tower->o_cannon = ede_gui_image_load(tc->image2);
   evas_object_pass_events_set(tower->o_cannon, EINA_TRUE);
   evas_object_layer_set(tower->o_cannon, LAYER_TOWER);
   evas_object_resize(tower->o_cannon, CELL_W * cols, CELL_H * rows);
   evas_object_move(tower->o_cannon, x, y);

   // mark all the tower cells as unwalkable
   for (i = col; i < col + cols; i++)
      for (j = row; j < row + rows; j++)
         cells[j][i] = CELL_TOWER;

   // tell the enemies that the grid has changed
   ede_enemy_path_recalc_all();

   // add to the towers list
   alive_towers = eina_list_append(alive_towers, tower);
}

static void
_tower_del(Ede_Tower *tower)
{
   Ede_Level *level;
   int i, j;

   // mark all the tower cells as empty
   level = ede_level_current_get();
   for (i = tower->col; i < tower->col + tower->cols; i++)
      for (j = tower->row; j < tower->row + tower->rows; j++)
         cells[j][i] = CELL_EMPTY;

   // tell the enemies that the grid has changed
   ede_enemy_path_recalc_all();

   // hide the selection
   ede_gui_selection_hide();

   // free stuff
   alive_towers = eina_list_remove(alive_towers, tower);
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

   ede_gui_tower_info_set(tower->class->name, tower->class->icon, buf);

   ede_gui_upgrade_box_hide_all();
   ede_gui_upgrade_box_set(0, "Level UP");
   ede_gui_upgrade_box_set(1, "Damage");
   ede_gui_upgrade_box_set(2, "Range");
   ede_gui_upgrade_box_set(3, "Reload");

   ede_gui_selection_type_set(SELECTION_TOWER);
   ede_gui_selection_show_at(tower->row, tower->col, tower->rows, tower->cols, tower->range);
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

/* tower class stuff */
static void
_tower_class_del(Ede_Tower_Class *tc)
{
   // free stuff
   if (tc->id) eina_stringshare_del(tc->id);
   if (tc->name) eina_stringshare_del(tc->name);
   if (tc->engine) eina_stringshare_del(tc->engine);
   if (tc->desc) eina_stringshare_del(tc->desc);
   if (tc->icon) eina_stringshare_del(tc->icon);
   if (tc->image1) eina_stringshare_del(tc->image1);
   if (tc->image2) eina_stringshare_del(tc->image2);
   if (tc->image3) eina_stringshare_del(tc->image3);
   EDE_FREE(tc);
}
static void
_parse_tower_class_file(const char *path)
{
   Ede_Tower_Class *tc;
   //~ Ede_Level *level;
   //~ Eina_List *levels = NULL;
   char line[PATH_MAX], id[64], name[64], eng[64], desc[256];//, str[256];
   char icon[64], im1[64], im2[64], im3[64];
   int cost = 0;
   double sell_factor = 1.0;
   FILE *fp;

   D("parse tower from file: %s", path);

   // open the tower file
   fp = fopen(path, "r");
   if (fp == NULL) return;
   // parse it
   id[0] = name[0] = eng[0] = desc[0] = icon[0] = im1[0] = im2[0] = im3[0] = '\0';
   while (fgets(line, sizeof(line), fp) != NULL)
   {
      //~ D("%s", line);
      if (line[0] == '#') continue;

      if (sscanf(line, "ID=%[^\n]", id) == 1) {}
      if (sscanf(line, "Name=%[^\n]", name) == 1) {}
      else if(sscanf(line, "Engine=%[^\n]", eng) == 1) {}
      else if(sscanf(line, "Description=%[^\n]", desc) == 1) {}
      else if(sscanf(line, "Icon=%[^\n]", icon) == 1) {}
      else if(sscanf(line, "Image1=%[^\n]", im1) == 1) {}
      else if(sscanf(line, "Image2=%[^\n]", im2) == 1) {}
      else if(sscanf(line, "Image3=%[^\n]", im3) == 1) {}
      else if(sscanf(line, "Cost=%d", &cost) == 1) {}
      else if(sscanf(line, "SellFactor=%lf", &sell_factor) == 1) {}
      //~ else if(sscanf(line, "Level=%[^\n]", str) == 1)
      //~ {
         //~ level = _load_level_header(str);
         //~ levels = eina_list_append(levels, level);
      //~ }
   }
   //close
   fclose(fp);

   // alloc the Ede_Tower_Class struct
   if (id[0] && name[0] && eng[0] && desc[0] && icon[0])
   {
      tc = EDE_NEW(Ede_Tower_Class);
      if (!tc) return;
      tc->id = eina_stringshare_add(id);
      tc->name = eina_stringshare_add(name);
      tc->engine = eina_stringshare_add(eng);
      tc->desc = eina_stringshare_add(desc);
      tc->icon = eina_stringshare_add(icon);
      if (im1[0]) tc->image1 = eina_stringshare_add(im1);
      if (im2[0]) tc->image2 = eina_stringshare_add(im2);
      if (im3[0]) tc->image3 = eina_stringshare_add(im3);
      tc->cost = cost;
      tc->sell_factor = sell_factor;
      //~ sce->levels = levels;
      tower_classes = eina_list_append(tower_classes, tc);
      return;
   }
}

/* Externally accessible functions */
EAPI Eina_Bool
ede_tower_init(void)
{
   Eina_Iterator *files;
   char *f;
   D(" ");

   // read all the classes from the '.towers' files in the 'towers/' dir
   // and fill the tower_classes list
   files = eina_file_ls(PACKAGE_DATA_DIR"/towers/");
   EINA_ITERATOR_FOREACH(files, f)
      if (eina_str_has_suffix(f, ".tower"))
         _parse_tower_class_file(f);
   eina_iterator_free(files);
   // TODO CHECK ALSO IN USER DIR

   // TODO FREE THE tower_classes list at the end
   
   // DEBUG  dump classes
   Eina_List *l;
   Ede_Tower_Class *tc;
   EINA_LIST_FOREACH(tower_classes, l, tc)
   {
      D("********************");
      D("id: %s", tc->id);
      D("name: %s", tc->name);
      D("engine: %s", tc->engine);
      D("desc: %s", tc->desc);
      D("icon: %s", tc->icon);
      D("image1: %s", tc->image1);
      D("image2: %s", tc->image2);
      D("image3: %s", tc->image3);
      D("cost: %d", tc->cost);
      D("sell factor: %.2f", tc->sell_factor);
   }
   // END DEBUG

   return EINA_TRUE;
}

EAPI Eina_Bool
ede_tower_shutdown(void)
{
   Ede_Tower *tower;
   Ede_Tower_Class *tc;
   D(" ");

   EINA_LIST_FREE(alive_towers, tower)
      _tower_del(tower);

   EINA_LIST_FREE(tower_classes, tc)
      _tower_class_del(tc);

   return EINA_TRUE;
}

EAPI Ede_Tower_Class *
ede_tower_class_get_by_id(const char *id)
{
   Ede_Tower_Class *tc;
   Eina_List *l;

   EINA_LIST_FOREACH(tower_classes, l, tc)
      if (strcmp(tc->id, id) == 0)
         return tc;

   return NULL;
}

EAPI void
ede_tower_add(Ede_Tower_Class *tc)
{
   ede_gui_request_area(2, 2, _tower_add_real, tc);
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

   EINA_LIST_FOREACH(alive_towers, l, tower)
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
ede_tower_reset(void)
{
   Ede_Tower *tower;

   ede_gui_selection_hide();
   EINA_LIST_FREE(alive_towers, tower)
   {
      EDE_OBJECT_DEL(tower->o_base);
      EDE_OBJECT_DEL(tower->o_cannon);
      EDE_FREE(tower);
   }
   selected_tower = NULL;
}

EAPI void
ede_tower_upgrade(int button_num)
{
   if (!selected_tower) return;
   printf("UPGRADE %d\n", button_num);
}


EAPI void
ede_tower_one_step_all(double time)
{
   Ede_Tower *tower;
   Eina_List *l;

   //~ D("STEP [time %f]", time);
   EINA_LIST_FOREACH(alive_towers, l, tower)
      _tower_step(tower, time);
}

EAPI void
ede_tower_debug_info_fill(Eina_Strbuf *t)
{
   eina_strbuf_append(t, "<h3>towers:</h3><br>");
   eina_strbuf_append_printf(t, "count %d<br>", eina_list_count(alive_towers));
   eina_strbuf_append(t, "<br>");
}
