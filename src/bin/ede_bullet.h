/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifndef EDE_BULLET_H
#define EDE_BULLET_H


EAPI Eina_Bool ede_bullet_init(void);
EAPI Eina_Bool ede_bullet_shutdown(void);
EAPI void ede_bullet_reset(void);

EAPI void ede_bullet_add(int start_x, int start_y, Ede_Enemy *target, int speed, int damage);
EAPI void ede_bullet_one_step_all(double time);
EAPI void ede_bullet_debug_info_fill(Eina_Strbuf *t);


#endif /* EDE_BULLET_H */
