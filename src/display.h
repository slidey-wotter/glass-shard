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
#include "vector.h"
#include "window-dimensions.h"
#include "workspace-type.h"

enum {
	wm_protocols,
	wm_colormap_windows,
	wm_delete_window,
	wm_take_focus,
	net_wm_name,
	net_wm_icon_name,
	net_wm_desktop,
	net_wm_window_type,
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
	net_wm_strut,
	net_wm_strut_partial,
	net_wm_icon_geometry,
	net_wm_icon,
	net_wm_pid,
	net_wm_handled_icons,
	net_wm_user_time,
	net_wm_user_time_window,
	net_wm_opaque_region,
	net_wm_bypass_compositor,
	net_supported,
	net_active_window,
	atoms_size
};

static char* const atoms_string_list[] = {
"WM_PROTOCOLS",
"WM_COLORMAP_WINDOWS",
"WM_DELETE_WINDOW",
"WM_TAKE_FOCUS",
"_NET_WM_NAME",
"_NET_WM_ICON_NAME",
"_NET_WM_DESKTOP",
"_NET_WM_WINDOW_TYPE",
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
"_NET_WM_STRUT",
"_NET_WM_STRUT_PARTIAL",
"_NET_WM_ICON_GEOMETRY",
"_MET_WM_ICON",
"_NET_WM_PID",
"_NET_WM_HANDLED_ICONS",
"_NET_WM_USER_TIME",
"_NET_WM_USER_TIME_WINDOW",
"_NET_WM_OPAQUE_REGION",
"_NET_WM_BYPASS_COMPOSITOR",
"_NET_SUPPORTED",
"_NET_ACTIVE_WINDOW"};

#define M_invalid_window_index (size_t) - 1

#define is_valid_window_index(M_index) (M_index != M_invalid_window_index)

typedef struct sl_display {
	Display* const x_display;
	Window const root;
	Cursor const cursor;
	sl_vector* const windows;
	sl_vector* const unmanaged_windows;
	Atom const atoms[atoms_size];
	sl_window_dimensions const dimensions;
	size_t focused_window_index, raised_window_index;
	uint numlockmask;

	struct sl_u32_position {
		u32 x;
		u32 y;
	} mouse;

	workspace_type current_workspace, workspaces_size;
	bool user_input_since_last_workspace_change;
} sl_display;

typedef struct sl_window sl_window; // foward declaration

extern sl_display* sl_display_create (Display* x_display);
extern void sl_display_delete (sl_display* display);

extern void sl_notify_supported_atom (sl_display* display, i8 index);
extern void sl_grab_keys (sl_display* display);

extern sl_window* sl_focused_window (sl_display* display);
extern sl_window* sl_raised_window (sl_display* display);
extern sl_window* sl_window_at (sl_display* display, size_t index);
extern sl_window* sl_unmanaged_window_at (sl_display* display, size_t index);

extern void sl_cycle_windows_up (sl_display* display, Time time);
extern void sl_cycle_windows_down (sl_display* display, Time time);

extern void sl_push_workspace (sl_display* display);
extern void sl_pop_workspace (sl_display* display, Time time);

extern void sl_next_workspace (sl_display* display, Time time);
extern void sl_previous_workspace (sl_display* display, Time time);
extern void sl_switch_to_workspace (sl_display* display, size_t index, Time time);
extern void sl_next_workspace_with_raised_window (sl_display* display, Time time);
extern void sl_previous_workspace_with_raised_window (sl_display* display, Time time);

extern void sl_map_windows_for_current_workspace (sl_display* display);
extern void sl_map_windows_for_next_workspace (sl_display* display);
extern void sl_unmap_windows_for_current_workspace (sl_display* display);
extern void sl_map_windows_for_current_workspace_except_raised_window (sl_display* display);
extern void sl_unmap_windows_for_current_workspace_except_raised_window (sl_display* display);

extern void sl_focus_window (sl_display* display, size_t index, Time time);
extern void sl_raise_window (sl_display* display, size_t index, Time time);
extern void sl_focus_and_raise_window (sl_display* display, size_t index, Time time);
extern void sl_focus_raised_window (sl_display* display, Time time);
extern void sl_swap_window_with_raised_window (sl_display* display, size_t index, Time time);

extern void sl_focus_and_raise_unmanaged_window (sl_display* display, size_t index, Time time);

extern void sl_move_window (sl_display* display, sl_window* window, i16 x, i16 y);
extern void sl_resize_window (sl_display* display, sl_window* window, u16 width, u16 height);
extern void sl_move_and_resize_window (sl_display* display, sl_window* window, sl_window_dimensions dimensions);
extern void sl_configure_window (sl_display* display, sl_window* window, uint value_mask, XWindowChanges window_changes);

extern void sl_maximize_raised_window (sl_display* display);
extern void sl_expand_raised_window_to_max (sl_display* display);
extern void sl_close_raised_window (sl_display* display, Time time);

extern void sl_delete_all_windows (sl_display* display, Time time);
extern void sl_delete_window (sl_display* display, size_t index, Time time);
extern void sl_delete_raised_window (sl_display* display, Time time);
extern void sl_window_erase (sl_display* display, size_t index, Time time);
extern void sl_raised_window_erase (sl_display* display, Time time);
