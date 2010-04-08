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
#include <Ecore.h>

#include "ede.h"
#include "ede_game.h"
#include "ede_level.h"
#include "ede_gui.h"
#include "ede_astar.h"
#include "ede_enemy.h"
#include "ede_tower.h"
#include "ede_bullet.h"

#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif


#define MAX_FPS 30

/* Local subsystem vars */
static Ede_Game_State _game_state;
static Ede_Level *current_level = NULL;
static int current_wave_num = 0;
static int _player_lives;
static int _player_bucks;
static Eina_Bool _debug_panel_enable = EINA_FALSE;
static double _start_time;

/* Local subsystem functions */
static int
_game_loop(void *data)
{
   static double last_time = 0;
   double elapsed, now;

   // calc time between each frame
   now = ecore_loop_time_get();
   elapsed = now - last_time;
   last_time = now;
   
   // recalc enemys
   ede_enemy_one_step_all(elapsed);
   // recalc towers
   ede_tower_one_step_all(elapsed);
   // recalc bullets
   ede_bullet_one_step_all(elapsed);

   // update debug panel
   if (_debug_panel_enable)
   {
      Eina_Strbuf *t;
      char buf[1024];
      char *ts;
      static int last_second = 0;
      static int fps_counter = 0;
      static int FPS = 0;
      
      // calc FPS
      fps_counter++;
      if ((int)now > last_second)
      {
         //~ D("last second %d %d", last_second, fps_counter);
         last_second = now;
         FPS = fps_counter;
         fps_counter = 0;
      }

      t = eina_strbuf_new();

      // game info
      eina_strbuf_append(t, "<h3>game:</h3><br>");
      ts = ede_game_time_get(now);
      snprintf(buf, sizeof(buf), "FPS %d  time %s<br>", FPS,ts);
      EDE_FREE(ts);
      eina_strbuf_append(t, buf);
      snprintf(buf, sizeof(buf), "waves %d  lives %d<br>",
               current_wave_num, _player_lives);
      eina_strbuf_append(t, buf);
      snprintf(buf, sizeof(buf), "bucks %d<br>", _player_bucks);
      eina_strbuf_append(t, buf);
      eina_strbuf_append(t, "<br>");

      // info from other components
      ede_enemy_debug_info_fill(t);
      ede_tower_debug_info_fill(t);
      ede_bullet_debug_info_fill(t);

      ede_gui_debug_text_set(eina_strbuf_string_get(t));
      eina_strbuf_free(t);
   }
   
   return ECORE_CALLBACK_RENEW;
}

static int
_delayed_spawn(void *data)
{
   Ede_Wave *wave = data;
   int count;
   int start_row, start_col;
   Eina_List *points;
   //~ D(" ");

   wave->count--;

   // get number of cells that are starting point for this start_base
   points = current_level->starts[wave->start_base];
   count = eina_list_count(points) / 2; // two elements for each point (row, col)
   
   // choose a random starting point from the list
   count = rand() % count;
   start_row = (int)eina_list_nth(points, count * 2);
   start_col = (int)eina_list_nth(points, count * 2 + 1);

   // spaw the new enemy
   ede_enemy_spawn(wave->type, wave->speed, wave->energy, wave->bucks,
                   start_row, start_col,
                   current_level->home_row, current_level->home_col);

   if (wave->count <= 0 )
      return ECORE_CALLBACK_CANCEL;
   else
      return ECORE_CALLBACK_RENEW;
}

static int
_next_wave(void *unused)
{
   Ede_Wave *wave;

   wave = eina_list_nth(current_level->waves, current_wave_num);
   if (!wave)
   {
      D("   !!! YOU WON !!!");
      return ECORE_CALLBACK_CANCEL;
   }

   // spawn 2 enemy per second ... hmmmm not good
   wave->count = wave->total;
   ecore_timer_add(0.5, _delayed_spawn, wave); // TODO Also clear this timer

   // wait, and then spawn the next wave
   current_wave_num++;
   ecore_timer_add(wave->wait, _next_wave, NULL); //TODO need to clear this timer when exit, abort, ... pause???

   return ECORE_CALLBACK_CANCEL;
}

