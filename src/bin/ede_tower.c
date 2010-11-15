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
#include <Edje.h>

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


/* Local subsystem vars */
static Eina_List *tower_classes = NULL;
static Eina_List *alive_towers = NULL; // TODO rename to alive_towers
static Ede_Tower *selected_tower = NULL;


/* Local subsystem callbacks */


/* Local subsystem functions */

Ede_Tower_Class_Param * // unused
_tower_class_param_get(Ede_Tower_Class *tc, const char *param, int up_level)
{
   Ede_Tower_Class_Param *par;
   Eina_List *l;
   EINA_LIST_FOREACH(tc->params, l, par)
      if (streql(par->name, param))
         return par;
   return NULL;
}// unused

static void
_tower_add_real(int row, int col, int rows, int cols, void *data)
{
   Ede_Tower_Class *tc = data;
   Ede_Tower_Class_Param *par;
   Ede_Tower *tower;
   Eina_List *l;
   char buf[64];
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

   // set params upgrade to level 0
   EINA_LIST_FOREACH(tc->params, l, par)
   {
      Ede_Tower_Class_Param_Upgrade *base;
      Ede_Tower_Class_Param_Upgrade *up;
      
      base = eina_list_data_get(par->upgrades);
      up = eina_list_data_get(eina_list_next(par->upgrades));
      if (streql(par->name, "Reload"))
      {
         tower->reload = base->value;
         tower->reload_up = up;
      }
      else if (streql(par->name, "Damage"))
      {
         tower->damage = base->value;
         tower->damage_up = up;
      }
      else if (streql(par->name, "Range"))
      {
         tower->range = base->value;
         tower->range_up = up;
      }
   }

   // calc center
   ede_gui_cell_coords_get(row, col, &x, &y, EINA_FALSE);
   tower->center_x = x + (cols * CELL_W / 2);
   tower->center_y = y + (rows * CELL_H / 2);

   // create the edje object
   tower->obj = edje_object_add(ede_gui_canvas_get());
   snprintf(buf, sizeof(buf), "ede/tower/%s", tc->id);;
   edje_object_file_set(tower->obj, ede_gui_theme_get(), buf);
   //~ evas_object_pass_events_set(tower->obj, EINA_TRUE);
   evas_object_layer_set(tower->obj, LAYER_TOWER);
   evas_object_resize(tower->obj, CELL_W * cols, CELL_H * rows);
   evas_object_move(tower->obj, x, y);
   evas_object_show(tower->obj);

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
   EDE_OBJECT_DEL(tower->obj);
   EDE_FREE(tower);
}

static void
_tower_select(Ede_Tower *tower)
{
   D(" ");
   selected_tower = tower;
   ede_gui_tower_info_set(tower);
   ede_gui_selection_type_set(SELECTION_TOWER);
   ede_gui_selection_show_at(tower->row, tower->col, tower->rows, tower->cols,
                             tower->range);
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
   double fangle = 0.0;
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
      fangle = angle;
      edje_object_message_send(tower->obj, EDJE_MESSAGE_FLOAT, 1, &fangle);
      _tower_shoot_at(tower, e);
   }
}

/* tower class stuff */
static void
_tower_class_del(Ede_Tower_Class *tc)
{
   Ede_Tower_Class_Param *par;
   Ede_Tower_Class_Param_Upgrade *up;
   
   if (tc->id) eina_stringshare_del(tc->id);
   if (tc->name) eina_stringshare_del(tc->name);
   if (tc->engine) eina_stringshare_del(tc->engine);
   if (tc->desc) eina_stringshare_del(tc->desc);
   if (tc->icon) eina_stringshare_del(tc->icon);
   if (tc->image1) eina_stringshare_del(tc->image1);
   if (tc->image2) eina_stringshare_del(tc->image2);
   if (tc->image3) eina_stringshare_del(tc->image3);
   EINA_LIST_FREE(tc->params, par)
   {
      if (par->name) eina_stringshare_del(par->name);
      if (par->icon) eina_stringshare_del(par->icon);
      EINA_LIST_FREE(par->upgrades, up)
      {
         if (up->name) eina_stringshare_del(up->name);
         EDE_FREE(up);
      }
      EDE_FREE(par);
   }
   EDE_FREE(tc);
}

