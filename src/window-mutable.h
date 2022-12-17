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

#include "types.h"
#include "window-dimensions.h"
#include "workspace-type.h"

struct sl_sized_string_mutable {
	char* data;
	size_t size;
};

typedef struct sl_window_mutable {
	Window x_window;
	u64 flags;
	sl_window_dimensions dimensions;
	sl_window_dimensions saved_dimensions;

	struct sl_sized_string_mutable name;
	struct sl_sized_string_mutable icon_name;

	struct window_normal_hints {
		u16 min_width;
		u16 min_height;
		u16 max_width;
		u16 max_height;
		u16 width_inc;
		u16 height_inc;

		struct window_normal_hints_aspect {
			u16 numerator;
			u16 denominator;
		} min_aspect, max_aspect;

		u16 base_width;
		u16 base_height;
		u16 gravity;
	} normal_hints;

	struct sl_sized_string_mutable net_wm_name;
	struct sl_sized_string_mutable net_wm_visible_name;
	struct sl_sized_string_mutable net_wm_icon_name;
	struct sl_sized_string_mutable net_wm_visible_icon_name;
} sl_window_mutable;
