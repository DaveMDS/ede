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
static Ede_Scenario *_current_scenario = NULL;

static int _player_lives, _player_bucks, _player_score;
static Eina_Bool _debug_panel_enable = EINA_FALSE;
static double _play_time;
static Ecore_Animator *_animator = NULL;

/**********   Menu Stuff   ****************************************************/
static void
_level_selected_cb(void *data)
{
   Ede_Level *level = data;
   D("PLAY LEVEL %s", level->name);
   ede_level_load_data(level);
   ede_game_start();
}

static void
_level_selector_populate(Ede_Scenario *sce)
{
   Eina_List *l;
   Ede_Level *level;

   D("POPULATE %s", sce->name);
   _current_scenario = sce;
   ede_game_reset();

   // add all the scenarios to the mainmenu
   EINA_LIST_FOREACH(sce->levels, l, level)
      ede_gui_level_selector_item_add(level->name, _level_selected_cb, level);

   _game_state = GAME_STATE_LEVELSELECTOR;
}

static void
_scenario_selected_cb(void *data)
{
   Ede_Scenario *sce = data;

   // show the scenario menu
   ede_gui_menu_hide();
   _level_selector_populate(sce);
   ede_gui_level_selector_show();
}

static void
_restart_level_cb(void *data)
{
   D(" ");
   ede_game_reset();
   ede_game_start();
}

static void
_mainmenu_cb(void *data)
{
   D(" ");
   ede_game_reset();
   ede_gui_menu_hide();
   _game_state = GAME_STATE_LEVELSELECTOR;
   ede_game_mainmenu_populate();
}

static void
_exit_game_cb(void *data)
{
   ede_game_quit();
}

static void
_continue_game_cb(void *data)
{
   ede_gui_menu_hide();
   _game_state = GAME_STATE_PLAYING;
}

EAPI void
ede_game_mainmenu_populate(void)
{
   Eina_List *l;
   Ede_Scenario *sce;

   if (_game_state == GAME_STATE_MAINMENU) return;
   if (_game_state == GAME_STATE_LEVELSELECTOR)
      ede_gui_level_selector_hide();

   // show the menu
   ede_gui_menu_show("Main Menu");

   if (_game_state >= GAME_STATE_PAUSE)
   {
      ede_gui_menu_item_add("Continue current game", "", _continue_game_cb, NULL);
      ede_gui_menu_item_add("Restart level", "", _restart_level_cb, NULL);
      ede_gui_menu_item_add("Exit to main menu", "", _mainmenu_cb, NULL);
   }
   else
   {
      // add all the scenarios to the mainmenu
      EINA_LIST_FOREACH(ede_level_scenario_list_get(), l, sce)
         ede_gui_menu_item_add(sce->name, sce->desc, _scenario_selected_cb, sce);
   }

   // add the 'exit' item
   ede_gui_menu_item_add("Exit game", "", _exit_game_cb, NULL);

   _game_state = GAME_STATE_MAINMENU;
}

/**********   Main Game Animator Loop   **************************************/
static Eina_Bool
_game_loop(void *data)
{
   static double last_time = 0;
   double elapsed, now;
   int num_enemies;
   int remaining_waves;

   // calc time between each frame
   now = ecore_loop_time_get();
   elapsed = last_time ? now - last_time : 0.0;
   last_time = now;

   if (_game_state >= GAME_STATE_PLAYING)
   {
      // keep track of play time
      _play_time += elapsed;

      // spawn wave/enemy as required
      remaining_waves = ede_wave_step(elapsed);
      // recalc every enemys
      num_enemies = ede_enemy_one_step_all(elapsed);
      // recalc every towers
      ede_tower_one_step_all(elapsed);
      // recalc every bullets
      ede_bullet_one_step_all(elapsed);

      // no more lives ? LOOSER !!
      if (_player_lives < 0)
      {
         ede_gui_menu_show("Looser !!");
         ede_gui_menu_item_add("Retry Level", "", _restart_level_cb, NULL);
         ede_gui_menu_item_add("Main Menu", "", _mainmenu_cb, NULL);
         ede_gui_menu_item_add("Level Selector", "", _scenario_selected_cb, _current_scenario);

         _game_state = GAME_STATE_PAUSE;
      }

      // no more enemies ? WINNER !!
      if (remaining_waves < 1 && num_enemies < 1)
      {
         ede_gui_menu_show("Victory !!");
         ede_gui_menu_item_add("Main Menu", "", _mainmenu_cb, NULL);
         ede_gui_menu_item_add("Level selector", "", _scenario_selected_cb,
                                                     _current_scenario);
         ede_gui_menu_item_add("Retry Level", "", _restart_level_cb, NULL);

         _game_state = GAME_STATE_PAUSE;
      }
   }

   // update debug panel (if visible)
   ede_game_debug_panel_update(now);

   return ECORE_CALLBACK_RENEW;
}


