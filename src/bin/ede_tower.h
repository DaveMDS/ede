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
}Ede_Tower_Type;


EAPI Eina_Bool ede_tower_init(void);
EAPI Eina_Bool ede_tower_shutdown(void);

EAPI void ede_tower_add(const char *type);
EAPI void ede_tower_destroy_selected(void);
EAPI void ede_tower_deselect(void);
EAPI void ede_tower_one_step_all(double time);



#endif /* EDE_TOWER_H */
