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

#include "ede_level.h"

typedef enum _Ede_Game_State Ede_Game_State;
enum _Ede_Game_State {
   GAME_STATE_UNKNOW,
   GAME_STATE_MAINMENU,
   GAME_STATE_LEVELSELECTOR,
   GAME_STATE_PAUSE,
   GAME_STATE_PLAYING,
   GAME_STATE_AREA_REQUEST
};


EAPI Eina_Bool ede_game_init(void);
EAPI Eina_Bool ede_game_shutdown(void);

EAPI void  ede_game_mainmenu_populate(void);
EAPI void  ede_game_start(Ede_Level *level);
EAPI void  ede_game_reset(void);
EAPI void  ede_game_pause(void);
EAPI void  ede_game_quit(void);

EAPI void  ede_game_debug_hook(void);
EAPI void  ede_game_debug_panel_enable(Eina_Bool enable);
EAPI void  ede_game_debug_panel_update(double now);

EAPI char *ede_game_time_get(double now);

EAPI void            ede_game_state_set(Ede_Game_State state);
EAPI Ede_Game_State  ede_game_state_get(void);
EAPI void            ede_game_home_violated(void);
EAPI int             ede_game_bucks_get(void);
EAPI void            ede_game_bucks_gain(int bucks);

#endif /* EDE_GAME_H */
