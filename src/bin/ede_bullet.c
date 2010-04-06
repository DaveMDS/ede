/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#include <stdio.h>
#include <math.h>
#include <Eina.h>
#include <Evas.h>

#include "ede.h"
#include "ede_tower.h"
#include "ede_gui.h"
#include "ede_level.h"
#include "ede_utils.h"


#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif


typedef struct _Ede_Bullet Ede_Bulllet;
struct _Ede_Bullet {
   Evas_Object *obj;
   float x, y;
   int w, h;
   Ede_Enemy *target;
   int speed;
   int damage;
};


/* Local subsystem vars */
static Eina_List *bullets = NULL;   //** Current active bullets */
static Eina_List *inactives = NULL; //** List of Ede_Bullet* to reuse */


/* Local subsystem callbacks */


/* Local subsystem functions */
static void
_bullet_del(Ede_Bulllet *b)
{
   EDE_OBJECT_DEL(b->obj);
   EDE_FREE(b);
}


/* Externally accessible functions */
EAPI Eina_Bool
ede_bullet_init(void)
{
   D(" ");

   return EINA_TRUE;
}

EAPI Eina_Bool
ede_bullet_shutdown(void)
{
   Ede_Bulllet *b;
   D(" ");

   EINA_LIST_FREE(inactives, b)
      _bullet_del(b);
   EINA_LIST_FREE(bullets, b)
      _bullet_del(b);
   
   return EINA_TRUE;
}

EAPI void
ede_bullet_add(int start_x, int start_y, Ede_Enemy *target, int speed, int damage)
{
   Ede_Bulllet *b;

   //~ D("active: %d inactive: %d [speed %d]", eina_list_count(bullets), eina_list_count(inactives), speed);

   // get a previusly allocated bullet from the inactive list
   b = EINA_LIST_POP(inactives);
   if (!b)
   {
      // or create a new one
      b = EDE_NEW(Ede_Bulllet);
      if (!b) return;

      b->obj = evas_object_image_filled_add(ede_gui_canvas_get());
      evas_object_image_file_set(b->obj, PACKAGE_DATA_DIR"/themes/bullet1.png", NULL);
      evas_object_image_size_get(b->obj, &b->w, &b->h);
      evas_object_resize(b->obj, b->w, b->h);
   }

   b->speed = speed;
   b->damage = damage;
   b->target = target;
   b->x = start_x - b->w / 2;
   b->y = start_y - b->h / 2;

   evas_object_raise(b->obj);
   evas_object_move(b->obj, b->x, b->y);
   evas_object_show(b->obj);

   EINA_LIST_PUSH(bullets, b);
}

EAPI void
ede_bullet_one_step_all(double time)
{
   Ede_Bulllet *b;
   Eina_List *l, *ll;
   float distance;

   EINA_LIST_FOREACH_SAFE(bullets, l, ll, b)
   {
      // calc distance from target
      distance = ede_util_distance_calc(b->target->x, b->target->y, b->x, b->y);

      // target reached
      if (distance < 10)
      {
         evas_object_hide(b->obj);
         EINA_LIST_PUSH(inactives, b);
         bullets = eina_list_remove_list(bullets, l);
      }

      // calc new position
      b->x += (b->target->x - b->x) / distance * time * 128;
      b->y += (b->target->y - b->y) / distance * time * 128;
      evas_object_move(b->obj, b->x + 0.5, b->y + 0.5);
   }
}
