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

EAPI void* **ede_parray_new(int rows, int cols);
EAPI void    ede_parray_free(void* **array);


#endif /* EDE_UTILS_H */
