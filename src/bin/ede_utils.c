/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */


#include <math.h>
#include <Eina.h>
#include <Evas.h>

#include "ede.h"
#include "ede_utils.h"



#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif


EAPI void
ede_util_obj_rotate(Evas_Object *obj, int angle)
{
   Evas_Map *map;
   int x, y, w, h;

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   map = evas_map_new(4);
   evas_map_util_points_populate_from_object(map, obj);

   evas_map_util_rotate(map, angle, x + w / 2, y + h / 2);
   evas_object_map_enable_set(obj, 1);
   evas_object_map_set(obj, map);
   evas_map_free(map);
}


/**************   BASIC CALCS   ***********************************************/
EAPI int
ede_util_angle_calc(int x1, int y1, int x2, int y2)
{
   float a;

   a = atan2(y2 - y1, x2 - x1) * 180 / PI;
   return (int)(a + 0.5) + 90;
}

EAPI int
ede_util_distance_calc(int x1, int y1, int x2, int y2)
{
   int dx, dy;
   dx = x1 - x2;
   dy = y1 - y2;
   return sqrt(dx*dx + dy*dy);
}


/**************   2D ARRAY STUFF   ********************************************/
EAPI int **
ede_array_new(int rows, int cols)
{
   int row;
   int *aptr;
   int **rptr;

   // alloc mem for the cells and the rows pointers
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


/**************   VECTOR STUFF   **********************************************/
EAPI Vector
vector_add(Vector v1, Vector v2)
{
   Vector sum;
   sum.x = v1.x + v2.x;
   sum.y = v1.y + v2.y;
   return sum;
}

EAPI void
vector_set(Vector *v, float x, float y)
{
   (*v).x = x;
   (*v).y = y;
}

EAPI float
vector_lenght(Vector v)
{
   return sqrt((v.x * v.x) + (v.y * v.y));
}

EAPI Vector
vector_normalize(Vector v)
{
   Vector n;
   float l;

   l = vector_lenght(v);
   n.x = v.x / l;
   n.y = v.y / l;

   return n;
}
