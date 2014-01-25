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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <math.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_File.h>

//~ #include "gettext.h"
#include "ede.h"
#include "ede_gui.h"
#include "ede_level.h"
#include "ede_enemy.h"
#include "ede_astar.h"
#include "ede_game.h"
#include "ede_tower.h"
#include "ede_bullet.h"

#define LOCAL_DEBUG 0
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif

int ede_log_domain;


int
main(int argc, char **argv)
{
   char buf[PATH_MAX];
   eina_init();
   ecore_init();

   // init log domain
   // eina_log_level_set(EINA_LOG_LEVEL_DBG);
   ede_log_domain = eina_log_domain_register("ede", EINA_COLOR_GREEN);
   if (ede_log_domain < 0)
   {
      EINA_LOG_CRIT("could not create log domain 'ede'.");
      exit(1);
   }
   eina_log_domain_level_set("ede", EINA_LOG_LEVEL_DBG);
   INF("Welcome to EDE :)");
   INF("Eina host a population of %d hamsters.", eina_hamster_count());

   // create user folders
   snprintf(buf, sizeof(buf), "%s/.config/ede/themes", getenv("HOME"));
   if (!ecore_file_is_dir(buf)) ecore_file_mkpath(buf);
   snprintf(buf, sizeof(buf), "%s/.config/ede/levels", getenv("HOME"));
   if (!ecore_file_is_dir(buf)) ecore_file_mkpath(buf);
   if (!ecore_file_exists(buf))
      ERR("Failed to create user folder.");

   // init gui
   if (!ede_gui_init())
   {
      ERR("Failed to init gui. Exiting...");
      goto shutdown;
   }
   ede_level_init();
   ede_pathfinder_init();
   ede_bullet_init();
   ede_enemy_init();
   ede_tower_init();
   ede_game_init();

   //DBG
   //~ ecore_timer_add(0.4, _fake_start, NULL);

   // start the main loop
   ecore_main_loop_begin();

   // shutdown
shutdown:
   ede_game_shutdown();
   ede_tower_shutdown();
   ede_enemy_shutdown();
   ede_bullet_shutdown();
   ede_pathfinder_shutdown();
   ede_level_shutdown();
   ede_gui_shutdown();
   ecore_shutdown();

   // shutdown eina
   eina_log_domain_unregister(ede_log_domain);
   ede_log_domain = -1;
   eina_shutdown();

   return 0;
}

