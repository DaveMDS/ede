/*
 *  Ede - EFL Defender Environment
 *
 *  Copyright (C) 2010 Davide Andreoli <dave@gurumeditation.it>
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_File.h>
#include <Edje.h>

#include "ede.h"
#include "ede_gui.h"
#include "ede_game.h"
#include "ede_level.h"
#include "ede_astar.h"
#include "ede_tower.h"
#include "ede_utils.h"

#define LOCAL_DEBUG 1
#if LOCAL_DEBUG
#define D DBG
#else
#define D(...)
#endif


#define STAGE_OFFSET_X 180
#define STAGE_OFFSET_Y 60

/* Local subsystem vars */
static Evas_Object ***overlays = NULL; /** 2D dynamic array of Evas_Object pointers.
                                           One for each cell of the grid */

static const char *theme_file; /** full path to the theme file */
static Ecore_Evas *window;     /** window handle */
static Evas *canvas;           /** evas canvas */
static Evas_Object *o_layout;  /** main edje object containing the interface */
static Evas_Object *o_menu;           /** main menu edje object (group: ede/menu) */
static Evas_Object *o_levelselector;  /** levelselector menu  (group: ede/levelselector) */

static Eina_List *event_handlers; /** list of connected event handlers */

static Evas_Object *o_checkboard; /**< the level background object */
static int checkboard_rows, checkboard_cols; /**< current size of the checkboard */


static Evas_Object *o_selection; /** the object used to select map locations */
static Evas_Object *o_circle; /** Polygon obj used for the selection range */
static int area_req_rows, area_req_cols; /** size of the current area request */
static void (*area_req_done_cb)(int row, int col, int w, int h, void *data); /** function to call on area selection complete */
static void *area_req_done_data; /** user data to pass-back in the area_req_done_cb */
static Eina_Bool selection_ok;   /** true if the selection is in a free position */


/* Local protos */
static void _area_request_mouse_down(int x, int y, Eina_Bool inside_checkboard, Eina_Bool on_a_tower);
static void _area_request_mouse_move(int x, int y);


/* Local subsystem functions */
static void // TODO removeme
_move_at(Evas_Object *obj, int row, int col)
{
   int x = 0, y = 0;

   ede_gui_cell_coords_get(row, col, &x, &y, EINA_FALSE);
   evas_object_move(obj, x, y);
}

void
_circle_recalc(Evas_Object *obj, int center_x, int center_y, int radius)
{
   int x, y, r2;

   D("Circle recalc x: %d  y: %d  radius: %d", center_x, center_y, radius);
   evas_object_polygon_points_clear(obj);

   r2 = radius * radius;
   for (x = -radius; x <= radius; x += 2)
   {
      y = (int)(sqrt(r2 - x*x) + 0.5);
      evas_object_polygon_point_add(obj, center_x + x, center_y + y);
   }
   for (x = radius; x > -radius; x -= 2)
   {
      y = (int) (sqrt(r2 - x*x) + 0.5);
      evas_object_polygon_point_add(obj, center_x + x, center_y - y);
   }
}

static Eina_Bool
_point_inside_checkboard(int x, int y)
{
   Ede_Level *level = ede_level_current_get();

   return (x > STAGE_OFFSET_X &&
           x < STAGE_OFFSET_X + level->cols * CELL_W &&
           y > STAGE_OFFSET_Y &&
           y < STAGE_OFFSET_Y + level->rows * CELL_H);
}

/* Local subsystem callbacks */
static void
_window_delete_req_cb(Ecore_Evas *window)
{
   D(" ");
  ede_game_quit();
}

static void
_debug_button_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
   D(" ");
   ede_game_debug_hook();
}

static void
_menu_button_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
   D(" ");
   ede_game_mainmenu_populate();
}

static void
_upgrade_button_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
   int num;

   D(" as");

   num = atoi(source);
   printf("ASD %s %d\n", source, num);
   ede_tower_upgrade(num);
}

