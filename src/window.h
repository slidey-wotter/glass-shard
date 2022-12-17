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

#define window_started_bit                       0x0000000000000001
#define window_hints_input_bit                   0x0000000000000002
#define window_hints_urgent_bit                  0x0000000000000004
#define window_protocols_take_focus_bit          0x0000000000000008
#define window_protocols_delete_window_bit       0x0000000000000010
#define window_type_desktop_bit                  0x0000000000000020
#define window_type_dock_bit                     0x0000000000000040
#define window_type_toolbar_bit                  0x0000000000000080
#define window_type_menu_bit                     0x0000000000000100
#define window_type_utility_bit                  0x0000000000000200
#define window_type_splash_bit                   0x0000000000000400
#define window_type_dialog_bit                   0x0000000000000800
#define window_type_dropdown_menu_bit            0x0000000000001000
#define window_type_popup_menu_bit               0x0000000000002000
#define window_type_tooltip_bit                  0x0000000000004000
#define window_type_notification_bit             0x0000000000008000
#define window_type_combo_bit                    0x0000000000010000
#define window_type_dnd_bit                      0x0000000000020000
#define window_type_normal_bit                   0x0000000000040000
#define window_all_types                         0x000000000007ffe0
#define window_state_normal_bit                  0x0000000000080000
#define window_state_iconified_bit               0x0000000000100000
#define window_state_modal_bit                   0x0000000000200000
#define window_state_sticky_bit                  0x0000000000400000
#define window_state_maximized_vert_bit          0x0000000000800000
#define window_state_maximized_horz_bit          0x0000000001000000
#define window_state_shaded_bit                  0x0000000002000000
#define window_state_skip_taskbar_bit            0x0000000004000000
#define window_state_skip_pager_bit              0x0000000008000000
#define window_state_hidden_bit                  0x0000000010000000
#define window_state_fullscreen_bit              0x0000000020000000
#define window_state_above_bit                   0x0000000040000000
#define window_state_below_bit                   0x0000000080000000
#define window_state_demands_attention_bit       0x0000000100000000
#define window_state_focused_bit                 0x0000000200000000
#define window_all_states                        0x00000003fff80000
#define window_allowed_action_move_bit           0x0000000400000000
#define window_allowed_action_resize_bit         0x0000000800000000
#define window_allowed_action_minimize_bit       0x0000001000000000
#define window_allowed_action_shade_bit          0x0000002000000000
#define window_allowed_action_stick_bit          0x0000004000000000
#define window_allowed_action_maximize_horz_bit  0x0000008000000000
#define window_allowed_action_maximize_vert_bit  0x0000010000000000
#define window_allowed_action_fullscreen_bit     0x0000020000000000
#define window_allowed_action_change_desktop_bit 0x0000040000000000
#define window_allowed_action_close_bit          0x0000080000000000
#define window_allowed_action_above_bit          0x0000100000000000
#define window_allowed_action_below_bit          0x0000200000000000
#define window_all_allowed_actions               0x00003ffb00000000
#define window_all_flags                         0x00003fffffffffff

struct sl_sized_string {
	char const* data;
	size_t const size;
};

typedef struct sl_window {
	Window const x_window;
	u64 flags;
	sl_window_dimensions dimensions;
	sl_window_dimensions saved_dimensions;

	struct sl_sized_string const name;
	struct sl_sized_string const icon_name;

	struct {
		u16 min_width;
		u16 min_height;
		u16 max_width;
		u16 max_height;
		u16 width_inc;
		u16 height_inc;

		struct {
			u16 numerator;
			u16 denominator;
		} min_aspect, max_aspect;

		u16 base_width;
		u16 base_height;
		u16 gravity;
	} const normal_hints;

	struct sl_sized_string const net_wm_name;
	struct sl_sized_string const net_wm_visible_name;
	struct sl_sized_string const net_wm_icon_name;
	struct sl_sized_string const net_wm_visible_icon_name;
} sl_window;

extern void sl_window_destroy (sl_window* window);

typedef struct sl_display sl_display;

extern void sl_set_window_name (sl_window* restrict, sl_display* restrict);
extern void sl_set_window_icon_name (sl_window* restrict, sl_display* restrict);
extern void sl_set_window_normal_hints (sl_window* restrict, sl_display* restrict);
extern void sl_set_window_hints (sl_window* restrict, sl_display* restrict);
extern void sl_set_window_class (sl_window* restrict, sl_display* restrict);
extern void sl_set_window_transient_for (sl_window* restrict, sl_display* restrict);
extern void sl_set_window_protocols (sl_window* restrict, sl_display* restrict);
extern void sl_set_window_colormap_windows (sl_window* restrict, sl_display* restrict);
extern void sl_set_window_client_machine (sl_window* restrict, sl_display* restrict);

extern void sl_window_set_net_wm_name (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_visible_name (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_icon_name (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_visible_icon_name (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_desktop (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_window_type (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_state (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_allowed_actions (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_strut (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_strut_partial (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_icon_geometry (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_icon (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_pid (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_handled_icons (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_user_time (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_user_time_window (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_frame_extents (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_opaque_region (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_net_wm_bypass_compositor (sl_window* restrict, sl_display* restrict);

extern void sl_window_set_all_properties (sl_window* restrict, sl_display* restrict);

extern void sl_window_set_withdrawn (sl_window* restrict);
extern void sl_window_set_normal (sl_window* restrict);
extern void sl_window_set_iconified (sl_window* restrict);
extern void sl_window_set_fullscreen (sl_window* restrict, sl_display* restrict, bool);
extern void sl_window_toggle_fullscreen (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_horizontally_maximized (sl_window* restrict, sl_display* restrict, bool);
extern void sl_window_toggle_horizontally_maximized (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_vertically_maximized (sl_window* restrict, sl_display* restrict, bool);
extern void sl_window_toggle_vertically_maximized (sl_window* restrict, sl_display* restrict);
extern void sl_window_set_maximized (sl_window* restrict, sl_display* restrict, bool);
extern void sl_window_toggle_maximized (sl_window* restrict, sl_display* restrict);
