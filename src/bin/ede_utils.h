/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifndef EDE_UTILS_H
#define EDE_UTILS_H

#include <Evas.h>

#define PI 3.14159265

// UNUSED
typedef struct _Vector
{
   double x;
   double y;
} Vector;

EAPI void   vector_set(Vector *v, float x, float y);
EAPI Vector vector_add(Vector v1, Vector v2);
EAPI float  vector_lenght(Vector v);
EAPI Vector vector_normalize(Vector v);
// UNUSED END


EAPI void ede_util_obj_rotate(Evas_Object *obj, int angle);

EAPI int ede_util_angle_calc(int x1, int y1, int x2, int y2);
EAPI int ede_util_distance_calc(int x1, int y1, int x2, int y2);

EAPI int **ede_array_new(int rows, int cols);
EAPI void  ede_array_free(int **array);


#endif /* EDE_UTILS_H */