static void
_add_tower_add_button_cb(void *data, Evas_Object *o, Evas *e, void *event_info)
{
   Ede_Tower_Class *tc = data;
   D("'%s'", tc->engine);
   ede_tower_add(tc);
}

static void
_next_wave_button_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
   D(" ");
   ede_wave_send();
}

static void
_menu_item_selected(void *data, Evas_Object *o, const char *emission, const char *source)
{
   void (*selected_cb)(void *data);
   void *cb_data;

   selected_cb = evas_object_data_get(o, "item_cb");
   cb_data = evas_object_data_get(o, "item_data");

   if (selected_cb)
      selected_cb(cb_data);
}

EAPI void
ede_gui_debug_text_set(const char *text)
{
   edje_object_part_text_set(o_layout, "debug.panel.text", text);
}

static int
_ecore_event_key_down_cb(void *data, int type, void *event)
{
   Ecore_Event_Key *ev = event;
   static Eina_Bool dpanel_visible = EINA_FALSE;

   if (!strcmp(ev->key, "d"))
   {
      D("D [destroy]");
   }
   if (!strcmp(ev->key, "p"))
   {
      D("P [pause]");
      ede_game_pause();
   }
   else if (!strcmp(ev->key, "F12"))
   {
      D("F12: toggle debug panel");
      dpanel_visible = !dpanel_visible;
      if (dpanel_visible)
      {
         edje_object_signal_emit(o_layout, "debug,panel,show", "");
         ede_game_debug_panel_enable(EINA_TRUE);
         ede_game_debug_panel_update(0.0);
      }
      else
      {
         edje_object_signal_emit(o_layout, "debug,panel,hide", "");
         ede_game_debug_panel_enable(EINA_FALSE);
      }
   }
   else if (!strcmp(ev->key, "Escape"))
   {
      D("ESC");
   }


   return ECORE_CALLBACK_CANCEL;
}

static int
_ecore_event_mouse_move_cb(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Move *ev = event;
   Ede_Game_State state = ede_game_state_get();

   if (state == GAME_STATE_AREA_REQUEST)
      _area_request_mouse_move(ev->x, ev->y);

   return ECORE_CALLBACK_CANCEL;
}

static int
_ecore_event_mouse_down_cb(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Ede_Game_State state = ede_game_state_get();
   Eina_Bool inside_checkboard;
   Eina_Bool on_a_tower = EINA_FALSE;
   int row, col;

   if (state < GAME_STATE_PLAYING) return ECORE_CALLBACK_CANCEL;

   inside_checkboard = _point_inside_checkboard(ev->x, ev->y);
   if (inside_checkboard)
   {
      ede_gui_cell_get_at_coords(ev->x, ev->y, &row, &col);
      on_a_tower = (cells[row][col] == CELL_TOWER);
   }

   if (state == GAME_STATE_AREA_REQUEST)
   {
      _area_request_mouse_down(ev->x, ev->y, inside_checkboard, on_a_tower);
      return ECORE_CALLBACK_CANCEL;
   }

   if (state == GAME_STATE_PLAYING)
   {
      if (inside_checkboard)
      {
         if (on_a_tower)
            ede_tower_select_at(row, col);
         else
            ede_tower_deselect();
      }
      return ECORE_CALLBACK_CANCEL;
   }

   return ECORE_CALLBACK_CANCEL;
}


/* Externally accessible functions */

/**
 * Init gui. Create the main window
 */