static void
_parse_tower_class_file(const char *path)
{
   Eina_List *params = NULL;
   char line[PATH_MAX], id[64], name[64], eng[64], desc[256];
   char icon[64], icon2[64], im1[64], im2[64], im3[64], param[256];
   int cost = -1;
   double sell_factor = -1.0;
   FILE *fp;

   D("parse tower from file: %s", path);

   // open the tower file
   fp = fopen(path, "r");
   if (fp == NULL) return;

   // parse it
   id[0] = name[0] = eng[0] = desc[0] = '\0';
   icon[0] = icon2[0] = im1[0] = im2[0] = im3[0] = '\0';
   while (fgets(line, sizeof(line), fp) != NULL)
   {
      // skip comment
      if (line[0] == '#') continue;

      // parse header
      if (!id[0] && (sscanf(line, "ID=%[^\n]", id) == 1)) {}
      else if(!name[0] && (sscanf(line, "Name=%[^\n]", name) == 1)) {}
      else if(!eng[0] && (sscanf(line, "Engine=%[^\n]", eng) == 1)) {}
      else if(!desc[0] && (sscanf(line, "Description=%[^\n]", desc) == 1)) {}
      else if(!icon[0] && (sscanf(line, "Icon=%[^\n]", icon) == 1)) {}
      else if(!im1[0] && (sscanf(line, "Image1=%[^\n]", im1) == 1)) {}
      else if(!im2[0] && (sscanf(line, "Image2=%[^\n]", im2) == 1)) {}
      else if(!im3[0] && (sscanf(line, "Image3=%[^\n]", im3) == 1)) {}
      else if((cost == -1) && (sscanf(line, "Cost=%d", &cost) == 1)) {}
      else if((sell_factor == -1.0) && (sscanf(line, "SellFactor=%lf", &sell_factor) == 1)) {}
      else if(sscanf(line, "PARAM=%[^(](%[^)]", param, icon2) == 2)
      {
         Ede_Tower_Class_Param *par;

         // new param, create it
         par = EDE_NEW(Ede_Tower_Class_Param);
         if (!par) return;
         par->name = eina_stringshare_add(param);
         par->icon = eina_stringshare_add(icon2);
         par->upgrades = NULL;
         params = eina_list_append(params, par);

         // parse upgrades info until empty line
         while (fgets(line, sizeof(line), fp) != NULL)
         {
            char _name[64];
            int _value, _bucks;

            if (line[0] == '\n') break;

            _name[0] = '\0'; _value = _bucks = 0;
            if (3 == sscanf(line, "%[^:]: value=%d bucks=%d",
                            _name, &_value, &_bucks))
            {
               Ede_Tower_Class_Param_Upgrade *up;

               up = EDE_NEW(Ede_Tower_Class_Param_Upgrade);
               if (!up) return;
               up->name = eina_stringshare_add(_name);
               up->value = _value;
               up->bucks = _bucks;
               par->upgrades = eina_list_append(par->upgrades, up);
            }
         }
      }
   }
   //close
   fclose(fp);

   // alloc the Ede_Tower_Class struct
   if (id[0] && name[0] && eng[0] && desc[0] && icon[0])
   {
      Ede_Tower_Class *tc;

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
      tc->params = params;
      tower_classes = eina_list_append(tower_classes, tc);
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

   // DEBUG  dump classes
   Eina_List *l1, *l2, *l3;
   Ede_Tower_Class *tc;
   Ede_Tower_Class_Param *par;
   Ede_Tower_Class_Param_Upgrade *up;
   EINA_LIST_FOREACH(tower_classes, l1, tc)
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
      EINA_LIST_FOREACH(tc->params, l2, par)
      {
         D("PARAM: '%s' ('%s')", par->name, par->icon);
         EINA_LIST_FOREACH(par->upgrades, l3, up)
            D(" + '%s' val: %d  bucks: %d", up->name, up->value, up->bucks);
      }
      D("********************");
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
      if (streql(tc->id, id))
         return tc;

   return NULL;
}

EAPI Ede_Tower *
ede_tower_selected_get(void)
{
   return selected_tower;
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
      ede_gui_tower_info_set(NULL);
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
      EDE_OBJECT_DEL(tower->obj);
      EDE_FREE(tower);
   }
   selected_tower = NULL;
}

EAPI void
ede_tower_upgrade(int button_num)
{
   Ede_Tower_Class_Param *param;
   Ede_Tower_Class_Param_Upgrade *next;
   Eina_List *l;
   
   if (!selected_tower) return;
   D("UPGRADE %d\n", button_num);

   param = eina_list_nth(selected_tower->class->params, button_num);
   D("%s", param->name);
   if (streql(param->name, "Damage"))
   {
      l = eina_list_data_find_list(param->upgrades, selected_tower->damage_up);
      next = eina_list_data_get(eina_list_next(l));
      selected_tower->damage = selected_tower->damage_up->value;
      selected_tower->damage_up = next;
   }
   else if (streql(param->name, "Reload"))
   {
      l = eina_list_data_find_list(param->upgrades, selected_tower->reload_up);
      next = eina_list_data_get(eina_list_next(l));
      selected_tower->reload = selected_tower->reload_up->value;
      selected_tower->reload_up = next;
   }
   else if (streql(param->name, "Range"))
   {
      l = eina_list_data_find_list(param->upgrades, selected_tower->range_up);
      next = eina_list_data_get(eina_list_next(l));
      selected_tower->range = selected_tower->range_up->value;
      selected_tower->range_up = next;
   }
   // Update selection
   _tower_select(selected_tower);
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
