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

#include "workspace-type.h"

#define M_invalid_index ((size_t)-1)

struct sl_workspace_vector {
	size_t const* indexes;
	size_t const size;
	size_t const allocated_size;
};

struct sl_window_node; // foward declaration

typedef struct sl_window_stack {
	struct sl_window_node const* data;
	size_t const size;
	size_t const allocated_size;

	struct sl_workspace_vector workspace_vector;

	workspace_type const current_workspace;
} sl_window_stack;
