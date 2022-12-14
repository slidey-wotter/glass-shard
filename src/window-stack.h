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

#include "window.h"
#include "workspace-type.h"

struct sl_workspace_vector {
	size_t const* indexes;
	size_t const size;
	size_t const allocated_size;
};

typedef struct sl_window_node {
	sl_window window;
	size_t previous;
	size_t next;
	bool flagged_for_deletion;
} sl_window_node;

typedef struct sl_window_stack {
	struct sl_window_node const* data;
	size_t const size;
	size_t const allocated_size;

	struct sl_workspace_vector const workspace_vector;

	workspace_type const current_workspace;

	size_t const focused_window_index;
} sl_window_stack;

void sl_window_stack_create (sl_window_stack* restrict, size_t size);
void sl_window_stack_delete (sl_window_stack* restrict);
void sl_window_stack_add_window (sl_window_stack* restrict, sl_window* window);
void sl_window_stack_remove_window (sl_window_stack* restrict, size_t index);
void sl_window_stack_add_window_to_current_workspace (sl_window_stack* restrict, size_t index);
void sl_window_stack_remove_window_from_its_workspace (sl_window_stack* restrict, size_t index);
void sl_window_stack_add_workspace (sl_window_stack* restrict);
void sl_window_stack_remove_workspace (sl_window_stack* restrict);
void sl_window_stack_cycle_up (sl_window_stack* restrict);
void sl_window_stack_cycle_down (sl_window_stack* restrict);
void sl_window_stack_cycle_workspace_up (sl_window_stack* restrict);
void sl_window_stack_cycle_workspace_down (sl_window_stack* restrict);
void sl_window_stack_set_raised_window (sl_window_stack* restrict, size_t index);
void sl_window_stack_set_focused_window (sl_window_stack* restrict, size_t index);
void sl_window_stack_set_raised_window_as_focused (sl_window_stack* restrict);
void sl_window_stack_set_current_workspace (sl_window_stack* restrict, workspace_type workspace);

sl_window* sl_window_stack_get_raised_window (sl_window_stack* restrict);
sl_window* sl_window_stack_get_focused_window (sl_window_stack* restrict);
size_t sl_window_stack_get_raised_window_index (sl_window_stack* restrict);
bool sl_window_stack_is_valid_index(size_t);
