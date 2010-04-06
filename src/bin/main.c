/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
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

#define LOCAL_DEBUG 0
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif

Ede *ede;


/******************************************************************************/
EAPI int **
ede_array_new(int rows, int cols)
{
   D("rows: %d cols: %d", rows, cols);

   int row;
   int *aptr;
   int **rptr;

   aptr = calloc(cols * rows, sizeof(int));
   rptr = malloc(rows * sizeof(int *));
   if (!aptr || !rptr)
   {
      CRITICAL("Failure to allocate mem for the array");
      EDE_FREE(aptr);
      EDE_FREE(rptr);
      return NULL;
   }
   // 'point' the rows pointers
   for (row = 0; row < rows; row++)
      rptr[row] = aptr + (row * cols);

   return rptr;
}

EAPI void
ede_array_free(int **array)
{
   D(" ");
   if (!array || !*array)
      return;

   EDE_FREE(*array);
   EDE_FREE(array);
}
/******************************************************************************/



/******************************************************************************/

   

static int
_fake_end(void *data)
{
   ede_gui_level_clear();
   ede_level_free(data);
   return ECORE_CALLBACK_CANCEL;
}


static int
_fake_start(void *data)
{
      
// overlay test
   //~ ede_gui_cell_overlay_add(OVERLAY_BORDER_BLUE, 4, 4);
   //~ ede_gui_cell_overlay_add(OVERLAY_IMAGE_WALL, 4, 5);
   //~ ede_gui_cell_overlay_add(OVERLAY_7, 7, 4);
   //~ ede_gui_cell_overlay_add(OVERLAY_NONE, 4, 4);
   
// enenmy tests
   //~ ede_enemy_spawn("standard");
   //~ ede_enemy_spawn("standard");
   //~ ede_enemy_spawn("standard");

// test level shutdown
   //~ ecore_timer_add(4.0, _fake_end, level);
   
   return ECORE_CALLBACK_CANCEL;
}

/******************************************************************************/
int
main(int argc, char **argv)
{
   char buf[PATH_MAX];
   eina_init();
   ecore_init();

   // alloc ede main structure
   ede = EDE_NEW(Ede);
   if (!ede) exit(1);

   // init log domain
   //~ eina_log_level_set(EINA_LOG_LEVEL_DBG);
   ede->log_domain = eina_log_domain_register("ede", EINA_COLOR_GREEN);
   if (ede->log_domain < 0)
   {
      EINA_LOG_CRIT("could not create log domain 'ede'.");
      exit(1);
   }
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
   ede_pathfinder_init();
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
   ede_enemy_shutdown();
   ede_tower_shutdown();
   ede_pathfinder_shutdown();
   ede_gui_shutdown();
   ecore_shutdown();

   // shutdown eina
   eina_log_domain_unregister(ede->log_domain);
   ede->log_domain = -1;
   eina_shutdown();

   // free ede
   EDE_FREE(ede);

   return 0;
}

