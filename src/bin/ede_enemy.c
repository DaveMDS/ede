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

#include <math.h>
#include <stdio.h>
#include <Eina.h>
#include <Ecore.h>


#include "ede.h"
#include "ede_enemy.h"
#include "ede_level.h"
#include "ede_gui.h"
#include "ede_astar.h"
#include "ede_utils.h"
#include "ede_game.h"

#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif


#define GAUGE_W 20
#define GAUGE_H 4


/* Local subsystem vars */
static Eina_List *deads = NULL;
static Eina_List *alives = NULL;
static int _count_spawned = 0;
static int _count_killed = 0;

/* Local subsystem callbacks */
static void
_enemy_del(Ede_Enemy *e)
{
   EDE_OBJECT_DEL(e->obj);
   EDE_OBJECT_DEL(e->o_gauge1);
   EDE_OBJECT_DEL(e->o_gauge2);
   if (e->path)
   {
      eina_list_free(e->path);
      e->path = NULL;
   }
   EDE_FREE(e);
}

static void
_path_recalc(Ede_Enemy *e)
{
   int row, col;
   Ede_Level *level;

   // get current enemy cell
   ede_gui_cell_get_at_coords(e->x, e->y, &row, &col);

   //~ D("hops: %d [pos %d %d][goto %d %d]", eina_list_count(e->path), row, col, e->target_row, e->target_col);

   // free old path
   eina_list_free(e->path);

   // run the pathfinder
   level = ede_level_current_get();
   e->path = ede_pathfinder(level->rows, level->cols,
                  row, col, e->target_row, e->target_col,
                  ede_level_walkable_get, 0, EINA_FALSE);

   // TODO the travel from the current position (in pixel) from the first
   //      hop is not really defined, but it seems quiet good.
   //~ D("NEW PATH %d", eina_list_count(e->path));
}

static void
_gauge_recalc(Ede_Enemy *e)
{
   int x, y;
   double val;

   x = (int)(e->x + 0.5) - e->w / 2;
   y = (int)(e->y + 0.5) + e->h / 2;
   val = (double)e->energy / (double)e->strength;
   evas_object_move(e->o_gauge1, x, y);
   evas_object_move(e->o_gauge2, x, y);
   evas_object_resize(e->o_gauge2, val * GAUGE_W, GAUGE_H);
}

static void
_standard_enemy_step(Ede_Enemy *e, double time)
{
   int row, col;
   int dx, dy;

   // if we don't have a destination (local movement inside a path), get a new
   // dest from the path list
   if (!e->dest_x)
   {
      // if the path list is empty then the target is reached !
      if (eina_list_count(e->path) < 2)
      {
         ede_game_home_violated();
         ede_enemy_kill(e);
         return;
      }

      // pop 2 elements (row & col of the next path hop) from the path list
      row = (int)(long)EINA_LIST_POP(e->path);
      col = (int)(long)EINA_LIST_POP(e->path);

      // get destination center point in pixel
      ede_gui_cell_coords_get(row, col, &e->dest_x, &e->dest_y, EINA_TRUE);
      //~ D("New destination: row:%d col:%d (%d,&d)", row, col, e->dest_x, e->dest_y);

      // calc direction angle
      // NOTE: enemy will follow a really simple path, so we can use this stupid
      // but really fast approach
      dx = e->dest_x - e->x;
      dy = e->dest_y - e->y;
      if (dx > 0)
      {
         if (dy < 0) e->angle = 45;       // top-right
         else if (dy == 0) e->angle = 90; // right
         else e->angle = 135;             // bottom-right
      }
      else if (dx < 0)
      {
         if (dy < 0) e->angle = 315;       // top-left
         else if (dy == 0) e->angle = 270; // left
         else e->angle = 225;              // bottom-left
      }
      else
      {
         if (dy > 0) e->angle = 180;        // bottom
         else e->angle = 0;                 // top
      }
      //~ D("angle: %d [dx: %d dy: %d]", e->angle, dx, dy);
   }


   // calc the new position (...another stupid but really fast method)
   switch (e->angle)
   {
      case 0: // going up
         e->y -= time * e->speed * 1.41;
         break;
      case 45: // going up-right
         e->x += time * e->speed;
         e->y -= time * e->speed;
         break;
      case 90: // going right
         e->x += time * e->speed * 1.41;
         break;
      case 135: // going down-right
         e->x += time * e->speed;
         e->y += time * e->speed;
         break;
      case 180: // going down
         e->y += time * e->speed * 1.41;
         break;
      case 225: // going down-left
         e->x -= time * e->speed;
         e->y += time * e->speed;
         break;
      case 270: // going left
         e->x -= time * e->speed * 1.41;
         break;
      case 315: // going up-left
         e->x -= time * e->speed;
         e->y -= time * e->speed;
         break;
      default:
         break;
   }

   // hop reached ?
   #define MOVE_TO_TARGET() { e->x = e->dest_x; e->y = e->dest_y; e->dest_x = 0; }
   if ((e->angle == 45 || e->angle == 90 || e->angle == 135) && (e->x >= e->dest_x))
      MOVE_TO_TARGET()
   else if ((e->angle == 225 || e->angle == 270 || e->angle == 315) && (e->x <= e->dest_x))
      MOVE_TO_TARGET()
   else if (e->angle == 0 && e->y <= e->dest_y)
      MOVE_TO_TARGET()
   else if (e->angle == 180 && e->y >= e->dest_y)
      MOVE_TO_TARGET()

   // apply the new position
   //~ D("%f %f",e->position.x, e->position.y);
   dx = (int)(e->x + 0.5) - e->w / 2;
   dy = (int)(e->y + 0.5) - e->h / 2;
   evas_object_move(e->obj, dx, dy);

   // TODO need to optimize rotation...or made prerotated edje version :(
   ede_util_obj_rotate(e->obj, e->angle);// TODO really I need to rotate everytime ???
}

