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


#ifndef EDE_H
#define EDE_H


#include "config.h"
//~ #include "gettext.h"



#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define streql(s1,s2) (strcmp(s1, s2) == 0)

#define EINA_LIST_PUSH(LIST, ELEM) \
   LIST = eina_list_prepend(LIST, ELEM);

#define EINA_LIST_POP(LIST) \
   eina_list_data_get(LIST); \
   LIST = eina_list_remove_list(LIST, LIST);


#define EDE_NEW(s) (s *)calloc(1, sizeof(s))
#define EDE_FREE(p) do { free (p); p = NULL; } while (0)

#define EDE_TIMER_DEL(timer)                      \
   if (timer) {                                   \
       ecore_timer_del(timer);                    \
       timer = NULL;                              \
   }                                              \

#define EDE_EVENT_HANDLER_DEL(event_handler)      \
   if (event_handler) {                           \
       ecore_event_handler_del(event_handler);    \
       event_handler = NULL;                      \
   }                                              \

#define EDE_STRINGSHARE_DEL(string)               \
   if (string) {                                  \
       eina_stringshare_del(string);              \
       string = NULL;                             \
   }                                              \

#define EDE_OBJECT_DEL(obj)                       \
   if (obj) {                                     \
       evas_object_del(obj);                      \
       obj = NULL;                                \
   }                                              \

extern int ede_log_domain;
#define CRITICAL(...) EINA_LOG_DOM_CRIT(ede_log_domain, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR (ede_log_domain, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(ede_log_domain, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(ede_log_domain, __VA_ARGS__)
#define DBG(...)      EINA_LOG_DOM_DBG (ede_log_domain, __VA_ARGS__)


#endif /* EDE_H */
