/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifndef EDE_LEVEL_H
#define EDE_LEVEL_H

//~ #include "ede.h"

typedef enum {
   CELL_UNKNOW,
   CELL_START0,
   CELL_START1,
   CELL_START2,
   CELL_START3,
   CELL_START4,
   CELL_START5,
   CELL_START6,
   CELL_START7,
   CELL_START8,
   CELL_START9,
   CELL_EMPTY,
   CELL_WALL,  // all the value from here are considered unwalkable.
   CELL_TOWER,
   CELL_HOME
}Ede_Level_Cell;


typedef struct _Ede_Level Ede_Level;
struct _Ede_Level
{
   const char *file;
   const char *name;
   const char *description;
   const char *author;
   int         version;
   int         cols, rows;
   int         lives;
   int         bucks;
   int         data_start_at_line;
   Ede_Level_Cell **cells; // access as: level->cells[row][col]
   int         home_row, home_col;
   

   Eina_List *starts[10]; // 10 lists of enemy starting points (row, col, row, col, etc..)
   Eina_List *waves;
};

typedef struct _Ede_Wave Ede_Wave;
struct _Ede_Wave{
   int total; // number of enemy in this wave
   int count; // just used as counter by the game engine
   const char *type;
   int start_base;
   int speed;
   int energy;
   int wait;
   int bucks;
};


EAPI Ede_Level *ede_level_load_header(const char *name);
EAPI Eina_Bool  ede_level_load_data(Ede_Level *level);
EAPI Ede_Level *ede_level_current_get(void);
EAPI Eina_Bool  ede_level_walkable_get(int row, int col);
EAPI void       ede_level_free(Ede_Level *level);
EAPI void       ede_level_dump(Ede_Level *level);

#endif /* EDE_LEVEL_H */