static void
_flyer_enemy_step(Ede_Enemy *e, double time)
{
   int row, col;
   float distance;

   if (!e->dest_x)
   {
      // if the path list is empty then the target is reached !
      if (eina_list_count(e->path) < 2)
      {
         ede_game_home_violated();
         ede_enemy_kill(e);
         return;
      }

      // pop 2 elements (row & col of the next path hop) from the path list
      row = (int)(long)EINA_LIST_POP(e->path);
      col = (int)(long)EINA_LIST_POP(e->path);

      // get destination center point in pixel
      ede_gui_cell_coords_get(row, col, &e->dest_x, &e->dest_y, EINA_TRUE);
      //~ D("New destination: row:%d col:%d (%d,%d)", row, col, e->dest_x, e->dest_y);

      // calc direction angle
      e->angle = ede_util_angle_calc(e->x, e->y, e->dest_x, e->dest_y);
   }

   // calc distance from target
   distance = ede_util_distance_calc(e->dest_x, e->dest_y, e->x, e->y);

   // destination reached
   if (distance < 10)
   {
      e->x = e->dest_x; e->y = e->dest_y; e->dest_x = 0;
   }
   else
   {
      // calc new position
      e->x += (e->dest_x - e->x) / distance * time * e->speed; // TODO a simpler way ??
      e->y += (e->dest_y - e->y) / distance * time * e->speed;

      evas_object_move(e->obj, e->x - e->w / 2 + 0.5, e->y - e->h / 2 + 0.5);
      // TODO need to optimize rotation...or made prerotated edje version :(
      ede_util_obj_rotate(e->obj, e->angle);// TODO really I need to rotate everytime ???
   }
}

/* Externally accessible functions */
EAPI Eina_Bool
ede_enemy_init(void)
{

   return EINA_TRUE;
}

EAPI Eina_Bool
ede_enemy_shutdown(void)
{
   Ede_Enemy *e;

   D(" ");
   EINA_LIST_FREE(deads, e)
      _enemy_del(e);
   EINA_LIST_FREE(alives, e)
      _enemy_del(e);
   return EINA_TRUE;
}


