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

#ifndef EDE_GAME_H
#define EDE_GAME_H


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
EAPI void  ede_game_start(void);
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
EAPI Eina_Bool       ede_game_bucks_pay(int bucks);

#endif /* EDE_GAME_H */
