/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#include <stdio.h>
#include <ctype.h>

#include <Eina.h>
#include <Ecore_File.h>

#include "ede.h"
#include "ede_level.h"
#include "ede_utils.h"

#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif

#define MAX_LEVEL_COLS 1024 /* this is just the lenght of the buffer used to
                               read the lines from the level file
                               Can be bigger*/


/* Local subsystem vars */
static Eina_List *scenarios = NULL;
static Ede_Level *current_level = NULL;

/* Local subsystem callbacks */
/**
 * Load a level (just the header, not the data).
 * @param name Name of the level file, without the path
 * @return A newly allocated level with info filled
 */
static Ede_Level*
_load_level_header(const char *name)
{
   char buf[PATH_MAX];
   char str[1024];
   int count = 0;
   Ede_Level *level;
   FILE *fp;

   D(" ");

   // search the level in user dir
   snprintf(buf, sizeof(buf), "%s/.config/ede/levels/%s", getenv("HOME"), name);
   if (!ecore_file_exists(buf))
   {
      // or in system dir
      snprintf(buf, sizeof(buf), PACKAGE_DATA_DIR"/levels/%s", name);
      if (!ecore_file_exists(buf))
      {
         ERR("Can't find level: %s", name);
         return NULL;
      }
   }

   INF("Loading level: %s", buf);

   // open the file
   fp = fopen(buf, "r");
   if (fp == NULL) return NULL;

   // alloc the Ede_Level struct
   level = EDE_NEW(Ede_Level);
   if (!level)
   {
      fclose(fp);
      return NULL;
   }
   level->file = eina_stringshare_add(buf);

   // read the file line by line
   while (fgets(buf, sizeof(buf), fp) != NULL)
   {
      count++;
      // skip invalid lines
      if (!isalnum(buf[0]))
         continue;

      //~ printf("LINE:%s", buf);
      if (sscanf(buf, "Name=%[^\n]", str) == 1)
         level->name = eina_stringshare_add(str);
      else if (sscanf(buf, "Description=%[^\n]", str) == 1)
         level->description = eina_stringshare_add(str);
      else if (sscanf(buf, "Author=%[^\n]", str) == 1)
         level->author = eina_stringshare_add(str);
      else if (sscanf(buf, "Version=%d", &level->version) == 1)
         {}
      else if (sscanf(buf, "Size=%dx%d", &level->cols, &level->rows) == 2)
         {}
      else if (sscanf(buf, "Lives=%d", &level->lives) == 1)
         {}
      else if (sscanf(buf, "Bucks=%d", &level->bucks) == 1)
         {}
      else if (strncmp(buf, "DATA", 4) == 0)
         level->data_start_at_line = count;
   }
   fclose(fp);

   if (!level->file || !level->name || !level->data_start_at_line ||
       !level->cols || !level->rows)
   {
      ERR("Error parsing level.");
      EDE_STRINGSHARE_DEL(level->file);
      EDE_STRINGSHARE_DEL(level->name);
      EDE_STRINGSHARE_DEL(level->description);
      EDE_STRINGSHARE_DEL(level->author);
      EDE_FREE(level);
      return NULL;
   }

   return level;
}

 static void
_wave_add(Ede_Level *level, int count, const char *type,
          int start_base, int speed, int energy, int bucks, int wait)
{
   Ede_Wave *wave;

   wave = EDE_NEW(Ede_Wave);
   if (!wave) return;

   wave->total = count;
   wave->type = eina_stringshare_add(type);
   wave->start_base = start_base;
   wave->speed = speed;
   wave->energy = energy;
   wave->bucks = bucks;
   wave->wait = wait;

   level->waves = eina_list_append(level->waves, wave);
}

static void
_wave_free(Ede_Wave *wave)
{
   EDE_STRINGSHARE_DEL(wave->type);
   EDE_FREE(wave);
}

static void
_level_free(Ede_Level *level)
{
   Ede_Wave *wave;
   D(" ");

   EINA_LIST_FREE(level->waves, wave)
      _wave_free(wave);

   EDE_STRINGSHARE_DEL(level->file);
   EDE_STRINGSHARE_DEL(level->name);
   EDE_STRINGSHARE_DEL(level->description);
   EDE_STRINGSHARE_DEL(level->author);

   ede_array_free((int **)level->cells);

   EDE_FREE(level);
}

static void
_scenario_free(Ede_Scenario *sce)
{
   Ede_Level *level;

   EINA_LIST_FREE(sce->levels, level)
      _level_free(level);
   EDE_STRINGSHARE_DEL(sce->name);
   EDE_STRINGSHARE_DEL(sce->desc);
   EDE_FREE(sce);
}