EAPI void //TODO rename end_* in target_*
ede_enemy_spawn(const char *type, int speed, int strength, int bucks,
                int start_row, int start_col, int end_row, int end_col)
{
   Ede_Level *level;
   Ede_Enemy *e;
   char buf[PATH_MAX];

   //~ D("alives %d  deads %d", eina_list_count(alives), eina_list_count(deads));

   // get an enemy from the deads list
   e = EINA_LIST_POP(deads);
   if (!e)
   {
      // or create a new one
      e = EDE_NEW(Ede_Enemy);
      if (!e) return;

      // main image
      e->obj = evas_object_image_filled_add(ede_gui_canvas_get());

      // create the 2 rects used for the energy meter
      e->o_gauge1 = evas_object_rectangle_add(ede_gui_canvas_get());
      evas_object_pass_events_set(e->o_gauge1, EINA_TRUE);
      evas_object_color_set(e->o_gauge1, 200, 0, 0, 255);
      evas_object_resize(e->o_gauge1, GAUGE_W, GAUGE_H);

      e->o_gauge2 = evas_object_rectangle_add(ede_gui_canvas_get());
      evas_object_pass_events_set(e->o_gauge2, EINA_TRUE);
      evas_object_color_set(e->o_gauge2, 0, 200, 0, 255);
   }

   // load correct image (evas cache this)
   snprintf(buf, sizeof(buf), PACKAGE_DATA_DIR"/themes/enemy_%s.png", type);
   evas_object_image_file_set(e->obj, buf, NULL);
   evas_object_image_size_get(e->obj, &e->w, &e->h);
   evas_object_resize(e->obj, e->w, e->h);

   //~ D("SPAW ENEMEY ID: %d", e->id);

   // set the starting position
   int xi, yi;
   ede_gui_cell_coords_get(start_row, start_col, &xi, &yi, EINA_TRUE);
   e->x = xi;
   e->y = yi;
   e->target_row = end_row;
   e->target_col = end_col;
   e->killed = EINA_FALSE;
   e->born_count++;

   // reset the local destination
   e->dest_x = e->dest_y = 0;
   e->speed = speed;
   e->bucks = bucks;
   e->energy = e->strength = strength;

   // put the enemy in the alives list
   EINA_LIST_PUSH(alives, e);

   if (streql(type, "flyer"))
   {
      e->step_func = _flyer_enemy_step;
      // go directly to the target, ignoring walls
      EINA_LIST_PUSH(e->path, (void*)e->target_col);
      EINA_LIST_PUSH(e->path, (void*)e->target_row);
      evas_object_layer_set(e->obj, LAYER_FLYER);
      evas_object_layer_set(e->o_gauge1, LAYER_FLYER);
      evas_object_layer_set(e->o_gauge2, LAYER_FLYER);
   }
   else
   {
      e->step_func = _standard_enemy_step;
      // calc the route using the A* pathfinder
      level = ede_level_current_get();
      e->path = ede_pathfinder(level->rows, level->cols,
                               start_row, start_col, end_row, end_col,
                               ede_level_walkable_get, 0, EINA_FALSE);
      evas_object_layer_set(e->obj, LAYER_WALKER);
      evas_object_layer_set(e->o_gauge1, LAYER_WALKER);
      evas_object_layer_set(e->o_gauge2, LAYER_WALKER);
   }

   // calc the initial position/rotation and show the enemy
   e->step_func(e, 0.0);
   evas_object_show(e->obj);
   evas_object_show(e->o_gauge1);
   evas_object_show(e->o_gauge2);

   // global counter
   _count_spawned++;
}

EAPI void
ede_enemy_kill(Ede_Enemy *e)
{
   //~ D("alives %d  deads %d", eina_list_count(alives), eina_list_count(deads));
   if (e->killed) return;
   else e->killed = EINA_TRUE;

   _count_killed++;

   if (e->path)
   {
      eina_list_free(e->path);
      e->path = NULL;
   }
   alives = eina_list_remove(alives, e);
   EINA_LIST_PUSH(deads, e);
   evas_object_hide(e->obj);
   evas_object_hide(e->o_gauge1);
   evas_object_hide(e->o_gauge2);
}

EAPI void
ede_enemy_reset(void)
{
   Ede_Enemy *e;

   while (alives)
   {
      e = EINA_LIST_POP(alives);
      e->killed = EINA_TRUE;
      evas_object_hide(e->obj);
      evas_object_hide(e->o_gauge1);
      evas_object_hide(e->o_gauge2);

      EINA_LIST_PUSH(deads, e);
   }
   _count_spawned = _count_killed = 0;
}

