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


/* Local subsystem vars */
static Evas_Object ***overlays = NULL; /** 2D dynamic array of Evas_Object pointers.
                                           One for each cell of the grid */

static const char *theme_file; /** full path to the theme file */
static Ecore_Evas *window;     /** window handle */
static Evas *canvas;           /** evas canvas */
static Evas_Object *o_layout;    /** main edje object containing the interface */
static Ecore_Event_Handler *_keyb_handler; /** event handler for key press */

static Evas_Object *o_checkboard; /**< the level background object */
static int checkboard_rows, checkboard_cols; /**< current size of the checkboard */
static Eina_Bool checkboard_click_handled = EINA_FALSE; /** stupid trick to stop the propagatioin of the click */


static Evas_Object *o_selection; /** the object used to select map locations */
static Evas_Object *o_circle; /** Polygon obj used for the selection range */
static int area_req_rows, area_req_cols; /** size of the current area request */
static void (*area_req_done_cb)(int row, int col, int w, int h, void *data); /** function to call on area selection complete */
static void *area_req_done_data; /** user data to pass-back in the area_req_done_cb */
static Eina_Bool selection_ok;   /** true if the selection is in a free position */



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

   evas_object_polygon_points_clear(obj);

   r2 = radius * radius;
   for (x = -radius; x <= radius; x+=2)
   {
      y = (int) (sqrt(r2 - x*x) + 0.5);
      evas_object_polygon_point_add(obj, center_x + x, center_y + y);
   }
   for (x = radius; x > -radius; x-=2)
   {
      y = (int) (sqrt(r2 - x*x) + 0.5);
      evas_object_polygon_point_add(obj, center_x + x, center_y - y);
   }
}

/* Local subsystem callbacks */
static void
_window_delete_req_cq(Ecore_Evas *window)
{
   D(" ");
   ecore_main_loop_quit();
}

static void
_debug_button_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
   ede_game_debug_hook();
   D(" ");
}

static void
_add_tower_button_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
   D("'%s' '%s'", emission, source);
   ede_tower_add(source);
}

static void
_checkboard_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   D(" ");
   if (!checkboard_click_handled)
   {
      ede_gui_selection_hide();
      ede_gui_tower_info_set(NULL, NULL, NULL);
      ede_tower_deselect();
   }
   else checkboard_click_handled = EINA_FALSE;
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

   if (ev->modifiers) return ECORE_CALLBACK_RENEW;

   if (!strcmp(ev->key, "d"))
   {
      D("D [destroy]");
   }
   else if (!strcmp(ev->key, "F12"))
   {
      D("F12: toggle debug panel");
      dpanel_visible = !dpanel_visible;
      if (dpanel_visible)
      {
         edje_object_signal_emit(o_layout, "debug,panel,show", "");
         ede_game_debug_panel_enable(EINA_TRUE);
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
   ecore_evas_callback_delete_request_set(window, _window_delete_req_cq);
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
   edje_object_signal_callback_add(o_layout, "button,pressed", "debug", _debug_button_cb, NULL);
   edje_object_signal_callback_add(o_layout, "tower,add", "*", _add_tower_button_cb, NULL);

   // create the checkboard object
   o_checkboard = edje_object_add(canvas);
   edje_object_file_set(o_checkboard, theme_file, "ede/checkboard");
   _move_at(o_checkboard, 0, 0);
   evas_object_resize(o_checkboard, 0, 0);
   evas_object_show(o_checkboard);
   evas_object_event_callback_add(o_checkboard, EVAS_CALLBACK_MOUSE_DOWN, _checkboard_mouse_down_cb, NULL);
   
   // create the selection object
   o_selection = edje_object_add(canvas);
   edje_object_file_set(o_selection, theme_file, "ede/selection");
   // selection circle
   o_circle = evas_object_polygon_add(canvas);
   evas_object_pass_events_set(o_circle, EINA_TRUE);
   evas_object_color_set(o_circle, 100, 100, 100, 100);
   Evas_Object *clipper; //TODO clip to the checkboard, not the stage.clipper
   clipper = (Evas_Object *)edje_object_part_object_get(o_layout, "stage.clipper");
   evas_object_clip_set(o_circle, clipper);


   // connect keyboard press event
   _keyb_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                           _ecore_event_key_down_cb, NULL);

   return EINA_TRUE;
}

/**
 * Shutdown gui system, close the window and free all the resource
 */