EAPI Eina_Bool
ede_gui_init(void)
{
   char buf[PATH_MAX];
   Eina_List *engines, *l;
   const char *e;

   D(" ");
   evas_init();
   ecore_evas_init();
   edje_init();

   // list ecore_evas engines
   *buf = '\0';
   engines = ecore_evas_engines_get();
   EINA_LIST_FOREACH(engines, l, e)
   {
      strcat(buf, e);
      if (l->next) strcat(buf, ", ");
   }
   INF("Available evas engine: %s", buf);
   ecore_evas_engines_free(engines);

   // create window
   window = ecore_evas_new(NULL, 0, 0, WIN_W, WIN_H, NULL);
   if (!window)
   {
      CRITICAL("could not create window.");
      return EINA_FALSE;
   }
   ecore_evas_size_min_set(window, WIN_W, WIN_H);
   ecore_evas_size_step_set(window, 50, 50);
   ecore_evas_title_set(window, "EFL Defender Game");
   ecore_evas_callback_delete_request_set(window, _window_delete_req_cb);
   INF("Using evas engine: %s", ecore_evas_engine_name_get(window));
   ecore_evas_show(window);
   canvas = ecore_evas_get(window);


   // find the theme to use
   snprintf(buf, sizeof(buf), PACKAGE_DATA_DIR"/themes/default.edj");
   if (!ecore_file_exists(buf))
   {
      CRITICAL("can not find default theme. Aborting...");
      return EINA_FALSE;
   }
   theme_file = eina_stringshare_add(buf);
   INF("Theme: '%s'", theme_file);


   // create the main layout edje object
   o_layout = edje_object_add(canvas);
   if (!edje_object_file_set(o_layout, theme_file, "ede/layout"))
   {
      CRITICAL("error loading  theme file: %s", buf);
      return EINA_FALSE;
   }
   ecore_evas_object_associate(window, o_layout, ECORE_EVAS_OBJECT_ASSOCIATE_BASE);
   evas_object_resize(o_layout, WIN_W, WIN_H);
   evas_object_show(o_layout);
   edje_object_signal_callback_add(o_layout, "mouse,down,1", "a button", _debug_button_cb, NULL);
   edje_object_signal_callback_add(o_layout, "send,next,wave", "", _next_wave_button_cb, NULL);
   edje_object_signal_callback_add(o_layout, "mouse,down,1", "menu_button", _menu_button_cb, NULL);
   edje_object_signal_callback_add(o_layout, "upgrade,pressed", "*", _upgrade_button_cb, NULL);
   ede_gui_upgrade_box_hide_all();

   // create the checkboard object
   o_checkboard = edje_object_add(canvas);
   edje_object_file_set(o_checkboard, theme_file, "ede/checkboard");
   _move_at(o_checkboard, 0, 0);
   evas_object_resize(o_checkboard, 0, 0);
   evas_object_show(o_checkboard);

   // create the selection object
   o_selection = edje_object_add(canvas);
   edje_object_file_set(o_selection, theme_file, "ede/selection");
   evas_object_layer_set(o_selection, LAYER_SELECTION);
   evas_object_pass_events_set(o_selection, EINA_TRUE);
   // selection circle
   o_circle = evas_object_polygon_add(canvas);
   evas_object_pass_events_set(o_selection, EINA_TRUE);
   evas_object_color_set(o_circle, 40, 40, 40, 40);
   Evas_Object *clipper; //TODO clip to the checkboard, not the stage.clipper
   clipper = (Evas_Object *)edje_object_part_object_get(o_layout, "stage.clipper");
   evas_object_clip_set(o_circle, clipper);


   // create the mainmenu object
   o_menu = edje_object_add(canvas);
   edje_object_file_set(o_menu, theme_file, "ede/menu");
   evas_object_layer_set(o_menu, LAYER_MENU);
   evas_object_move(o_menu, 200, 80); //TODO FIXME
   evas_object_resize(o_menu, 400, 400); //TODO FIXME

   // create the levelselector menu object
   o_levelselector = edje_object_add(canvas);
   edje_object_file_set(o_levelselector, theme_file, "ede/levelselector");
   evas_object_move(o_levelselector, 200, 80); //TODO FIXME
   evas_object_resize(o_levelselector, 400, 400); //TODO FIXME

   // connect keyboard & mouse event
   event_handlers = eina_list_append(event_handlers,
                     ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                             _ecore_event_key_down_cb, NULL));
   event_handlers = eina_list_append(event_handlers,
                     ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                             _ecore_event_mouse_move_cb, NULL));
   event_handlers = eina_list_append(event_handlers,
                     ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                             _ecore_event_mouse_down_cb, NULL));

   return EINA_TRUE;
}