EAPI void
ede_enemy_hit(Ede_Enemy *e, int damage)
{
   //~ D("DAMAGE %d [%d]", e->energy, e->strength);

   e->energy -= damage;
   if (e->energy <= 0)
   {
      ede_game_bucks_gain(e->bucks);
      ede_enemy_kill(e);
   }
}

EAPI Ede_Enemy *
ede_enemy_nearest_get(int x, int y, int *angle, int *distance)
{
   Ede_Enemy *e, *nearest = NULL;
   Eina_List *l;
   int min_d = 999999;

   //~ D("NEAREST OF %d %d", x, y);
   EINA_LIST_FOREACH(alives, l, e)
   {
      int dx, dy, d;

      //~ if (e->id != 1) continue; //dbg

      // calc distance
      dx = x - e->x;
      dy = y - e->y;
      d = sqrt(dx*dx + dy*dy); // SLOWWWWW TODO simplify
      if (d < min_d)
      {
         min_d = d;
         nearest = e;
      }
      //~ D("  buddy: %.2f %.2f [dx %d dy %d] %d", e->x, e->y, dx, dy, d);
   }
   //~ if (nearest)
      //~ D("NEAREST: id %d, distance: %d", nearest->id, min_d);

   if (angle && nearest)
   {
      *angle = ede_util_angle_calc(x, y, nearest->x, nearest->y);
   }
   if (distance)
      *distance = min_d;

   return nearest;
}

EAPI int
ede_enemy_one_step_all(double time)
{
   Ede_Enemy *e;
   Eina_List *l, *ll;

   // calc every alive enemy
   EINA_LIST_FOREACH_SAFE(alives, l, ll, e)
   {
      e->step_func(e, time);
      _gauge_recalc(e);
   }

   return eina_list_count(alives);
}

EAPI void
ede_enemy_path_recalc_all(void)
{
   Eina_List *l;
   Ede_Enemy *e;

   //TODO maybe just mark the path as invalidd and recald on next loop ??
   D(" ");
   EINA_LIST_FOREACH(alives, l, e)
      _path_recalc(e);
}

EAPI void
ede_enemy_debug_info_fill(Eina_Strbuf *t)
{
   eina_strbuf_append(t, "<h3>enemies:</h3><br>");
   eina_strbuf_append_printf(t, "on %.3d  off %.3d [max: %d]<br>",
                             eina_list_count(alives), eina_list_count(deads),
                             eina_list_count(alives) + eina_list_count(deads));
   eina_strbuf_append_printf(t, "spawned %d  killed %d<br>",
                             _count_spawned, _count_killed);
   eina_strbuf_append(t, "<br>");
}

