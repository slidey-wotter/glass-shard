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

#include "window.h"

typedef struct sl_window_mutable {
	Window x_window;
	bool started, mapped, fullscreen, maximized;

	sl_window_dimensions saved_dimensions;

	workspace_type workspace;

	struct sl_window_have_protocols have_protocols;
} sl_window_mutable;

void sl_window_swap (sl_window* lhs, sl_window* rhs) {
	sl_window_mutable temp = *(sl_window_mutable*)lhs;
	*(sl_window_mutable*)lhs = *(sl_window_mutable*)rhs;
	*(sl_window_mutable*)rhs = temp;
}
