/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#ifndef EDE_GAME_H
#define EDE_GAME_H

//~ #include "ede.h"


EAPI Eina_Bool ede_game_init(void);
EAPI Eina_Bool ede_game_shutdown(void);

EAPI void  ede_game_debug_hook(void);
EAPI void  ede_game_debug_panel_enable(Eina_Bool enable);
EAPI char *ede_game_time_get(double now);

#endif /* EDE_GAME_H */