static void
_game_start(Ede_Level *level)
{
   int row = 0, col = 0;

   D(" ");

   ede_gui_level_init(level->rows, level->cols);
  

   // populate the checkboard with walls, start points and home
   for (row = 0; row < level->rows; row++)
   {
      for (col = 0; col < level->cols; col++)
      {
         // DBG col/row numbers
         if (row == 0)
         {
            ede_gui_cell_overlay_add(OVERLAY_BORDER_RED, row, col);
            ede_gui_cell_overlay_text_set(row, col, col, 1);
         }
         if (col == 0)
         {
            ede_gui_cell_overlay_add(OVERLAY_BORDER_RED, row, col);
            ede_gui_cell_overlay_text_set(row, col, row, 2);
         }
         // DBG

         //~ printf("%d\n", level->cells[row][col]);
         switch (level->cells[row][col])
         {
            case CELL_WALL:
               ede_gui_cell_overlay_add(OVERLAY_IMAGE_WALL, row, col);
               break;
            case CELL_START0:case CELL_START1:case CELL_START2:case CELL_START3:
            case CELL_START4:case CELL_START5:case CELL_START6:case CELL_START7:
            case CELL_START8:case CELL_START9:
               ede_gui_cell_overlay_add(OVERLAY_COLOR_RED, row, col);
               ede_gui_cell_overlay_text_set(row, col, level->cells[row][col] - CELL_START0, 1);
               break;
            default:
               break;
         }
      }
   }
   ede_gui_cell_overlay_add(OVERLAY_COLOR_GREEN, level->home_row, level->home_col);


   _start_time = ecore_loop_time_get();
   _player_lives = level->lives;
   _player_bucks = level->bucks;

   ede_game_state_set(GAME_STATE_PLAYING);
   
   // spawn the first wave (that will spawn the others, in chain)
   _next_wave(NULL);

   ecore_animator_frametime_set(1.0 / MAX_FPS);
   ecore_animator_add(_game_loop, NULL);
}

/* Local subsystem callbacks */


/* Externally accessible functions */

/**
 * TODO
 */
EAPI Eina_Bool
ede_game_init(void)
{
   D(" ");

   current_level = ede_level_load_header("asd.txt");
   
   ede_level_load_data(current_level);
   current_wave_num = 0;

   // set debug level in the pathfinder
   ede_pathfinder_info_set(EINA_FALSE, EINA_FALSE);

   _game_start(current_level);


   return EINA_TRUE;
}

/**
 * TODO
 */
EAPI Eina_Bool
ede_game_shutdown(void)
{
   D(" ");

   return EINA_TRUE;
}

/**
 * TODO
 */

EAPI void
ede_game_debug_hook(void)
{
   //~ _next_wave();
   ede_tower_destroy_selected();
   
   
}

EAPI void
ede_game_debug_panel_enable(Eina_Bool enable)
{
   _debug_panel_enable = enable;
}

EAPI char *
ede_game_time_get(double now)
{
   char buf[16];
   int seconds, minutes, hours;

   seconds = (int)(now - _start_time);
   minutes = seconds / 60;
   hours = minutes / 60;

   if (hours > 0)
      snprintf(buf, sizeof(buf), "%dh %dm %ds", hours, minutes % 60, seconds % 60);
   else if (minutes > 0)
      snprintf(buf, sizeof(buf), "%dm %ds", minutes % 60, seconds % 60);
   else
      snprintf(buf, sizeof(buf), "%ds", seconds);

   return strdup(buf);
}

EAPI void
ede_game_state_set(Ede_Game_State state)
{
   _game_state = state;
}

EAPI Ede_Game_State
ede_game_state_get(void)
{
   return _game_state;
}

EAPI void
ede_game_home_violated(void)
{
   if (_player_lives-- <= 0)
      D("YOU LOOSE");
}

EAPI int
ede_game_bucks_get(void)
{
   return _player_bucks;
}

EAPI void
ede_game_bucks_gain(int bucks)
{
   _player_bucks += bucks;
}

