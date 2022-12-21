/*
  glass shard, a window manager for X11
  Copyright (C) 2022, David Cardoso <slidey-wotter.tumblr.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <X11/Xlib.h>

#include "message.h"
#include "window-dimensions.h"
#include "window-stack.h"
#include "workspace-type.h"

enum {
	type_utf8_string,
	wm_protocols,
	wm_colormap_windows,
	wm_take_focus,
	wm_delete_window,
	wm_change_state,
	net_supported,
	net_wm_ping,
	net_wm_sync_request,
	net_wm_fullscreen_monitors,
	net_wm_name,
	net_wm_visible_name,
	net_wm_icon_name,
	net_wm_visible_icon_name,
	net_wm_desktop,
	net_wm_window_type,
	net_wm_window_type_desktop,
	net_wm_window_type_dock,
	net_wm_window_type_toolbar,
	net_wm_window_type_menu,
	net_wm_window_type_utility,
	net_wm_window_type_splash,
	net_wm_window_type_dialog,
	net_wm_window_type_dropdown_menu,
	net_wm_window_type_popup_menu,
	net_wm_window_type_tooltip,
	net_wm_window_type_notification,
	net_wm_window_type_combo,
	net_wm_window_type_dnd,
	net_wm_window_type_normal,
	net_wm_state,
	net_wm_state_modal,
	net_wm_state_sticky,
	net_wm_state_maximized_vert,
	net_wm_state_maximized_horz,
	net_wm_state_shaded,
	net_wm_state_skip_taskbar,
	net_wm_state_skip_pager,
	net_wm_state_hidden,
	net_wm_state_fullscreen,
	net_wm_state_above,
	net_wm_state_below,
	net_wm_state_demands_attention,
	net_wm_state_focused,
	net_wm_allowed_actions,
	net_wm_action_move,
	net_wm_action_resize,
	net_wm_action_minimize,
	net_wm_action_shade,
	net_wm_action_stick,
	net_wm_action_maximize_horz,
	net_wm_action_maximize_vert,
	net_wm_action_fullscreen,
	net_wm_action_change_desktop,
	net_wm_action_close,
	net_wm_action_above,
	net_wm_action_below,
	net_wm_strut,
	net_wm_strut_partial,
	net_wm_icon_geometry,
	net_wm_icon,
	net_wm_pid,
	net_wm_handled_icons,
	net_wm_user_time,
	net_wm_user_time_window,
	net_frame_extents,
	net_wm_opaque_region,
	net_wm_bypass_compositor,
	atoms_size
};

static char* const atoms_string_list[] = {
"UTF8_STRING",
"WM_PROTOCOLS",
"WM_COLORMAP_WINDOWS",
"WM_TAKE_FOCUS",
"WM_DELETE_WINDOW",
"WM_CHANGE_STATE",
"_NET_SUPPORTED",
"_NET_WM_PING",
"_NET_WM_SYNC_REQUEST",
"_NET_WM_FULLSCREEN_MONITORS",
"_NET_WM_NAME",
"_NET_WM_VISIBLE_NAME",
"_NET_WM_ICON_NAME",
"_NET_WM_VISIBLE_ICON_NAME",
"_NET_WM_DESKTOP",
"_NET_WM_WINDOW_TYPE",
"_NET_WM_WINDOW_TYPE_DESKTOP",
"_NET_WM_WINDOW_TYPE_DOCK",
"_NET_WM_WINDOW_TYPE_TOOLBAR",
"_NET_WM_WINDOW_TYPE_MENU",
"_NET_WM_WINDOW_TYPE_UTILITY",
"_NET_WM_WINDOW_TYPE_SPLASH",
"_NET_WM_WINDOW_TYPE_DIALOG",
"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
"_NET_WM_WINDOW_TYPE_POPUP_MENU",
"_NET_WM_WINDOW_TYPE_TOOLTIP",
"_NET_WM_WINDOW_TYPE_NOTIFICATION",
"_NET_WM_WINDOW_TYPE_COMBO",
"_NET_WM_WINDOW_TYPE_DND",
"_NET_WM_WINDOW_TYPE_NORMAL",
"_NET_WM_STATE",
"_NET_WM_STATE_MODAL",
"_NET_WM_STATE_STICKY",
"_NET_WM_STATE_MAXIMIZED_VERT",
"_NET_WM_STATE_MAXIMIZED_HORZ",
"_NET_WM_STATE_SHADED",
"_NET_WM_STATE_SKIP_TASKBAR",
"_NET_WM_STATE_SKIP_PAGER",
"_NET_WM_STATE_HIDDEN",
"_NET_WM_STATE_FULLSCREEN",
"_NET_WM_STATE_ABOVE",
"_NET_WM_STATE_BELOW",
"_NET_WM_STATE_DEMANDS_ATTENTION",
"_NET_WM_STATE_FOCUSED",
"_NET_WM_ALLOWED_ACTIONS",
"_NET_WM_ACTION_MOVE",
"_NET_WM_ACTION_RESIZE",
"_NET_WM_ACTION_MINIMIZE",
"_NET_WM_ACTION_SHADE",
"_NET_WM_ACTION_STICK",
"_NET_WM_ACTION_MAXIMIZE_HORZ",
"_NET_WM_ACTION_MAXIMIZE_VERT",
"_NET_WM_ACTION_FULLSCREEN",
"_NET_WM_ACTION_CHANGE_DESKTOP",
"_NET_WM_ACTION_CLOSE",
"_NET_WM_ACTION_ABOVE",
"_NET_WM_ACTION_BELOW",
"_NET_WM_STRUT",
"_NET_WM_STRUT_PARTIAL",
"_NET_WM_ICON_GEOMETRY",
"_MET_WM_ICON",
"_NET_WM_PID",
"_NET_WM_HANDLED_ICONS",
"_NET_WM_USER_TIME",
"_NET_WM_USER_TIME_WINDOW",
"_NET_FRAME_EXTENTS",
"_NET_WM_OPAQUE_REGION",
"_NET_WM_BYPASS_COMPOSITOR"};

#define M_net_wm_state_remove 0
#define M_net_wm_state_add    1
#define M_net_wm_state_toggle 2

typedef struct sl_display {
	Display* const x_display;
	Window const root;
	Cursor const cursor;
	sl_window_stack const window_stack;
	Atom const atoms[atoms_size];
	sl_window_dimensions const dimensions;

	uint numlockmask;

	struct sl_u32_position {
		u32 x;
		u32 y;
	} mouse;
} sl_display;

typedef struct sl_window sl_window; // foward declaration

extern sl_display* sl_display_create (Display* restrict);
extern void sl_display_delete (sl_display* restrict);

extern void sl_grab_keys (sl_display* restrict);

extern void sl_cycle_windows_up (sl_display* restrict, Time);
extern void sl_cycle_windows_down (sl_display* restrict, Time);
extern void sl_next_workspace (sl_display* restrict, Time);
extern void sl_previous_workspace (sl_display* restrict, Time);
extern void sl_switch_to_workspace (sl_display* restrict, workspace_type, Time);
extern void sl_next_workspace_with_raised_window (sl_display* restrict);
extern void sl_previous_workspace_with_raised_window (sl_display* restrict);
extern void sl_push_workspace (sl_display* restrict);
extern void sl_pop_workspace (sl_display* restrict, Time);

extern void sl_focus_window (sl_display* restrict, size_t, Time);
extern void sl_raise_window (sl_display* restrict, size_t);
extern void sl_focus_and_raise_window (sl_display* restrict, size_t, Time);
extern void sl_focus_raised_window (sl_display* restrict, Time);

extern void sl_move_window (sl_display* restrict, sl_window* restrict, i16 x, i16 y);
extern void sl_resize_window (sl_display* restrict, sl_window* restrict, u16 width, u16 height);
extern void sl_move_and_resize_window (sl_display* restrict, sl_window* restrict, sl_window_dimensions);
extern void sl_window_fullscreen_change_response (sl_display* restrict, sl_window* restrict);
extern void sl_window_maximized_change_response (sl_display* restrict, sl_window* restrict);

extern void sl_maximize_raised_window (sl_display* restrict);
extern void sl_expand_raised_window_to_max (sl_display* restrict);
extern void sl_close_raised_window (sl_display* restrict, Time);

extern void sl_delete_window (sl_display* restrict, size_t, Time);
extern void sl_delete_raised_window (sl_display* restrict, Time);
extern void sl_delete_all_windows (sl_display* restrict, Time);