/**
 * Shutdown gui system, close the window and free all the resource
 */
EAPI Eina_Bool
ede_gui_shutdown(void)
{
   Ecore_Event_Handler *h;

   D(" ");

   // disconnect all ecore events
   EINA_LIST_FREE(event_handlers, h)
      EDE_EVENT_HANDLER_DEL(h);

   // free all interface components
   EDE_OBJECT_DEL(o_circle);
   EDE_OBJECT_DEL(o_selection);
   EDE_OBJECT_DEL(o_checkboard);
   EDE_OBJECT_DEL(o_layout);
   if (window) ecore_evas_free(window);
   EDE_STRINGSHARE_DEL(theme_file);

   // shoutdown graphic library
   edje_shutdown();
   ecore_evas_shutdown();
   evas_shutdown();

   return EINA_TRUE;
}

/**
 * Get the evas canvas
 */
EAPI Evas *
ede_gui_canvas_get(void)
{
   return canvas;
}

/**
 * Get the full path to the edje theme file
 */
EAPI const char *
ede_gui_theme_get(void)
{
   return theme_file;
}

/**
 * This function actually show the checkboard background at the right size.
 * And create the array for store all the Evas_Object * of the overlays.
 * Also the towers buttons are created according the level requested towers
 */
EAPI Eina_Bool
ede_gui_level_init(int rows, int cols, const char *towers)
{
   char **split;
   int i = 0;

   D("%d %d", rows, cols);
   
   // resize the checkboard
   evas_object_resize(o_checkboard, cols * CELL_W, rows * CELL_H);

   // resize the overlays array
   ede_array_free((int **)overlays);
   overlays = (Evas_Object ***)ede_array_new(rows, cols);
   if (!overlays) return EINA_FALSE;
   checkboard_rows = rows;
   checkboard_cols = cols;

   // add the buttons for the requested towers class
   split = eina_str_split(towers, ",", 0);
   while (split[i])
      ede_gui_tower_button_add(split[i++]);
   free(split[0]);
   free(split);

   return EINA_TRUE;
}

/**
 * This function actually hide the checkboard and free all the objs
 */
EAPI void
ede_gui_level_clear(void)
{
   int row, col;

   D(" ");

   // del all the overlay objs
   for (row = 0; row < checkboard_rows; row++)
      for (col = 0; col < checkboard_cols; col++)
         EDE_OBJECT_DEL(overlays[row][col]);

   // free the overlays array
   ede_array_free((int **)overlays);
   overlays = NULL;

   // hide the checkboard
   evas_object_resize(o_checkboard, 0, 0);
   checkboard_rows = checkboard_cols = 0;
}

/**********   TOWER BUTTONS FUNCS  *******************************************/
/**
 * Add a new button to add a tower
 */
EAPI void
ede_gui_tower_button_add(const char *tower_class_id)
{
   Evas_Object *obj;
   Ede_Tower_Class *tc;
   char buf[PATH_MAX];

   tc = ede_tower_class_get_by_id(tower_class_id);
   if (!tc)
   {
      ERR("Can't find tower class: %s", tower_class_id);
      return;
   }

   // create the image object (button)
   obj = evas_object_image_filled_add(ede_gui_canvas_get());
   snprintf(buf, sizeof(buf), PACKAGE_DATA_DIR"/themes/%s", tc->icon);
   evas_object_image_file_set(obj, buf, NULL);
   evas_object_resize(obj, 30, 30); // TODO fixme, should be themable
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP,
                                  _add_tower_add_button_cb, tc);
   evas_object_show(obj);

   // put the button in the edje box
   edje_object_part_box_append(o_layout, "towers_btn_box", obj);
}

/**
 * Clear the edje box that contain all the tower-add buttons
 */
EAPI void
ede_gui_tower_button_box_clear(void)
{
   edje_object_part_box_remove_all(o_layout, "towers_btn_box", EINA_TRUE);
}

/**********   UI UPDATE FUNCS  ************************************************/
/**
 * Change the tower information box
 */