EAPI Eina_Bool
ede_gui_shutdown(void)
{
   D(" ");

   // disconnect keyboard events
   ecore_event_handler_del(_keyb_handler);
   
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

EAPI Evas *
ede_gui_canvas_get(void)
{
   return canvas;
}

EAPI const char *
ede_gui_theme_get(void)
{
   return theme_file;
}

/**
 * This function actually show the checkboard background at the right size.
 * And create the array for store all the Evas_Object * of the overlays
 */
EAPI Eina_Bool
ede_gui_level_init(int rows, int cols)
{
   D("%d %d", rows, cols);
   evas_object_resize(o_checkboard, cols * CELL_W, rows * CELL_H);

   ede_array_free((int **)overlays);
   overlays = (Evas_Object ***)ede_array_new(rows, cols);
   if (!overlays) return EINA_FALSE;
   checkboard_rows = rows;
   checkboard_cols = cols;
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

/**
 * Get the screen coordinates (x,y) of the given level cell
 * If center is EINA_FALSE that the top-left corner of the cell is returned,
 * else the center point is calculated instead.
 */
EAPI Eina_Bool
ede_gui_cell_coords_get(int row, int col, int *x, int *y, Eina_Bool center)
{
   // TODO GET THE OFFSET FROM THE THEME
   int offset_x = 180;
   int offset_y = 60;

   if (row > checkboard_rows || col > checkboard_cols)
      return EINA_FALSE;
   
   if (x) *x = (col * CELL_W + offset_x) + (center * CELL_W / 2);
   if (y) *y = (row * CELL_H + offset_y) + (center * CELL_H / 2);
   return EINA_TRUE;
}


/**
 * Get the cell (row,col) at the given screen coords (in pixel) 
 */
EAPI Eina_Bool
ede_gui_cell_get_at_coords(int x, int y, int *row, int *col)
{
   // TODO GET THE OFFSET FROM THE THEME
   int offset_x = 180;
   int offset_y = 60;

   if (row) *row = (y - offset_y) / CELL_W;
   if (col) *col = (x - offset_x) / CELL_H;
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

   D("%d %d [%p]", col, row, overlays[row][col]);

   // if the object is not yet in the array create it
   if (!overlays[row][col])
   {
      obj = edje_object_add(canvas);
      edje_object_file_set(obj, theme_file, "ede/cell_overlay");
   
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
   D("%d %d %d %d", row, col, rows, cols);
   evas_object_raise(o_selection);

   // selection rect
   _move_at(o_selection, row, col);
   evas_object_resize(o_selection, cols * CELL_W, rows * CELL_H);
   evas_object_show(o_selection);

   if (radius)
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

/*****************  FREE AREA REQUEST  ****************************************/

static void
_sel_mouse_out_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   evas_object_hide(o_selection);
}

static void
_sel_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   int row = 0, col = 0, i, j;

   // get cell under the mouse
   if (ede_gui_cell_get_at_coords(ev->cur.canvas.x, ev->cur.canvas.y, &row, &col))
   {
      // check if all the cells needed are walkable
      selection_ok = EINA_TRUE;
      for (i = 0; i < area_req_cols; i++)
         for (j = 0; j < area_req_rows; j++)
            if (!ede_level_walkable_get(row + j, col + i))
               selection_ok = EINA_FALSE;

      // make the selection green or red
      ede_gui_selection_type_set(selection_ok ? SELECTION_FREE : SELECTION_WRONG);

      // move the selection at the right place
      ede_gui_selection_show_at(row, col, area_req_rows, area_req_cols, 0);
   }
   else ede_gui_selection_hide();

}

static void
_sel_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Ede_Level *level;
   Eina_List *l;
   int mouse_row, mouse_col;
   int row, col, i, j;

   D(" ");

   if (!selection_ok)
   {
      D("Wrong selection");
      return;
   }

   // stupid trick to stop the propagation of this event in the _checkboard_mouse_down_cb() callback
   checkboard_click_handled = EINA_TRUE;

   // all the needed cells are free, now check if we are blocking all the
   // possible path. Pathfind from all the starting base to the home.

   level = ede_level_current_get();
   ede_gui_cell_get_at_coords(ev->canvas.x, ev->canvas.y, &mouse_row, &mouse_col);

   // set all the needed cells to a temporary (unwalkable) value   
   for (i = 0; i < area_req_cols; i++)
      for (j = 0; j < area_req_rows; j++)
         level->cells[mouse_row + j][mouse_col + i] += 100;
   
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
         level->cells[mouse_row + j][mouse_col + i] -= 100;

   if (selection_ok)
   {
      // selection complete, call the selection callback
      if (area_req_done_cb)
         area_req_done_cb(mouse_row, mouse_col, area_req_cols, area_req_rows, area_req_done_data);

      // clear the selection stuff
      evas_object_event_callback_del(obj,EVAS_CALLBACK_MOUSE_MOVE,_sel_mouse_move_cb);
      evas_object_event_callback_del(obj,EVAS_CALLBACK_MOUSE_OUT,_sel_mouse_out_cb);
      evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_DOWN, _sel_mouse_down_cb);
      area_req_done_cb = NULL;
      area_req_done_data = NULL;
      area_req_cols = area_req_rows = 0;
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

   //~ evas_object_raise(o_selection);
   //~ evas_object_resize(o_selection, w * CELL_W, h * CELL_H);
   area_req_cols = w;
   area_req_rows = h;
   area_req_done_cb = done_cb;
   area_req_done_data = data;
   evas_object_event_callback_add(o_checkboard, EVAS_CALLBACK_MOUSE_MOVE, _sel_mouse_move_cb, NULL);
   evas_object_event_callback_add(o_checkboard, EVAS_CALLBACK_MOUSE_OUT, _sel_mouse_out_cb, NULL);
   evas_object_event_callback_add(o_checkboard, EVAS_CALLBACK_MOUSE_DOWN, _sel_mouse_down_cb, NULL);
}
