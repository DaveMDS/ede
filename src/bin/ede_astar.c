/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#include <stdio.h>
#include <Eina.h>

#include "ede.h"
#include "ede_gui.h"
#include "ede_utils.h"

#define LOCAL_DEBUG 0
#if LOCAL_DEBUG
#define D printf
#else
#define D(...)
#endif

#define FULL_DEBUG 0
#if LOCAL_DEBUG & FULL_DEBUG
#define FD printf
#else
#define FD(...)
#endif

// TODO write an intro to astar

/* algo states */
enum {
   ST_SEARCHING,
   ST_TARGET_FOUND,
   ST_TARGET_UNREACHABLE,
   ST_MAXLOOPS_REACHED,
   ST_COUNT
};

char *state_names[ST_COUNT] = {
   "ST_SEARCHING",
   "ST_FOUND",
   "ST_UNREACHABLE",
   "ST_MAXLOOPS"
};

/* Local subsystem vars */
static int **Gcost; // 2 dim array containing the G cost for each cell
static int **Hcost; // 2 dim array containing the H cost for each cell
static int **Fcost; // 2 dim array containing the F cost for each cell
static int **Parent;// 2 dim array containing the packed pos of the parent for each cell
static Eina_List *OpenList; // ordered eina_list of packed cells to check (sorted by F cost for fast access to the lowest F)
static Eina_List *CloseList; // ordered eina_list of packed cells checked yet (sorted by packed coord)
static int LevelRows; // current level size
static int LevelCols; // current level size

static Eina_Bool info_to_console; //whenever to dump to console
static Eina_Bool info_in_game; //whenever to dump results in game


//TODO can optimize this 2?
//TODO describe this
#define PACK(_ROW_,_COL_) (_ROW_ * LevelCols + _COL_)
#define UNPACK(_PACKED_) _PACKED_ / LevelCols, _PACKED_ % LevelCols
#define UNPACK_ROW(_PACKED_) (_PACKED_ / LevelCols)
#define UNPACK_COL(_PACKED_) (_PACKED_ % LevelCols)


/* Local subsystem functions */
static void
_dump_list(Eina_List *list, Eina_Bool to_console, Eina_Bool in_game)
{
   Eina_List *l;
   int r, c, pr, pc;
   int packed;

   if (to_console)
      printf("%sList [count: %d]\n", list == OpenList ? "Open" : "Close",
                                     eina_list_count(list));

   for (l = list; l; l = l->next)
   {
      packed = (long)l->data;

      r = UNPACK_ROW(packed);
      c = UNPACK_COL(packed);
      pr = UNPACK_ROW(Parent[r][c]);
      pc = UNPACK_COL(Parent[r][c]);

      if (to_console)
         printf("  cell: %d (%d,%d) [G:%d H:%d F:%d] [Parent:%d,%d]\n", packed,
                r, c, Gcost[r][c], Hcost[r][c], Fcost[r][c], pr, pc);
      if (in_game)
      {
         ede_gui_cell_overlay_add(list == OpenList ? OVERLAY_BORDER_GREEN : OVERLAY_BORDER_BLUE, r, c);
         ede_gui_cell_overlay_text_set(r, c, Fcost[r][c], 1);
         ede_gui_cell_overlay_text_set(r, c, Gcost[r][c], 2);
         ede_gui_cell_overlay_text_set(r, c, Hcost[r][c], 3);
         if      (pr == r - 1 && pc == c - 1) ede_gui_cell_overlay_add(OVERLAY_7, r, c);
         else if (pr == r - 1 && pc == c ) ede_gui_cell_overlay_add(OVERLAY_0, r, c);
         else if (pr == r - 1 && pc == c + 1) ede_gui_cell_overlay_add(OVERLAY_1, r, c);
         else if (pr == r && pc == c - 1) ede_gui_cell_overlay_add(OVERLAY_6, r, c);
         else if (pr == r && pc == c) {}
         else if (pr == r && pc == c + 1) ede_gui_cell_overlay_add(OVERLAY_2, r, c);
         else if (pr == r + 1 && pc == c - 1) ede_gui_cell_overlay_add(OVERLAY_5, r, c);
         else if (pr == r + 1 && pc == c) ede_gui_cell_overlay_add(OVERLAY_4, r, c);
         else if (pr == r + 1 && pc == c + 1) ede_gui_cell_overlay_add(OVERLAY_3, r, c);
      }
   }
   if (to_console) printf("\n");
}