EAPI void
ede_gui_tower_info_set(const char *name, const char *icon, const char *text)
{
   D(" ");

   if (!name)
   {
      edje_object_part_text_set(o_layout, "tower.name", "");
      edje_object_part_text_set(o_layout, "tower.info", "");
      edje_object_signal_emit(o_layout, "tower,icon,set", "hide");
      return;
   }

   edje_object_part_text_set(o_layout, "tower.name", name);
   edje_object_part_text_set(o_layout, "tower.info", text);
   edje_object_signal_emit(o_layout, "tower,icon,set", icon);
}

EAPI void
ede_gui_wave_info_set(int tot, int cur, int num, const char *type)
{
   char buf[32];

   D(" ");
   snprintf(buf, sizeof(buf), "Wave %d/%d", cur, tot);
   edje_object_part_text_set(o_layout, "waves.title", buf);

   if (num > 0)
   {
      snprintf(buf, sizeof(buf), "%d x %s", num, type);
      edje_object_part_text_set(o_layout, "wave.next.text", buf);
   }
   else
      edje_object_part_text_set(o_layout, "wave.next.text", "");
}

EAPI void
ede_gui_wave_timer_update(int secs)
{
   char buf[32];
   D(" ");

   snprintf(buf, sizeof(buf), "SEND (%d)", secs);
   edje_object_part_text_set(o_layout, "wave.button.text", buf);
}

EAPI void
ede_gui_upgrade_box_set(int pos, const char *name)
{
   char buf[32];
   D(" ");
   
   snprintf(buf, sizeof(buf), "upgrade.%d.name", pos);
   edje_object_part_text_set(o_layout, buf, name);

   snprintf(buf, sizeof(buf), "%d", pos);
   edje_object_signal_emit(o_layout, "upgrade,show", buf);
}

EAPI void
ede_gui_upgrade_box_hide_all(void)
{
   D(" ");

   edje_object_signal_emit(o_layout, "upgrade,hide,all", "");
}

EAPI void
ede_gui_lives_set(int lives)
{
   char buf[16];
   eina_convert_itoa(lives, buf);
   edje_object_part_text_set(o_layout, "lives.icon.text", buf);
}

EAPI void
ede_gui_bucks_set(int bucks)
{
   char buf[16];
   eina_convert_itoa(bucks, buf);
   edje_object_part_text_set(o_layout, "bucks.icon.text", buf);
}

EAPI void
ede_gui_score_set(int score)
{
   char buf[16];
   eina_convert_itoa(score, buf);
   edje_object_part_text_set(o_layout, "score.icon.text", buf);
}

/**********   MAIN MENU   *****************************************************/
EAPI void
ede_gui_menu_show(const char *title)
{
   edje_object_part_text_set(o_menu, "menu.title", title);
   evas_object_show(o_menu);
}

EAPI void
ede_gui_menu_item_add(const char *label1, const char *label2,
                     void (*selected_cb)(void *data), void *data)
{
   Evas_Object *item;
   int w, h;

   item = edje_object_add(canvas);
   edje_object_file_set(item, theme_file, "ede/menu_item");
   edje_object_signal_callback_add(item, "item,selected", "",
                                   _menu_item_selected, item);


   edje_object_part_text_set(item, "label1.text", label1);
   edje_object_part_text_set(item, "label2.text", label2);
   evas_object_data_set(item, "item_cb", selected_cb);
   evas_object_data_set(item, "item_data", data);

   edje_object_size_min_get(item, &w, &h);
   evas_object_resize(item, w, h);
   evas_object_show(item);

   edje_object_part_box_append(o_menu, "menu.box", item);
}

EAPI void
ede_gui_menu_hide(void)
{
   edje_object_part_box_remove_all(o_menu, "menu.box", EINA_TRUE);
   evas_object_hide(o_menu);
}

/**********   LEVEL SELECTOR   ************************************************/
EAPI void
ede_gui_level_selector_show(void)
{
   evas_object_show(o_levelselector);
}

