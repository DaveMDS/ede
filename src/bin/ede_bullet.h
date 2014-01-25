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

#ifndef EDE_BULLET_H
#define EDE_BULLET_H


EAPI Eina_Bool ede_bullet_init(void);
EAPI Eina_Bool ede_bullet_shutdown(void);
EAPI void ede_bullet_reset(void);

EAPI void ede_bullet_add(int start_x, int start_y, Ede_Enemy *target, int speed, int damage);
EAPI void ede_bullet_one_step_all(double time);
EAPI void ede_bullet_debug_info_fill(Eina_Strbuf *t);


#endif /* EDE_BULLET_H */
