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

#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif

#define MAX_LEVEL_COLS 1024
#define MAX_LEVEL_ROWS 1024


/* Local subsystem vars */
static Ede_Level *current_level = NULL;

/* Local subsystem callbacks */

 static void
_ede_level_wave_add(Ede_Level *level, int count, const char *type,
                    int start_base, int speed, int energy, int wait)
{
   Ede_Wave *wave;

   wave = EDE_NEW(Ede_Wave);
   if (!wave) return;

   wave->total = count;
   wave->type = eina_stringshare_add(type);
   wave->start_base = start_base;
   wave->speed = speed;
   wave->energy = energy;
   wave->wait = wait;

   level->waves = eina_list_append(level->waves, wave);
}

static void
_ede_wave_free(Ede_Wave *wave)
{
   EDE_STRINGSHARE_DEL(wave->type);
   EDE_FREE(wave);
}


/* Externally accessible functions */

/**
 * Load a level (just the header, not the data).
 * @param name Name of the level file, without the path
 * @return A newly allocated level with info filled
 */
EAPI Ede_Level*
ede_level_load_header(const char *name)
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
      else if (sscanf(buf, "Size=%dx%d", &level->cols, &level->rows) == 1)
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
      int start_base, speed, energy, wait;

      // skip comments, blank lines and too short lines
      if (strlen(line) < 5 || line[0] == '#')
         continue;

      // read and add the new wave to the level
      // example line: spawn 10 standard enemy from base 1 [speed: 10 energy: 100], wait 5
      if (sscanf(line, "spawn %d %s enemy from base %d [speed: %d energy: %d], wait %d",
                    &count, type, &start_base, &speed, &energy, &wait) == 6)
         _ede_level_wave_add(level, count, type, start_base, speed, energy, wait);
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

EAPI Eina_Bool
ede_level_walkable_get(int row, int col)
{
   if (row >= current_level->rows || col >= current_level->cols)
      return EINA_FALSE;
   return current_level->cells[row][col] <= CELL_EMPTY;
}

/**
 * Free all the level data structures
 */
EAPI void
ede_level_free(Ede_Level *level)
{
   Ede_Wave *wave;
   D(" ");

   EINA_LIST_FREE(level->waves, wave)
      _ede_wave_free(wave);
   
   EDE_STRINGSHARE_DEL(level->file);
   EDE_STRINGSHARE_DEL(level->name);
   EDE_STRINGSHARE_DEL(level->description);
   EDE_STRINGSHARE_DEL(level->author);
   
   ede_array_free((int **)level->cells);

   EDE_FREE(level);
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