static int
_sort_by_Fcost(const void *packed1, const void *packed2)
{
   int row, col;
   int Fa, Fb;

   // get F cost for element 1
   row = UNPACK_ROW((int)packed1);
   col = UNPACK_COL((int)packed1);
   Fa = Fcost[row][col];

   // get F cost for element 2
   row = UNPACK_ROW((int)packed2);
   col = UNPACK_COL((int)packed2);
   Fb = Fcost[row][col];

   // simply compare F costs
   return Fa - Fb;
}

static int
_sort_by_packed(const void *packed1, const void *packed2)
{
   return (int)packed1 - (int)packed2;
}

/* Externally accessible functions */
EAPI Eina_Bool
ede_pathfinder_init(void)
{
   DBG(" ");
   Gcost = Hcost = Fcost = Parent = NULL;
   OpenList = CloseList = NULL;
   LevelRows = LevelCols = 0;
   info_to_console = info_in_game = EINA_FALSE;
   return EINA_TRUE;
}

EAPI Eina_Bool
ede_pathfinder_shutdown(void)
{
   DBG(" ");
   ede_array_free(Gcost);
   ede_array_free(Hcost);
   ede_array_free(Fcost);
   ede_array_free(Parent);
   return EINA_TRUE;
}

EAPI Eina_List *
ede_pathfinder(int level_rows, int level_cols,
               int start_row, int start_col,
               int target_row, int target_col,
               Eina_Bool (*is_walkable)(int row, int col),
               int max_loops, Eina_Bool just_check)
{
   Eina_Counter *time_counter;
   Eina_List *path = NULL; // RETURNED. List of packed cells that make the route to follow for reaching the target
   int adiacentPacked, startPacked;
   int curRow, curCol, curPacked; // point to the cell we are checking
   int row, col; // used to loop the 8 adiacent cell
   int loops, G, state;
   Eina_List *l;
   Eina_Bool corner_walkable;

   if (max_loops < 1) max_loops = level_rows * level_cols;
   
   D("\n----------  A *  -----------\n");
   D("From: %d,%d To: %d,%d [map: %d,%d][max loops: %d]\n\n", start_row, start_col,
           target_row, target_col, level_rows, level_cols, max_loops);

   time_counter = eina_counter_new("Ede A*");
   eina_counter_start(time_counter);

   // alloc/realloc all the array if needed
   //TODO realloc on board size change
   if (!Gcost)
   {
      D("REALLOC costs & parent arrays (G, H and F. ParentX & ParentY)\n");
      Gcost = ede_array_new(level_rows, level_cols);
      Hcost = ede_array_new(level_rows, level_cols);
      Fcost = ede_array_new(level_rows, level_cols);
      Parent = ede_array_new(level_rows, level_cols);
      if (!Gcost || !Hcost || !Fcost || !Parent) return EINA_FALSE;
   }
   
   eina_counter_stop(time_counter, 0);
   eina_counter_start(time_counter);

   // store the level dimension in a global variable to be accessibile from
   // the list sort function
   LevelRows = level_rows;
   LevelCols = level_cols;

   // Check to see if start and target are walkable
   if (!is_walkable(start_row, start_col) ||
       !is_walkable(target_row, target_col))
   {
      state = ST_TARGET_UNREACHABLE;
      goto end;
   }

   // TODO check if start and target are the same cell


   // add the starting location to the open list of cells to be checked
   // and set its G cost to 0
   startPacked = PACK(start_row, start_col);
   OpenList = eina_list_append(OpenList, (void*)startPacked);
   Gcost[start_row][start_col] = 0;

   loops = 0;
   state = ST_SEARCHING;
   do // until a path is found, max loops reached or destination unreachable.
   {
      loops++;

      // if the open list is not empty, take the first cell of the list.
      // this is the lowest F cost cell (packed) on the open list.
      if (eina_list_count(OpenList) != 0)
      {
         // pop the first (packed) cell from the open list
         curPacked = (long)eina_list_data_get(OpenList);
         OpenList = eina_list_remove_list(OpenList, OpenList);
         // sorted insert the (packed) cell in the close list
         CloseList = eina_list_sorted_insert(CloseList, _sort_by_packed,
                                             (void*)curPacked);

         curRow = UNPACK_ROW(curPacked);
         curCol = UNPACK_COL(curPacked);
         FD("\nCHECKING CELL: %d,%d [%d]\n", curRow, curCol, curPacked);

         // check all the adjacent squares.
         for (row = curRow - 1; row <= curRow + 1; row++)
         {
            for (col = curCol - 1; col <= curCol + 1; col++)
            {
               // do not check ourself
               if (row == curRow && col == curCol) continue;

               FD("  checking adiacent: %d,%d ..", row, col);

               // If is inside the map (do this first to prevent array-out-of-bounds problems)
               if (row != -1 && col != -1 && row != LevelRows && col != LevelCols)
               {
                  // If is a walkable cell.
                  if (is_walkable(row, col))
                  {
                     adiacentPacked = PACK(row,col);
                     // If not already on the closed list
                     if (!eina_list_search_sorted(CloseList, _sort_by_packed, (void*)adiacentPacked)) //FAST
                     //~ if (!eina_list_data_find(CloseList, (void*)adiacentPacked)) /// !! SLOW !! ... in real I cant see any difference in total time :/
                     {
                        // Don't cut across corners
                        corner_walkable = EINA_TRUE;
                        if (row == curRow - 1) 
                        {
                           if (col == curCol - 1) // top-left
                           {
                              if (!is_walkable(curRow - 1, curCol) ||
                                  !is_walkable(curRow, curCol - 1))
                                corner_walkable = EINA_FALSE;
                           }
                           else if (col == curCol + 1) // top-right
                           {
                              if (!is_walkable(curRow, curCol + 1) ||
                                  !is_walkable(curRow - 1, curCol)) 
                                corner_walkable = EINA_FALSE;
                           }
                        }
                        else if (row == curRow + 1)
                        {
                           if (col == curCol - 1) // bottom-left
                           {
                              if (!is_walkable(curRow, curCol - 1) ||
                                 !is_walkable(curRow + 1, curCol)) 
                                 corner_walkable = EINA_FALSE;
                           }
                           else if (col == curCol + 1) // bottom-right
                           {
                              if (!is_walkable(curRow + 1, curCol) ||
                                  !is_walkable(curRow, curCol + 1))
                                 corner_walkable = EINA_FALSE;
                           }
                        }
                        if (corner_walkable)
                        {
                           // If not already on the open list, calculate and add it to the open list
                           if (!eina_list_data_find(OpenList, (void*)adiacentPacked)) // // !! SLOW !! can not use sorted search here because the open list is ordered by Fcost, not packed.
                           {
                              FD(". new cell, calc and put in open list.\n");
                              // calc G cost
                              Gcost[row][col] =
                                 (abs(row - curRow) == 1 && abs(col - curCol) == 1) ?
                                    Gcost[curRow][curCol] + 14 : // cost of going to diagonal cell
                                    Gcost[curRow][curCol] + 10 ; // cost of going to hortogonal cell
                              // calc H and F costs
                              Hcost[row][col] = 10 * (abs(row - target_row) + abs(col - target_col));
                              Fcost[row][col] = Gcost[row][col] + Hcost[row][col];
                              // store parent packed position
                              Parent[row][col] = curPacked;
                              // add to the open list
                              OpenList = eina_list_sorted_insert(OpenList, _sort_by_Fcost, (void*)adiacentPacked);// !! SLOW !!
                           }
                           else
                           {
                              FD(". already on open list, checking if it's a shorter way ..");
                              // If adjacent cell is already on the open list, check to see if this 
                              // path to that cell from the starting location is a better one. 
                              // If so, change the parent of the cell and its G and F costs.

                              //Figure out the G cost of this possible new path
                              G = (abs(row - curRow) == 1 && abs(col - curCol) == 1) ?
                                   Gcost[curRow][curCol] + 14 : // cost of going to diagonal cell
                                   Gcost[curRow][curCol] + 10 ; // cost of going to hortogonal cell

                              // If this path is shorter (G cost is lower) then change
                              // the parent cell, G cost and F cost.
                              if (G < Gcost[row][col]) //if G cost is less,
                              {
                                 FD(". yes, updating G and F.\n");
                                 Parent[row][col] = curPacked;          // change the square's parent
                                 Gcost[row][col] = G;                   // change the G cost
                                 Fcost[row][col] = G + Hcost[row][col]; // change F cost
                                 // because changing the G cost also changes the F cost, we
                                 // need to change the item's position in the open list to make
                                 // sure that we maintain a properly ordered open list.
                                 OpenList = eina_list_remove(OpenList, (void*)adiacentPacked); // !! SLOW !!
                                 OpenList = eina_list_sorted_insert(OpenList, _sort_by_Fcost, (void*)adiacentPacked); // !! SLOW !!
                              }else { FD(". no, leave as is.\n"); }
                           }
                        } else { FD(". cutting corner, forbidden.\n"); }
                     } else { FD(". cell on closed list yet, skipping.\n"); }
                  } else { FD(". cell not walkable, skipping.\n"); }
               } else { FD(". cell outside map, skipping.\n"); }
            }
         }
      }
      else // open_list is empty, there is no path.
      {
         state = ST_TARGET_UNREACHABLE;
         break;
      }

      // if target is in the open list then path has been found.
      if (eina_list_data_find(OpenList, (void*)PACK(target_row, target_col))) // !! SLOW !!
      {
         state = ST_TARGET_FOUND;
         break;
      }

      if (loops > max_loops)
      {
         state = ST_MAXLOOPS_REACHED;
         break;
      }

   }while (1); // break if a path is found, max loops is reached or destination is unreachable.

end:
   eina_counter_stop(time_counter, 1);

   // if target found (and not just_check mode) build the path to follow
   if (state == ST_TARGET_FOUND && !just_check)
   {
      D("\nTarget found, building path to follow.\n");
      // first insert the target in the path list
      path = eina_list_append(path, (void*)target_row);
      path = eina_list_append(path, (void*)target_col);
      // then insert each hops starting from the target and following the parents
      curPacked = Parent[target_row][target_col];
      while (curPacked != startPacked) // while start point reached
      {
         path = eina_list_prepend(path, (void*)UNPACK_COL(curPacked)); // prepend the new hop to the path list
         path = eina_list_prepend(path, (void*)UNPACK_ROW(curPacked)); // prepend the new hop to the path list
         curPacked = Parent[UNPACK_ROW(curPacked)][UNPACK_COL(curPacked)]; // 'follow' the parents
      }
   }

   // report
   D("\n---------   A*  ---------------\n");
   D("Result: %s\n", state_names[state]);
   D("Total loops: %d\n", loops);
   if (path)
   {
      D("Path (%d total hops):\n", eina_list_count(path));
      for (l = path; l; l = l->next->next)
         D(" (%ld,%ld)", (long)l->data, (long)l->next->data);
      D("\n");
   }
   D("\n%s\n", eina_counter_dump(time_counter));
   D("----------  A* end ----------\n\n");
   
   // dump open & close list, to console  and/or  in  game
   _dump_list(OpenList, info_to_console, info_in_game);
   _dump_list(CloseList, info_to_console, info_in_game);

   // free stuff
   eina_counter_free(time_counter);
   eina_list_free(OpenList);
   OpenList = NULL;
   eina_list_free(CloseList);
   CloseList = NULL;

   if (just_check)
      return (void *)(state == ST_TARGET_FOUND);
   else
      return path;
}

EAPI void
ede_pathfinder_info_set(Eina_Bool to_console, Eina_Bool in_game)
{
   info_to_console = to_console;
   info_in_game = in_game;
}
