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

#include <X11/X.h>

#include "window-dimensions.h"
#include "workspace-type.h"

enum { allowed_action_move_bit = 1 << 0, allowed_action_resize_bit = 1 << 1, allowed_action_minimize_bit = 1 << 2, allowed_action_shade_bit = 1 << 3, allowed_action_stick_bit = 1 << 4, allowed_action_maximize_horz_bit = 1 << 5, allowed_action_maximize_vert_bit = 1 << 6, allowed_action_fullscreen_bit = 1 << 7, allowed_action_change_desktop_bit = 1 << 8, allowed_action_close_bit = 1 << 9, allowed_action_above_bit = 1 << 10, allowed_action_below_bit = 1 << 11 };

typedef struct sl_window {
	Window const x_window;
	bool started, mapped, fullscreen, maximized;
	sl_window_dimensions saved_dimensions;
	workspace_type workspace;

	char const name[64];
	char const icon_name[64];

	struct sl_window_have_protocols {
		bool take_focus;
		bool delete_window;
	} const have_protocols;

	char const net_wm_name[64];

	u16 const allowed_actions;
} sl_window;

extern void sl_window_swap (sl_window* lhs, sl_window* rhs);

typedef struct sl_display sl_display;

extern void sl_set_window_name (sl_window* window, sl_display* display);
extern void sl_set_window_icon_name (sl_window* window, sl_display* display);
extern void sl_set_window_normal_hints (sl_window* window, sl_display* display);
extern void sl_set_window_hints (sl_window* window, sl_display* display);
extern void sl_set_window_class (sl_window* window, sl_display* display);
extern void sl_set_window_transient_for (sl_window* window, sl_display* display);
extern void sl_set_window_protocols (sl_window* window, sl_display* display);
extern void sl_set_window_colormap_windows (sl_window* window, sl_display* display);
extern void sl_set_window_client_machine (sl_window* window, sl_display* display);

extern void sl_window_set_net_wm_name (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_visible_name (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_icon_name (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_visible_icon_name (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_desktop (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_window_type (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_state (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_allowed_actions (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_strut (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_strut_partial (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_icon_geometry (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_icon (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_pid (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_handled_icons (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_user_time (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_user_time_window (sl_window* window, sl_display* display);
extern void sl_window_set_net_frame_extents (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_opaque_region (sl_window* window, sl_display* display);
extern void sl_window_set_net_wm_bypass_compositor (sl_window* window, sl_display* display);

extern void sl_window_set_all_properties (sl_window* window, sl_display* display);