static void
_parse_scenario(const char *path)
{
   Ede_Scenario *sce;
   Ede_Level *level;
   Eina_List *levels = NULL;
   char line[PATH_MAX], name[64], desc[256], str[256];
   int order = 10;
   FILE *fp;

   D("path: %s", path);

   // open the scenario file
   fp = fopen(path, "r");
   if (fp == NULL) return;
   // parse it
   name[0] = desc[0] = '\0';
   while (fgets(line, sizeof(line), fp) != NULL)
   {
      if (sscanf(line, "Name=%[^\n]", name) == 1) {}
      else if(sscanf(line, "Description=%[^\n]", desc) == 1) {}
      else if(sscanf(line, "Order=%d", &order) == 1) {}
      else if(sscanf(line, "Level=%[^\n]", str) == 1)
      {
         level = _load_level_header(str);
         levels = eina_list_append(levels, level);
      }
   }
   //close
   fclose(fp);

   // alloc the Ede_Scenario struct
   if (name[0] && desc[0] && eina_list_count(levels) > 0)
   {
      sce = EDE_NEW(Ede_Scenario);
      if (!sce) goto error;
      sce->name = eina_stringshare_add(name);
      sce->desc = eina_stringshare_add(desc);
      sce->order = order;
      sce->levels = levels;
      scenarios = eina_list_append(scenarios, sce);
      return;
   }

error:
   EINA_LIST_FREE(levels, level)
      _level_free(level);
}

/* Externally accessible functions */
/**
 * Init all the scenarios and all the levels present in the scenarios.
 * The level will only load header part, need to load level data before play
 */
EAPI Eina_Bool
ede_level_init(void)
{
   Eina_List *files;
   char buf[PATH_MAX];
   char *f;
   D(" ");

   // TODO CHECK ALSO IN USER DIR
   files = ecore_file_ls(PACKAGE_DATA_DIR"/levels/");
   EINA_LIST_FREE(files, f)
   {
      if (eina_str_has_suffix(f, ".scenario"))
      {
         snprintf(buf, sizeof(buf),  PACKAGE_DATA_DIR"/levels/%s", f);
         _parse_scenario(buf);
      }
      EDE_FREE(f);
   }
   return EINA_TRUE;
}

/**
 * Shutdown level system.
 * This will free all the scenarios, the levels and the waves
 */
EAPI Eina_Bool
ede_level_shutdown(void)
{
   Ede_Scenario *sce;
   D(" ");

   EINA_LIST_FREE(scenarios, sce)
      _scenario_free(sce);

   return EINA_TRUE;
}

/**
 * Load the data (grid & waves) from the given level
 * @param level A level stucture as retuned by ede_level_load()
 */
EAPI Eina_Bool
ede_level_load_data(Ede_Level *level)
{
   FILE *fp;
   char line[MAX_LEVEL_COLS];
   int count = 0;
   int col, row;

   if (!level) return EINA_FALSE;
   //~ D("%s [%d]", level->file, level->data_start_at_line);

   // open the file
   fp = fopen(level->file, "r");
   if (fp == NULL) return EINA_FALSE;

   // alloc the 2D array for the cell data
   level->cells = (Ede_Level_Cell**)ede_array_new(level->rows, level->cols);

   // read the DATA part
   row = col = 0;
   while (fgets(line, sizeof(line), fp) != NULL && row < level->rows)
   {
      // skip until the data start
      if (count++ < level->data_start_at_line)
         continue;

      // check row length
      if (strlen(line) - 1 != level->cols)
      {
         ERR("Error parsing level (line %d) wrong row length", count);
         break;
      }

      // read cells values
      for (col = 0; col < level->cols; col++)
      {
         switch (line[col])
         {
            // cell empty
            case '.': level->cells[row][col] = CELL_EMPTY; break;
            // wall
            case '#': level->cells[row][col] = CELL_WALL; break;
            // player home
            case '@': level->home_row = row; level->home_col = col; break;
            /* Enemy Start Base 0..9
             * Fill all the 10 lists in the 'level->starts' array.
             * One list for each base number (level->starts[2] is the list of points for all the base number 2).
             * Two elements(int) in the lists for each start point (row, col).
             * - '1'  convert char to int */
            case '0': case '1': case '2': case '3': case '4':
            case '5':case '6': case '7': case '8': case '9':
               level->starts[line[col] - '0'] = eina_list_append(level->starts[line[col] - '0'], (void*)row);
               level->starts[line[col] - '0'] = eina_list_append(level->starts[line[col] - '0'], (void*)col);
               level->cells[row][col] = CELL_START0 + line[col] - '0';
               break;

            // TODO place turrets here
            default : level->cells[row][col] = CELL_EMPTY; break;
         }
      }
      row++;
   }

   // read the WAVES part
   while (fgets(line, sizeof(line), fp) != NULL)
   {
      char type[32];
      int start_base, speed, energy, wait, bucks;

      // skip comments, blank lines and too short lines
      if (strlen(line) < 5 || line[0] == '#')
         continue;

      // read and add the new wave to the level
      // example line: "21 standard from base 1 [speed: 30 energy: 20 bucks: 15], wait 5"
      if (sscanf(line, "%d %s from base %d [speed: %d energy: %d bucks: %d], wait %d",
                    &count, type, &start_base, &speed, &energy, &bucks, &wait) == 7)
         _wave_add(level, count, type, start_base, speed, energy, bucks, wait);
   }
   fclose(fp);

   if (row != level->rows)
   {
      ERR("Error parsing level");
      //TODO is this the right way to free all the array? right?
      EDE_FREE(*level->cells);
      EDE_FREE(level->cells);
      return EINA_FALSE;
   }

   current_level = level;
   //~ ede_level_dump(level); // DBG
   return EINA_TRUE;
}

