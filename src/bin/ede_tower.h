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


//~ typedef enum {
   //~ TOWER_UNKNOW,
   //~ TOWER_NORMAL,
   //~ TOWER_GHOST,
   //~ TOWER_POWERUP,
   //~ TOWER_SLOWDOWN,
   //~ TOWER_TYPE_NUM
//~ }Ede_Tower_Engine;

/* structure to define a class of towers */
typedef struct _Ede_Tower_Class Ede_Tower_Class;
struct _Ede_Tower_Class {
   const char *id;     // ex: ghost
   const char *name;   // ex: Anti-air
   const char *engine; // ex: ghost
   const char *desc;
   const char *icon;
   const char *image1; // base
   const char *image2; // cannon
   const char *image3; // unused
   int cost;
   double sell_factor;
   Eina_List *params;  // list of Ede_Tower_Class_Param*
};

typedef struct _Ede_Tower_Class_Param Ede_Tower_Class_Param;
struct _Ede_Tower_Class_Param {
   const char *name;     // param name.  ex: Damage
   const char *icon;     // param icon.  ex: upgrade_damage_icon.png
   Eina_List *upgrades;  // list of Ede_Tower_Class_Param_Upgrade*
 };

typedef struct _Ede_Tower_Class_Param_Upgrade Ede_Tower_Class_Param_Upgrade;
struct _Ede_Tower_Class_Param_Upgrade {
   const char *name; // upgrade level name.  ex: level 1
   int value;        // upgrade level value. ex: 60
   int bucks;        // upgrade cost
};

/* structure to define a single tower */
typedef struct _Ede_Tower Ede_Tower;
struct _Ede_Tower {
   Ede_Tower_Class *class;

   Evas_Object *obj;          // edje group 'ede/tower/<class_id>'

   int row, col, rows, cols;  // position & size. In cells
   int center_x, center_y;    // center position. In pixel
   int damage, reload, range; // current values
   int damage_uplvl;          // damage current upgrade level
   int reload_uplvl;          // reload current upgrade level
   int range_uplvl;           // range current upgrade level
   
   double reload_counter; // accumulator for reloading
};

EAPI Eina_Bool ede_tower_init(void);
EAPI Eina_Bool ede_tower_shutdown(void);

EAPI Ede_Tower_Class *ede_tower_class_get_by_id(const char *id);
EAPI Ede_Tower *ede_tower_selected_get(void);

EAPI void ede_tower_add(Ede_Tower_Class *tc);
EAPI void ede_tower_info_update(Ede_Tower *tower);
EAPI void ede_tower_reset(void);
EAPI void ede_tower_upgrade(int button_num);
EAPI void ede_tower_destroy_selected(void);
EAPI void ede_tower_select_at(int row, int col);
EAPI void ede_tower_deselect(void);
EAPI void ede_tower_one_step_all(double time);
EAPI void ede_tower_debug_info_fill(Eina_Strbuf *t);



#endif /* EDE_TOWER_H */
