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
#include <Ecore_File.h>

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
static Ede_Level *_current_level = NULL;
static Ede_Scenario *_current_scenario = NULL;

static int _player_lives;
static int _player_bucks;
static int _player_score;
static Eina_Bool _debug_panel_enable = EINA_FALSE;
static double _play_time;
static Ecore_Animator *_animator = NULL;

/**********   Menu Stuff   ****************************************************/
static void
_level_selected_cb(void *data)
{
   Ede_Level *level = data;
   D("PLAY LEVEL %s", level->name);
   ede_game_start(level);
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
   ede_game_start(_current_level);
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

/**********   Wave spawning stuff   ******************************************/

static Ede_Wave *_current_wave = NULL;
static int       _current_wave_num = 0;
static double    _next_wave_accumulator;
static double    _next_enemy_accumulator;

static void
_spawn_one_enemy(Ede_Wave *wave)
{
   int count;
   int start_row, start_col;
   Eina_List *points;
   //~ D(" ");

   wave->count--;

   // get number of cells that are starting point for this start_base
   points = _current_level->starts[wave->start_base];
   count = eina_list_count(points) / 2; // two elements for each point (row, col)

   // choose a random starting point from the list
   count = rand() % count;
   start_row = (int)eina_list_nth(points, count * 2);
   start_col = (int)eina_list_nth(points, count * 2 + 1);

   // spaw the new enemy
   ede_enemy_spawn(wave->type, wave->speed, wave->energy, wave->bucks,
                   start_row, start_col,
                   _current_level->home_row, _current_level->home_col);
}

static void
_next_wave(void)
{
   D("NEXT WAVE");
   _current_wave = eina_list_nth(waves, _current_wave_num);
   if (!_current_wave) return;

   _current_wave->count = _current_wave->total;
   _next_wave_accumulator = _next_enemy_accumulator = 0.0;
   _current_wave_num++;
}

static void
_wave_step(double time)
{
   if (!_current_wave) return;

   // new wave to spawn?
   _next_enemy_accumulator += time;
   if (_next_enemy_accumulator >=  _current_wave->wait)
   {
      _next_wave();
      return;
   }

   // Enemy to spawn in the current wave?
   if (_current_wave->count > 0)
   {
      _next_wave_accumulator += time;
      if (_next_wave_accumulator >=  _current_wave->delay)
      {
         _next_wave_accumulator = 0.0;
         _spawn_one_enemy(_current_wave);
         return;
      }
   }
}

/**********   Main Game Animator Loop   **************************************/
static int
_game_loop(void *data)
{
   static double last_time = 0;
   double elapsed, now;
   int num_enemies;

   // calc time between each frame
   now = ecore_loop_time_get();
   elapsed = last_time ? now - last_time : 0.0;
   last_time = now;

   if (_game_state >= GAME_STATE_PLAYING)
   {
      // keep track of play time
      _play_time += elapsed;
      // spawn wave/enemy as required
      _wave_step(elapsed);
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
      if (!_current_wave && num_enemies < 1)
      {
         ede_gui_menu_show("Victory !!");
         ede_gui_menu_item_add("Main Menu", "", _mainmenu_cb, NULL);
         ede_gui_menu_item_add("Level selector", "", _scenario_selected_cb, _current_scenario);
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

   return EINA_TRUE;
}

EAPI void
ede_game_start(Ede_Level *level)
{
   int row = 0, col = 0;

   D(" ");


   ede_level_load_data(level);

   _current_level = level;

   ede_gui_level_selector_hide();
   ede_gui_menu_hide();

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

   // spawn the first wave (others will be spawned from the _wave_step() at the right time)
   _current_wave_num = 0;
   _next_wave();

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
   ede_gui_level_clear();
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
   eina_strbuf_append_printf(t, "waves %d [current %d]<br>",
                     eina_list_count(waves), _current_wave_num);
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

