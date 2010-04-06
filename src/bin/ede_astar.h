/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifndef EDE_ASTAR_H
#define EDE_ASTAR_H

//~ #include "ede.h"


EAPI Eina_Bool ede_pathfinder_init(void);
EAPI Eina_Bool ede_pathfinder_shutdown(void);

EAPI void ede_pathfinder_info_set(Eina_Bool to_console, Eina_Bool in_game);

EAPI Eina_List *ede_pathfinder(int level_rows, int level_cols,
                               int start_row, int start_col,
                               int target_row, int target_col,
                               Eina_Bool (*is_walkable)(int row, int col),
                               int max_loops, Eina_Bool just_check);



#endif /* EDE_ASTAR_H */
