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

typedef struct sl_window {
	Window const x_window;
	bool started, mapped, fullscreen, maximized;
	sl_window_dimensions saved_dimensions;
	workspace_type workspace;

	char name[64];
	char icon_name[64];

	struct sl_window_have_protocols {
		bool take_focus;
		bool delete_window;
	} const have_protocols;
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
