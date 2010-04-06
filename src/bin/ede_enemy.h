/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifndef EDE_ENEMY_H
#define EDE_ENEMY_H

//~ #include "ede.h"

#define EDE_MAX_ENEMIES 1000

typedef struct _Vector
{
   double x;
   double y;
} Vector;

typedef struct _Ede_Enemy Ede_Enemy;
struct _Ede_Enemy
{
   int id; // not really used atm
   float x, y;
   int angle; // current position & orientation
   int speed, energy; // current speed & energy

   Eina_List *path; // the path to follow as returned by the pathfinder
   int dest_x, dest_y;

   void *gui_data; // actually the enemy evas object
};


EAPI Eina_Bool ede_enemy_init(void);
EAPI Eina_Bool ede_enemy_shutdown(void);

EAPI void ede_enemy_spawn(const char *type, int speed, int energy,
                          int start_row, int start_col, int end_row, int end_col);
EAPI void ede_enemy_one_step_all(double time);
EAPI Ede_Enemy *ede_enemy_nearest_get(int x, int y, int *angle, int *distance);

#endif /* EDE_ENEMY_H */
