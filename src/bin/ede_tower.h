/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifndef EDE_TOWER_H
#define EDE_TOWER_H


typedef enum {
   TOWER_UNKNOW,
   TOWER_NORMAL,
   TOWER_GHOST,
   TOWER_POWERUP,
   TOWER_SLOWDOWN,
   TOWER_TYPE_NUM
}Ede_Tower_Engine;

/* structure to define a class of towers */
typedef struct _Ede_Tower_Class Ede_Tower_Class;
struct _Ede_Tower_Class {
   const char *id;
   const char *name;
   const char *engine;
   const char *desc;
   const char *icon;
   const char *image1; // base
   const char *image2; // cannon
   const char *image3; // unused
   int cost;
   double sell_factor;
};


EAPI Eina_Bool ede_tower_init(void);
EAPI Eina_Bool ede_tower_shutdown(void);

EAPI Ede_Tower_Class *ede_tower_class_get_by_id(const char *id);

EAPI void ede_tower_add(Ede_Tower_Class *tc);
EAPI void ede_tower_reset(void);
EAPI void ede_tower_upgrade(int button_num);
EAPI void ede_tower_destroy_selected(void);
EAPI void ede_tower_select_at(int row, int col);
EAPI void ede_tower_deselect(void);
EAPI void ede_tower_one_step_all(double time);
EAPI void ede_tower_debug_info_fill(Eina_Strbuf *t);



#endif /* EDE_TOWER_H */
