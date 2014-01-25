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

#ifndef EDE_LEVEL_H
#define EDE_LEVEL_H


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

typedef struct _Ede_Scenario Ede_Scenario;
struct _Ede_Scenario {
   const char *name;
   const char *desc;
   int order;
   Eina_List *levels;
   Eina_List *levels2;
};

typedef struct _Ede_Level Ede_Level;
struct _Ede_Level
{
   Ede_Scenario *scenario;
   const char *file;
   const char *name;
   const char *description;
   const char *author;
   const char *towers;
   int version;
   int cols, rows;
   int lives;
   int bucks;
   int data_start_at_line;
   int home_row, home_col;

   Eina_List *starts[10]; // 10 lists of enemy starting points (row, col, row, col, etc..)
};

typedef struct _Ede_Wave Ede_Wave;
struct _Ede_Wave{
   int total; // number of enemy in this wave
   int count; // just used as counter by the game engine
   const char *type;
   double delay; // time between each enemy (in sec)
   int start_base;
   int speed;
   int energy;
   int wait;
   int bucks;
};


extern Ede_Level_Cell **cells;
extern Eina_List *waves; // TODO remove this export


EAPI Eina_Bool ede_level_init(void);
EAPI Eina_Bool ede_level_shutdown(void);

EAPI Eina_Bool  ede_level_load_data(Ede_Level *level);
EAPI Ede_Level *ede_level_current_get(void);
EAPI Ede_Level *ede_level_next_get(void);
EAPI Eina_Bool  ede_level_walkable_get(int row, int col);
EAPI void       ede_level_free(Ede_Level *level);
EAPI void       ede_level_dump(Ede_Level *level);
EAPI Eina_List *ede_level_scenario_list_get(void);
EAPI void       ede_level_debug_info_fill(Eina_Strbuf *t);

EAPI void ede_wave_start(void);
EAPI void ede_wave_send(void);
EAPI int  ede_wave_step(double time);

#endif /* EDE_LEVEL_H */
