/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */


#ifndef EDE_H
#define EDE_H


#include "config.h"
//~ #include "gettext.h"



#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


#define EDE_NEW(s) (s *)calloc(1, sizeof(s))
#define EDE_FREE(p) do { free (p); p = NULL; } while (0)

#define EDE_TIMER_DEL(timer)                      \
   if (timer)                                     \
   {                                              \
       ecore_timer_del(timer);                    \
       timer = NULL;                              \
   }                                              \

#define EDE_EVENT_HANDLER_DEL(event_handler)      \
   if (event_handler)                             \
   {                                              \
       ecore_event_handler_del(event_handler);    \
       event_handler = NULL;                      \
   }                                              \

#define EDE_STRINGSHARE_DEL(string)               \
   if (string)                                    \
   {                                              \
       eina_stringshare_del(string);              \
       string = NULL;                             \
   }                                              \

#define EDE_OBJECT_DEL(obj)                       \
   if (obj) {                                     \
       evas_object_del(obj);                      \
       obj = NULL;                                \
   }                                              \

typedef struct _Ede Ede;
struct _Ede
{
   int log_domain; /**< Eina LogDomain */
};

extern Ede *ede;


EAPI int **ede_array_new(int rows, int cols);
EAPI void  ede_array_free(int **array);

#define CRITICAL(...) EINA_LOG_DOM_CRIT(ede->log_domain, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR (ede->log_domain, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(ede->log_domain, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(ede->log_domain, __VA_ARGS__)
#define DBG(...)      EINA_LOG_DOM_DBG (ede->log_domain, __VA_ARGS__)


#endif /* EDE_H */
