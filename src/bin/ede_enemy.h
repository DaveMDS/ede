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

#include <Evas.h>

typedef struct _Ede_Enemy Ede_Enemy;
struct _Ede_Enemy
{
   Evas_Object *obj;
   float x, y; // current position, in pixel (include accumulation)
   int w, h;   // size in pixel
   int angle; // current orientation
   int speed, energy; // current speed & energy
   int target_row, target_col; // target position

   Eina_List *path; // the path to follow as returned by the pathfinder
   int dest_x, dest_y; // this is the pos of the next hop (the one we are approaching)
};


EAPI Eina_Bool ede_enemy_init(void);
EAPI Eina_Bool ede_enemy_shutdown(void);

EAPI void ede_enemy_spawn(const char *type, int speed, int energy,
                          int start_row, int start_col, int end_row, int end_col);
EAPI void ede_enemy_kill(Ede_Enemy *e);
EAPI void ede_enemy_one_step_all(double time);
EAPI void ede_enemy_path_recalc_all(void);
EAPI Ede_Enemy *ede_enemy_nearest_get(int x, int y, int *angle, int *distance);

#endif /* EDE_ENEMY_H */
