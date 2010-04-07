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

typedef enum _Ede_Game_State Ede_Game_State;
enum _Ede_Game_State {
   GAME_STATE_UNKNOW,
   GAME_STATE_PAUSE,
   GAME_STATE_PLAYING,
   GAME_STATE_AREA_REQUEST
};

EAPI Eina_Bool ede_game_init(void);
EAPI Eina_Bool ede_game_shutdown(void);

EAPI void  ede_game_debug_hook(void);
EAPI void  ede_game_debug_panel_enable(Eina_Bool enable);

EAPI char *ede_game_time_get(double now);

EAPI void            ede_game_state_set(Ede_Game_State state);
EAPI Ede_Game_State  ede_game_state_get(void);

#endif /* EDE_GAME_H */