EAPI void
ede_gui_level_selector_item_add(const char *label,
                     void (*selected_cb)(void *data), void *data)
{
   Evas_Object *item;
   int w, h;

   item = edje_object_add(canvas);
   edje_object_file_set(item, theme_file, "ede/menu_item");
   edje_object_signal_callback_add(item, "item,selected", "",
                                   _menu_item_selected, item);


   edje_object_part_text_set(item, "label1.text", label);
   edje_object_part_text_set(item, "label2.text", "");
   evas_object_data_set(item, "item_cb", selected_cb);
   evas_object_data_set(item, "item_data", data);

   edje_object_size_min_get(item, &w, &h);
   evas_object_resize(item, w, h);
   evas_object_show(item);

   edje_object_part_box_append(o_levelselector, "level_selector.box", item);
}

EAPI void
ede_gui_level_selector_hide(void)
{
   edje_object_part_box_remove_all(o_levelselector, "level_selector.box", EINA_TRUE);
   evas_object_hide(o_levelselector);
}

/**********   UTILS   *********************************************************/
/**
 * Get the screen coordinates (x,y) of the given level cell
 * If center is EINA_FALSE that the top-left corner of the cell is returned,
 * else the center point is calculated instead.
 */
EAPI Eina_Bool
ede_gui_cell_coords_get(int row, int col, int *x, int *y, Eina_Bool center)
{
   if (row > checkboard_rows || col > checkboard_cols)
      return EINA_FALSE;

   if (x) *x = (col * CELL_W + STAGE_OFFSET_X) + (center * CELL_W / 2);
   if (y) *y = (row * CELL_H + STAGE_OFFSET_Y) + (center * CELL_H / 2);
   return EINA_TRUE;
}

/**
 * Get the cell (row,col) at the given screen coords (in pixel)
 */
EAPI Eina_Bool
ede_gui_cell_get_at_coords(int x, int y, int *row, int *col)
{
   if (row) *row = (y - STAGE_OFFSET_Y) / CELL_W;
   if (col) *col = (x - STAGE_OFFSET_X) / CELL_H;
   return EINA_TRUE;
}

/*****************  OVERLAY FUNCTIONS  ****************************************/

/**
 * Draw an overlay at the given cell, you can mix up different type of overlay.
 * For example one call to set the border and one call to set the orientation
 * end up in a cell with the give border and the given orientation.
 * Call with OVERLAY_NONE to clear all the cell.
 */
EAPI void
ede_gui_cell_overlay_add(Ede_Cell_Overlay overlay, int row, int col)
{
   Evas_Object *obj;

   //~ D("%d %d [%p]", col, row, overlays[row][col]);

   // if the object is not yet in the array create it
   if (!overlays[row][col])
   {
      obj = edje_object_add(canvas);
      edje_object_file_set(obj, theme_file, "ede/cell_overlay");
      evas_object_pass_events_set(obj, EINA_TRUE);

      _move_at(obj, row, col);
      evas_object_resize(obj, CELL_W, CELL_H);
      evas_object_show(obj);
      overlays[row][col] = obj;
   }

   obj = overlays[row][col];
   switch (overlay)
   {
      case OVERLAY_NONE:
         edje_object_signal_emit(obj, "overlay,set,none", "ede");
         break;
      case OVERLAY_IMAGE_WALL:
         edje_object_signal_emit(obj, "overlay,image,set,wall", "ede");
         break;
      case OVERLAY_BORDER_RED:
         edje_object_signal_emit(obj, "overlay,border,set,red", "ede");
         break;
      case OVERLAY_BORDER_GREEN:
         edje_object_signal_emit(obj, "overlay,border,set,green", "ede");
         break;
      case OVERLAY_BORDER_BLUE:
         edje_object_signal_emit(obj, "overlay,border,set,blue", "ede");
         break;
      case OVERLAY_COLOR_RED:
         edje_object_signal_emit(obj, "overlay,color,set,red", "ede");
         break;
      case OVERLAY_COLOR_GREEN:
         edje_object_signal_emit(obj, "overlay,color,set,green", "ede");
         break;
      case OVERLAY_COLOR_BLUE:
         edje_object_signal_emit(obj, "overlay,color,set,blue", "ede");
         break;
      case OVERLAY_0:
         edje_object_signal_emit(obj, "overlay,direction,set,0", "ede");
         break;
      case OVERLAY_1:
         edje_object_signal_emit(obj, "overlay,direction,set,1", "ede");
         break;
      case OVERLAY_2:
         edje_object_signal_emit(obj, "overlay,direction,set,2", "ede");
         break;
      case OVERLAY_3:
      edje_object_signal_emit(obj, "overlay,direction,set,3", "ede");
         break;
      case OVERLAY_4:
         edje_object_signal_emit(obj, "overlay,direction,set,4", "ede");
         break;
      case OVERLAY_5:
         edje_object_signal_emit(obj, "overlay,direction,set,5", "ede");
         break;
      case OVERLAY_6:
         edje_object_signal_emit(obj, "overlay,direction,set,6", "ede");
         break;
      case OVERLAY_7:
         edje_object_signal_emit(obj, "overlay,direction,set,7", "ede");
         break;
      default:
         break;
   }
}