/*
 * NOTHING BEHIND .. just some tests
 *
 *
EAPI void // TODO optimize!
ede_enemy_one_step_all_SIMPLE(void)
{
   Ede_Enemy *e;
   Eina_List *l;

   double elapsed = 0;
   static double last_time = 0;
   double now;
   now = ecore_loop_time_get();
   if (last_time) elapsed = now - last_time;
   last_time = now;


   //~ D("elapsed %f", elapsed);
   EINA_LIST_FOREACH(alives, l, e)
   {
      if (!e->destination.x)
      {
         int row, col;

         // pop 2 elements (row & col of the next hop) from the enemy path list
         if (eina_list_count(e->path) < 2)
         {
            D("KILL\n");
            continue;
         }
         row = (int)(long)e->path->data;
         col = (int)(long)e->path->next->data;
         e->path = eina_list_remove_list(e->path, e->path);
         e->path = eina_list_remove_list(e->path, e->path);


         // get destination center point in pixel
         int x, y;
         ede_gui_pos_to_center_coords(row, col, &x, &y);
         vector_set(&e->destination, x, y);
         e->move_time = now;

         D("AT2: %f,%f  GOTO: %f,%f", e->position.x, e->position.y,
                                      e->destination.x, e->destination.y);
      }

      //V' = Va + t.(Vb-Va)
      Vector v;
      v.x = e->position.x + (now - e->move_time) * (e->destination.x - e->position.x);
      v.y = e->position.y + (now - e->move_time) * (e->destination.y - e->position.y);


      // calc rotation angle
      if (e->velocity.x == 0)
      {
         if (e->velocity.y > 0) e->direction = 0;   // up
         if (e->velocity.y < 0) e->direction = 180; // down
      }
      else if (e->velocity.y == 0)
      {
         if (e->velocity.x > 0) e->direction = 90;  // right
         if (e->velocity.x < 0) e->direction = 270; // left
      }
      else if (e->velocity.x != 0 && e->velocity.y != 0)
      {
         e->direction = atan2(e->velocity.x, e->velocity.y);
         e->direction = (e->direction * (180 / 3.1415));
         D("[%f %f] angle: %.2f", e->velocity.x, e->velocity.y, e->direction);
      }



      if (abs((int)v.x - (int)e->destination.x) < 5)
      {
         e->position.x = e->destination.x;
         e->position.y = e->destination.y;
         e->destination.x = 0;
      }else
      {
         // move it
         //~ D("%f", v.x);
         //~ ede_gui_enemy_move(e, e->position.x + 0.5, e->position.y + 0.5);
         ede_gui_enemy_move(e, v.x + 0.5, v.y + 0.5);
      }
   }
}


EAPI void // TODO optimize!   // BOID
ede_enemy_one_step_all_BOID(void)
{
   Ede_Enemy *e;
   Eina_List *l;
   //~ double distance;

   double elapsed = 0;
   static double last_time = 0;
   double now;
   now = ecore_loop_time_get();
   if (last_time) elapsed = now - last_time;
   last_time = now;


   //~ D("elapsed %f", elapsed);
   EINA_LIST_FOREACH(alives, l, e)
   {
      if (!e->dest_x)
      {
         int row, col;

         // pop 2 elements (row & col of the next hop) from the enemy path list
         if (eina_list_count(e->path) < 2)
         {
            D("KILL\n");
            continue;
         }
         row = (int)(long)e->path->data;
         col = (int)(long)e->path->next->data;
         e->path = eina_list_remove_list(e->path, e->path);
         e->path = eina_list_remove_list(e->path, e->path);


         // get destination center point in pixel
         ede_gui_pos_to_center_coords(row, col, &e->dest_x, &e->dest_y);



         vector_set(&e->position, e->x, e->y);
         vector_set(&e->velocity, 0, 0);


         e->speed = 1;

         D("AT: %d,%d  GOTO: %d,%d ", e->x, e->y, e->dest_x, e->dest_y);
      }
      // rule1: goto dest
      Vector v;
      v.x = (e->dest_x - e->position.x) / 30;
      v.y = (e->dest_y - e->position.y) / 30;

      e->velocity = vector_add(e->velocity, v);
      e->position = vector_add(e->position, e->velocity);


      // last rule: max speed
      //~ float len;
      //~ len = vector_lenght(e->velocity);
      //~ if (len > e->speed)
      //~ {
         //~ e->velocity.x = (e->velocity.x / len) * e->speed;
         //~ e->velocity.y = (e->velocity.y / len) * e->speed;
      //~ }
//~
      //~ e->velocity = vector_add(e->velocity, v);
      //~ e->position = vector_add(e->position, e->velocity);


      // calc rotation angle
      if (e->velocity.x == 0)
      {
         if (e->velocity.y > 0) e->direction = 0;   // up
         if (e->velocity.y < 0) e->direction = 180; // down
      }
      else if (e->velocity.y == 0)
      {
         if (e->velocity.x > 0) e->direction = 90;  // right
         if (e->velocity.x < 0) e->direction = 270; // left
      }
      else if (e->velocity.x != 0 && e->velocity.y != 0)
      {
         e->direction = atan2(e->velocity.x, e->velocity.y);
         e->direction = (e->direction * (180 / 3.1415));
         D("[%f %f] angle: %.2f", e->velocity.x, e->velocity.y, e->direction);
      }

      // move it
      ede_gui_enemy_move(e, e->position.x + 0.5, e->position.y + 0.5);
      ede_gui_enemy_rotate(e, e->direction);

      // destination reached?
      //~ D("%d - %d (%f)", (int)e->position.x, e->dest_x, len);
      //~ if (e->position.x
      if ((int)e->position.x == e->dest_x)
      {
         e->x = e->dest_x;
         e->y = e->dest_y;
         e->dest_x = e->dest_y = 0;

      }
   }
}
   */
