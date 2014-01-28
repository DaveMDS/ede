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

#ifndef EDE_ASTAR_H
#define EDE_ASTAR_H


EAPI Eina_Bool ede_pathfinder_init(void);
EAPI Eina_Bool ede_pathfinder_shutdown(void);

EAPI void ede_pathfinder_info_set(Eina_Bool to_console, Eina_Bool in_game);

EAPI Eina_List *ede_pathfinder(int level_rows, int level_cols,
                               int start_row, int start_col,
                               int target_row, int target_col,
                               Eina_Bool (*is_walkable)(int row, int col),
                               int max_loops, Eina_Bool just_check);



#endif /* EDE_ASTAR_H */