EAPI Ede_Level *
ede_level_current_get(void)
{
   return current_level;
}

/**
 * Check if the given cell (in the current level) is walkable
 * NOTE: this need to be fast as it is called lots of time
 * TODO maybe make thia a macro
 */
EAPI Eina_Bool
ede_level_walkable_get(int row, int col)
{
   if (row >= current_level->rows || col >= current_level->cols)
      return EINA_FALSE;
   return current_level->cells[row][col] <= CELL_EMPTY;
}

/**
 * Dump a level to stdout (for debugging purpose)
 */
EAPI void
ede_level_dump(Ede_Level *level)
{
   int col, row, i;
   Eina_List *l;
   Ede_Wave *wave;

   if (!level) return;

   printf("\nLEVEL DUMP\n");
   printf(" File: '%s'\n", level->file);
   printf(" Name: '%s'\n", level->name);
   printf(" Description: '%s'\n", level->description);
   printf(" Author: '%s'\n", level->author);
   printf(" Version: '%d'\n", level->version);
   printf(" Size: '%dx%d'\n", level->cols, level->rows);

   if (level->cells)
   {
      printf("\nLEVEL GRID\n");
      for (row = 0; row < level->rows; row++)
      {
         for (col = 0; col < level->cols; col++)
            printf("%.2d ", level->cells[row][col]);
         printf("\n");
      }
   }

   printf("\nSTARTING POINTS\n");
   for (i = 0; i < 10; i++)
   {
      printf("base %d: %d points", i, eina_list_count(level->starts[i]) / 2);
      l = level->starts[i];
      while (l && l->next)
      {
         printf("(%d, %d)", (int)l->data, (int)l->next->data);
         l = l->next;
         if (l) l = l->next;
      }
      printf("\n");
   }

   printf("\nWAVES:\n");
   i = 0;
   EINA_LIST_FOREACH(level->waves, l, wave)
   {
      printf("#%.3d:  wait %d, spawn %.3d %s from base %d [s:%d e:%d]\n", i++,
             wave->wait,wave->total, wave->type, wave->start_base,
             wave->speed, wave->energy);
   }
   INF("DUMP END");
}

/**
 * Get the list of all the scenarios loaded.
 */
EAPI Eina_List *
ede_level_scenario_list_get(void)
{
   return scenarios;
}

EAPI void
ede_level_debug_info_fill(Eina_Strbuf *t)
{
   Ede_Scenario *sce;
   Ede_Level *level;
   Eina_List *l, *ll;
   char buf[1024];
   int level_count = 0;
   int loaded_level_count = 0;

   // count levels and loaded levels
   EINA_LIST_FOREACH(scenarios, l, sce)
      EINA_LIST_FOREACH(sce->levels, ll, level)
      {
         level_count ++;
         if (level->cells)
            loaded_level_count ++;
      }

   eina_strbuf_append(t, "<h3>Levels:</h3><br>");
   snprintf(buf, sizeof(buf), "scenarios %d<br>levels %d  loaded %d<br>",
            eina_list_count(scenarios), level_count, loaded_level_count);
   eina_strbuf_append(t, buf);
   eina_strbuf_append(t, "<br>");
}