/**
 * Set the given integer value to the text inside the cell, actually only used
 * to debug the pathfinder.
 * @param row,col Cell location
 * @param val The number to show
 * @param pos Where to show the number (1 = top-left, 2 = bottom-left, 3 = bottom-right)
 */
EAPI void
ede_gui_cell_overlay_text_set(int row, int col, int val, int pos)
{
   char buf[8];
   Evas_Object *obj = overlays[row][col];

   if (!obj) return;

   snprintf(buf, sizeof(buf), "%d", val);
   if (pos == 1) edje_object_part_text_set(obj, "label1", buf);
   else if (pos == 2) edje_object_part_text_set(obj, "label2", buf);
   else if (pos == 3) edje_object_part_text_set(obj, "label3", buf);
}

/*****************  SELECTION OBJECT ******************************************/

EAPI void
ede_gui_selection_show_at(int row, int col, int rows, int cols, int radius)
{
   int x, y, dx, dy;
   D("%d %d %d %d (radius %d)", row, col, rows, cols, radius);

   // selection rect
   _move_at(o_selection, row, col);
   evas_object_resize(o_selection, cols * CELL_W, rows * CELL_H);
   evas_object_show(o_selection);

   if (radius > 0)
   {
      ede_gui_cell_coords_get(row, col, &x, &y, EINA_FALSE);
      ede_gui_cell_coords_get(row + rows, col + cols, &dx, &dy, EINA_FALSE);
      dx = (dx - x) / 2;
      dy = (dy - y) / 2;
      _circle_recalc(o_circle, x + dx, y + dy, radius);
      evas_object_show(o_circle);
   }
   else evas_object_hide(o_circle);
}

EAPI void
ede_gui_selection_type_set(Ede_Selection_Type type)
{
   switch (type)
   {
      case SELECTION_FREE:
         edje_object_signal_emit(o_selection, "set,free", ""); break;
      case SELECTION_WRONG:
         edje_object_signal_emit(o_selection, "set,wrong", ""); break;
      case SELECTION_BLOCKING:
         edje_object_signal_emit(o_selection, "set,blocking", ""); break;
      case SELECTION_TOWER:
         edje_object_signal_emit(o_selection, "set,tower", ""); break;
      case SELECTION_UNKNOW: default:
         break;
   }
}

EAPI void
ede_gui_selection_hide(void)
{
   evas_object_hide(o_selection);
   evas_object_hide(o_circle);
}

/*****************  AREA REQUEST STUFF ****************************************/

