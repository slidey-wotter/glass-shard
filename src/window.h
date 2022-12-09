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

#include "workspace-type.h"

typedef struct sl_window {
	Window const x_window;
	bool started, mapped, fullscreen, maximized;
	i16 saved_position_x, saved_position_y;
	u16 saved_width, saved_height;
	workspace_type workspace;

	struct sl_window_have_protocols {
		bool take_focus;
		bool delete_window;
	} have_protocols;
} sl_window;

extern void sl_window_swap (sl_window* lhs, sl_window* rhs);