/*
EAPI void // TODO optimize!
ede_enemy_one_step_all_OK2(void)
{
   Ede_Enemy *e;
   Eina_List *l;
   double distance;

   double elapsed = 0;
   static double last_time = 0;
   double now;
   now = ecore_loop_time_get();
   if (last_time) elapsed = now - last_time;
   last_time = now;


   //~ D("elapsed %f", elapsed);
   EINA_LIST_FOREACH(alives, l, e)
   {
      if (!e->dest_x)
      {
         int row, col;

         // pop 2 elements (row & col of the next hop) from the enemy path list
         if (eina_list_count(e->path) < 2)
         {
            D("KILL\n");
            continue;
         }
         row = (int)(long)e->path->data;
         col = (int)(long)e->path->next->data;
         e->path = eina_list_remove_list(e->path, e->path);
         e->path = eina_list_remove_list(e->path, e->path);


         // get destination center point in pixel
         ede_gui_pos_to_center_coords(row, col, &e->dest_x, &e->dest_y);

         // get distance to target
         //~ int dx = e->dest_x - e->x;
         //~ int dy = e->dest_y - e->y;
         //~ distance = sqrt((dx * dx) + (dy * dy));

         e->speed = 3;

         e->v.x = (float)(e->dest_x - e->x)  / 60;
         e->v.y = (float)(e->dest_y - e->y)  / 60;
         e->direction = atanf(e->v.x);

         e->subx = e->x;
         e->suby = e->y;


         //~ D("pos: %d,%d (a: %f) vec[%f,%f] [to: %d,%d]",e->x, e->y, distance, e->v.x, e->v.y, e->dest_x, e->dest_y);
      }

      e->subx += e->v.x * e->speed;
      e->suby += e->v.y * e->speed;
      ede_gui_enemy_move(e, e->subx + 0.5, e->suby + 0.5);

      // destination reached
      if ((int)e->subx == e->dest_x && (int)e->suby == e->dest_y)
      {
         e->x = e->dest_x;
         e->y = e->dest_y;
         e->dest_x = e->dest_y = 0;
      }
   }
}


EAPI void // TODO optimize!
ede_enemy_one_step_all_OK(void)
{
   Ede_Enemy *e;
   Eina_List *l;
   static double last_time = 0;
   double elapsed = 0;
   double now;
   double distance = 0;


   now = ecore_loop_time_get();
   if (last_time) elapsed = now - last_time;
   last_time = now;


   //~ D("elapsed %f", elapsed);
   EINA_LIST_FOREACH(alives, l, e)
   {
      if (!e->dest_x)
      {
         int row, col;
         //~ int x, y;

         // pop 2 elements (row & col of the next hop) from the enemy path list
         if (eina_list_count(e->path) < 2)
         {
            D("KILL\n");
            continue;
         }
         row = (int)(long)e->path->data;
         col = (int)(long)e->path->next->data;
         e->path = eina_list_remove_list(e->path, e->path);
         e->path = eina_list_remove_list(e->path, e->path);

         D("e: %d [cur: %d %d - %f][goto: %d %d]", e->id, e->x, e->y, e->direction, row, col);
         ede_gui_pos_to_center_coords(row, col, &e->dest_x, &e->dest_y);

         e->subx = e->x;
         e->suby = e->y;
         e->speed = 50;

         // calc & normalize the vector
         e->v.x = (e->dest_x - e->x);
         e->v.y = (e->dest_y - e->y);
         distance = sqrt(e->v.x * e->v.x + e->v.y * e->v.y);
         e->v.x = e->v.x / distance;
         e->v.y = e->v.y / distance;
      }

      // calc new pos
      e->subx += e->v.x * ((elapsed) * e->speed);
      e->suby += e->v.y * ((elapsed) * e->speed);

      //~ if (distance < 5)
      D("DIS: %f", distance);
      if (e->x == e->dest_x && e->y == e->dest_y)
         e->dest_x = e->dest_y = 0;

      //~ ede_gui_enemy_redraw(e);
      ede_gui_enemy_move(e, e->subx + 0.5, e->suby + 0.5);
   }
}
*/