static void
_area_request_mouse_move(int x, int y)
{
   int row, col, i, j;

   // hide the selection when mouse is out the checkboard
   if (!_point_inside_checkboard(x, y))
   {
      ede_gui_selection_hide();
      return;
   }

   // check if all the requested cells are walkable
   selection_ok = EINA_TRUE;
   ede_gui_cell_get_at_coords(x, y, &row, &col);
   for (i = 0; i < area_req_cols; i++)
      for (j = 0; j < area_req_rows; j++)
         if (!ede_level_walkable_get(row + j, col + i))
            selection_ok = EINA_FALSE;

   // now check if an enemy is under the requested area
   if (selection_ok)
   {
      Evas_Object *under;
      int sx, sy, w, h;

      ede_gui_cell_coords_get(row, col, &sx, &sy, EINA_FALSE);
      w = area_req_cols * CELL_W;
      h = area_req_rows * CELL_H;
      under = evas_object_top_in_rectangle_get(ede_gui_canvas_get(),
                                               sx,  sy,  w,  h,
                                               EINA_FALSE, EINA_FALSE);
      if (under != o_checkboard)
         selection_ok = EINA_FALSE;
   }

   // make the selection green or red
   ede_gui_selection_type_set(selection_ok ? SELECTION_FREE : SELECTION_WRONG);

   // move the selection at the right place
   ede_gui_selection_show_at(row, col, area_req_rows, area_req_cols, 0);
}

static void
_area_request_mouse_down(int x, int y,
                         Eina_Bool inside_checkboard, Eina_Bool on_a_tower)
{
   Ede_Level *level = ede_level_current_get();
   Eina_List *l;
   int mouse_row, mouse_col;
   int row, col, i, j;

   D(" ");

   // clicked outside the checkboard, stop area-request
   if (!inside_checkboard)
   {
      ede_gui_request_area_end();
      return;
   }

   ede_gui_cell_get_at_coords(x, y, &mouse_row, &mouse_col);

   // clicked on a tower, select it
   if (on_a_tower)
   {
      ede_gui_request_area_end();
      ede_tower_select_at(mouse_row, mouse_col);
      return;
   }

   // selection is in a not valid position
   if (!selection_ok)
   {
      D("Wrong selection");
      return;
   }

   /*
    * ok, all the needed cells are free, now check if we are blocking some
    * possible path. Pathfind from all the starting bases to the home.
    */


   // set all the needed cells to a temporary (unwalkable) value
   for (i = 0; i < area_req_cols; i++)
      for (j = 0; j < area_req_rows; j++)
         cells[mouse_row + j][mouse_col + i] += 100;

   for (i = 0; i < 10; i++) // Base 0..9
   {
      for (l = level->starts[i]; l; l = l->next->next) // all the points for the Base
      {
         row = (int)l->data;
         col = (int)l->next->data;

         // run the pathfinder in 'just_check' mode
         //~ D("Check Start Base%d: %d %d", i, row, col);
         selection_ok = (int)ede_pathfinder(level->rows, level->cols, row, col,
                                            level->home_row, level->home_col,
                                            ede_level_walkable_get, 0, EINA_TRUE);
         if (!selection_ok)
         {
            D("WRONG !!!!");
            goto end_loop;
         }
      }
   }

end_loop:

   // restore all the needed cells values to the original state
   for (i = 0; i < area_req_cols; i++)
      for (j = 0; j < area_req_rows; j++)
         cells[mouse_row + j][mouse_col + i] -= 100;

   if (selection_ok)
   {
      // selection ok, call the selection callback
      if (area_req_done_cb)
         area_req_done_cb(mouse_row, mouse_col, area_req_cols, area_req_rows,
                          area_req_done_data);
   }
   else
   {
      // blocking path, alert the user and stay in selection mode
      ede_gui_selection_type_set(SELECTION_BLOCKING);
   }
}

EAPI void
ede_gui_request_area(int w, int h, void (*done_cb)(int row, int col, int w, int h, void *data), void *data)
{
   D(" ");

   ede_game_state_set(GAME_STATE_AREA_REQUEST);
   area_req_cols = w;
   area_req_rows = h;
   area_req_done_cb = done_cb;
   area_req_done_data = data;
}

EAPI void
ede_gui_request_area_end(void)
{
   D(" ");

   // clear the area request global stuff
   area_req_done_data = NULL;
   area_req_cols = area_req_rows = 0;
   ede_game_state_set(GAME_STATE_PLAYING);
}