/* Externally accessible functions */

/**
 * TODO
 */
EAPI Eina_Bool
ede_game_init(void)
{
   D(" ");

   // set debug level in the pathfinder
   ede_pathfinder_info_set(EINA_FALSE, EINA_FALSE);

   //show the main menu
   ede_game_mainmenu_populate();

   return EINA_TRUE;
}

/**
 * TODO
 */
EAPI Eina_Bool
ede_game_shutdown(void)
{
   D(" ");
   if (_animator) ecore_animator_del(_animator);
   return EINA_TRUE;
}

EAPI void
ede_game_start(void)
{
   Ede_Level *level;
   int row = 0, col = 0;

   D(" ");

   level = ede_level_current_get();

   ede_gui_level_selector_hide();
   ede_gui_menu_hide();

   ede_gui_level_init(level->rows, level->cols, level->towers);

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
         switch (cells[row][col])
         {
            case CELL_WALL:
               ede_gui_cell_overlay_add(OVERLAY_IMAGE_WALL, row, col);
               break;
            case CELL_START0:case CELL_START1:case CELL_START2:case CELL_START3:
            case CELL_START4:case CELL_START5:case CELL_START6:case CELL_START7:
            case CELL_START8:case CELL_START9:
               ede_gui_cell_overlay_add(OVERLAY_COLOR_RED, row, col);
               ede_gui_cell_overlay_text_set(row, col, cells[row][col] - CELL_START0, 1);
               break;
            default:
               break;
         }
      }
   }
   ede_gui_cell_overlay_add(OVERLAY_COLOR_GREEN, level->home_row, level->home_col);

   _player_lives = level->lives;
   _player_bucks = level->bucks;
   _player_score = 0;
   _play_time = 0.0;

   ede_gui_lives_set(_player_lives);
   ede_gui_bucks_set(_player_lives);
   ede_gui_score_set(_player_score);
   ede_game_state_set(GAME_STATE_PLAYING);

   ede_wave_start();

   ecore_animator_frametime_set(1.0 / MAX_FPS);

   if (!_animator)
      _animator = ecore_animator_add(_game_loop, NULL);
}

EAPI void
ede_game_reset(void)
{
   D(" ");
   ede_enemy_reset();
   ede_tower_reset();
   ede_bullet_reset();
   ede_level_load_data(ede_level_current_get());
   ede_gui_level_clear();
   ede_gui_tower_button_box_clear();
}

EAPI void
ede_game_pause(void)
{
   if (_game_state >= GAME_STATE_PLAYING)
      _game_state = GAME_STATE_PAUSE;
   else if (_game_state == GAME_STATE_PAUSE)
      _game_state = GAME_STATE_PLAYING;
}

EAPI void
ede_game_quit(void)
{
   ecore_main_loop_quit();
}

EAPI void
ede_game_debug_hook(void)
{
   //~ ede_gui_level_selector_hide();
   //~ _mainmenu_populate();
}

EAPI void
ede_game_debug_panel_enable(Eina_Bool enable)
{
   _debug_panel_enable = enable;
}

EAPI void
ede_game_debug_panel_update(double now)
{
   Eina_Strbuf *t;
   char *ts;
   static int last_second = 0;
   static int fps_counter = 0;
   static int FPS = 0;

   if (!_debug_panel_enable) return;

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
   eina_strbuf_append_printf(t, "FPS %d  time %s<br>", FPS, ts);
   EDE_FREE(ts);

   eina_strbuf_append_printf(t, "lives %d<br>bucks %d<br>",
                             _player_lives, _player_bucks);
   eina_strbuf_append(t, "<br>");

   // info from other components
   ede_enemy_debug_info_fill(t);
   ede_tower_debug_info_fill(t);
   ede_bullet_debug_info_fill(t);
   ede_level_debug_info_fill(t);

   ede_gui_debug_text_set(eina_strbuf_string_get(t));
   eina_strbuf_free(t);

}

EAPI char *
ede_game_time_get(double now)
{
   char buf[16];
   int seconds, minutes, hours;

   seconds = (int)(_play_time);
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
   _player_lives--;
   if (_player_lives >= 0)
      ede_gui_lives_set(_player_lives);
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
   ede_gui_bucks_set(_player_bucks);
}

