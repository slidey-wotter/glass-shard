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

#include "window-stack.h"

#include <stdlib.h>
#include <string.h>

#include "message.h"
#include "window-mutable.h"
#include "window.h"

#define max(a, b) ((a > b) ? a : b)

#define M_smallest_nonzero_size 4

typedef struct sl_workspace_vector sl_workspace_vector;

typedef struct sl_workspace_vector_mutable {
	size_t* indexes;
	size_t size;
	size_t allocated_size;
} sl_workspace_vector_mutable;

typedef struct sl_window_node {
	sl_window window;
	size_t previous;
	size_t next;
	bool flagged_for_deletion;
} sl_window_node;

typedef struct sl_window_node_mutable {
	sl_window_mutable window;
	size_t previous;
	size_t next;
	bool flagged_for_deletion;
} sl_window_node_mutable;

typedef struct sl_window_stack_mutable {
	struct sl_window_node_mutable* data;
	size_t size;
	size_t allocated_size;

	struct sl_workspace_vector_mutable workspace_vector;

	workspace_type current_workspace;

	size_t focused_window_index;
} sl_window_stack_mutable;

static void workspace_vector_initialize (size_t* restrict indexes, size_t size) {
	for (size_t i = 0; i < size; ++i)
		indexes[i] = M_invalid_index;
}

static void workspace_vector_create (sl_workspace_vector* restrict this, size_t size) {
	if (size == 0) {
		*(sl_workspace_vector_mutable*)this = (sl_workspace_vector_mutable) {};

		return;
	}

	size_t allocated_size = max(size, M_smallest_nonzero_size);

	size_t* indexes = malloc(sizeof(size_t) * allocated_size);

	if (!indexes) {
		warn_log_va("size of %lu is invalid", allocated_size);

		return;
	}

	workspace_vector_initialize(indexes, size);

	*(sl_workspace_vector_mutable*)this = (sl_workspace_vector_mutable) {.indexes = indexes, .size = size, .allocated_size = allocated_size};
}

static void workspace_vector_delete (sl_workspace_vector* restrict this) {
	if (this->indexes) free(((sl_workspace_vector_mutable*)this)->indexes);
}

static void workspace_vector_set_new_allocated_size (sl_workspace_vector* restrict this, size_t allocated_size) {
	size_t* new_indexes = malloc(sizeof(size_t) * allocated_size);

	if (!new_indexes) {
		warn_log_va("size of %lu is invalid", allocated_size);

		return;
	}

	memmove(new_indexes, this->indexes, this->size);

	*(sl_workspace_vector_mutable*)this = (sl_workspace_vector_mutable) {.indexes = new_indexes, .size = this->size, .allocated_size = allocated_size};
}

static void workspace_vector_ensure_capacity (sl_workspace_vector* restrict this, size_t size) {
	if (this->allocated_size >= size) return;

	size_t allocated_size = this->allocated_size;
	while (allocated_size < size)
		allocated_size <<= 1;

	workspace_vector_set_new_allocated_size(this, allocated_size);
}

static void workspace_vector_push (sl_workspace_vector* restrict this) {
	workspace_vector_ensure_capacity(this, this->size);

	((sl_workspace_vector_mutable*)this)->indexes[this->size] = M_invalid_index;
	++((sl_workspace_vector_mutable*)this)->size;
}

static void workspace_vector_pop (sl_workspace_vector* restrict this) {
	--((sl_workspace_vector_mutable*)this)->size;

	if (this->size > this->allocated_size >> 1) return;

	size_t allocated_size = this->allocated_size;
	while (allocated_size >> 1 > this->size)
		allocated_size >>= 1;

	workspace_vector_set_new_allocated_size(this, allocated_size);
}

void sl_window_stack_create (sl_window_stack* restrict this, size_t size) {
	if (size == 0) {
		*(sl_window_stack_mutable*)this = (sl_window_stack_mutable) {};

		return;
	}

	size_t allocated_size = max(size, M_smallest_nonzero_size);

	sl_window_node* data = malloc(sizeof(sl_window_node) * allocated_size);

	if (!data) {
		warn_log_va("size of %lu is invalid", allocated_size);

		return;
	}

	*(sl_window_stack_mutable*)this = (sl_window_stack_mutable
	) {.data = (sl_window_node_mutable*)data, .size = size, .allocated_size = allocated_size, .focused_window_index = M_invalid_index};

	workspace_vector_create(&this->workspace_vector, 1);
}

void sl_window_stack_delete (sl_window_stack* restrict this) {
	if (this->data) free(((sl_window_stack_mutable*)this)->data);

	workspace_vector_delete(&this->workspace_vector);
}

static void window_stack_ensure_capacity_plus_one (sl_window_stack* restrict this) {
	if (this->allocated_size >= this->size) return;

	size_t new_size = 0;
	for (size_t i = 0; i < this->size; ++i) {
		if (!this->data[i].flagged_for_deletion) ++new_size;
	}

	size_t allocated_size = this->allocated_size;
	while (allocated_size < new_size + 1)
		allocated_size <<= 1;
	while (allocated_size >> 1 > new_size + 1)
		allocated_size >>= 1;

	sl_window_node_mutable* new_data = malloc(sizeof(size_t) * allocated_size);

	if (!new_data) {
		warn_log_va("size of %lu is invalid", allocated_size);

		return;
	}

	for (size_t i = 0, j = 0; i < this->size; ++i) {
		if (this->data[i].flagged_for_deletion) {
			if (this->focused_window_index == i) ((sl_window_stack_mutable*)this)->focused_window_index = M_invalid_index;

			continue;
		}

		if (this->focused_window_index == i) ((sl_window_stack_mutable*)this)->focused_window_index = j;

		new_data[j] = *(sl_window_node_mutable*)&this->data[i];
	}

	((sl_window_stack_mutable*)this)->data = new_data;
	((sl_window_stack_mutable*)this)->size = new_size;
	((sl_window_stack_mutable*)this)->allocated_size = allocated_size;
}

void sl_window_stack_add_window (sl_window_stack* restrict this, sl_window window) {
	workspace_vector_ensure_capacity(&this->workspace_vector, 1);

	window_stack_ensure_capacity_plus_one(this);

	if (this->workspace_vector.indexes[this->current_workspace] == M_invalid_index) {
		((sl_window_stack_mutable*)this)->data[this->size] =
		(sl_window_node_mutable) {*(sl_window_mutable*)&window, .next = M_invalid_index, .previous = M_invalid_index};

		++((sl_window_stack_mutable*)this)->size;

		return;
	}

	if (this->data[this->workspace_vector.indexes[this->current_workspace]].next == M_invalid_index) {
		// note: we can assume both next and previous are invalid

		((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->current_workspace]].next = this->size;
		((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->current_workspace]].previous = this->size;

		((sl_window_stack_mutable*)this)->data[this->size] = (sl_window_node_mutable) {
		*(sl_window_mutable*)&window, .next = this->workspace_vector.indexes[this->current_workspace],
		.previous = this->workspace_vector.indexes[this->current_workspace]};

		++((sl_window_stack_mutable*)this)->size;

		return;
	}

	size_t old_next = this->data[this->workspace_vector.indexes[this->current_workspace]].next;

	((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->current_workspace]].next = this->size;
	((sl_window_stack_mutable*)this)->data[old_next].previous = this->size;

	((sl_window_stack_mutable*)this)->data[this->size] = (struct sl_window_node_mutable
	) {*(sl_window_mutable*)&window, .next = old_next, .previous = this->workspace_vector.indexes[this->current_workspace]};

	++((sl_window_stack_mutable*)this)->size;
}

void sl_window_stack_remove_window (sl_window_stack* restrict this, size_t index) {
	((sl_window_stack_mutable*)this)->data[index].flagged_for_deletion = true;

	if (this->data[index].previous == M_invalid_index) {
		// note: we can assume both next and previous are invalid

		if (this->workspace_vector.indexes[this->current_workspace] == index)
			((sl_window_stack_mutable*)this)->workspace_vector.indexes[this->current_workspace] = M_invalid_index;

		return;
	}

	if (this->workspace_vector.indexes[this->current_workspace] == index)
		((sl_window_stack_mutable*)this)->workspace_vector.indexes[this->current_workspace] = this->data[index].previous;

	((sl_window_stack_mutable*)this)->data[this->data[index].next].previous = this->data[index].previous;
	((sl_window_stack_mutable*)this)->data[this->data[index].previous].next = this->data[index].next;
}

void sl_window_stack_cycle_up (sl_window_stack* restrict this) {
	if (this->workspace_vector.indexes[this->current_workspace] == M_invalid_index) return;

	if (this->data[this->workspace_vector.indexes[this->current_workspace]].next == M_invalid_index) return;

	((sl_window_stack_mutable*)this)->workspace_vector.indexes[this->current_workspace] =
	this->data[this->workspace_vector.indexes[this->current_workspace]].next;
}

void sl_window_stack_cycle_down (sl_window_stack* restrict this) {
	if (this->workspace_vector.indexes[this->current_workspace] == M_invalid_index) return;

	if (this->data[this->workspace_vector.indexes[this->current_workspace]].previous == M_invalid_index) return;

	((sl_window_stack_mutable*)this)->workspace_vector.indexes[this->current_workspace] =
	this->data[this->workspace_vector.indexes[this->current_workspace]].previous;
}

void sl_window_stack_cycle_workspace_up (sl_window_stack* restrict this) {
	++((sl_window_stack_mutable*)this)->current_workspace;
	((sl_window_stack_mutable*)this)->current_workspace %= this->workspace_vector.size;
}

void sl_window_stack_cycle_workspace_down (sl_window_stack* restrict this) {
	if (this->current_workspace == 0)
		((sl_window_stack_mutable*)this)->current_workspace = this->workspace_vector.size - 1;
	else
		--((sl_window_stack_mutable*)this)->current_workspace;
}

void sl_window_stack_add_workspace (sl_window_stack* restrict this) { workspace_vector_push(&this->workspace_vector); }

void sl_window_stack_remove_workspace (sl_window_stack* restrict this) {
	if (this->workspace_vector.size <= 1) return;

	if (this->current_workspace == this->workspace_vector.size - 1) --((sl_window_stack_mutable*)this)->current_workspace;

	if (this->workspace_vector.indexes[this->workspace_vector.size - 1] == M_invalid_index) return workspace_vector_pop(&this->workspace_vector);

	if (this->workspace_vector.indexes[this->workspace_vector.size - 2] == M_invalid_index) {
		((sl_window_stack_mutable*)this)->workspace_vector.indexes[this->workspace_vector.size - 2] =
		this->workspace_vector.indexes[this->workspace_vector.size - 1];

		return workspace_vector_pop(&this->workspace_vector);
	}

	if (this->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].next == M_invalid_index) {
		// note: we can assume both next and previous are invalid

		if (this->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next == M_invalid_index) {
			// note: we can assume both next and previous are invalid

			((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].next =
			this->workspace_vector.indexes[this->workspace_vector.size - 1];
			((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].previous =
			this->workspace_vector.indexes[this->workspace_vector.size - 1];

			((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next =
			this->workspace_vector.indexes[this->workspace_vector.size - 2];
			((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].previous =
			this->workspace_vector.indexes[this->workspace_vector.size - 2];

			return workspace_vector_pop(&this->workspace_vector);
		}

		((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].next =
		this->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next;
		((sl_window_stack_mutable*)this)->data[this->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next].previous =
		this->workspace_vector.indexes[this->workspace_vector.size - 2];

		((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].previous =
		this->workspace_vector.indexes[this->workspace_vector.size - 1];
		((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next =
		this->workspace_vector.indexes[this->workspace_vector.size - 2];

		return workspace_vector_pop(&this->workspace_vector);
	}

	if (this->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next == M_invalid_index) {
		((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next =
		this->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].next;
		((sl_window_stack_mutable*)this)->data[this->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].next].previous =
		this->workspace_vector.indexes[this->workspace_vector.size - 1];

		((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].next =
		this->workspace_vector.indexes[this->workspace_vector.size - 1];
		((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].previous =
		this->workspace_vector.indexes[this->workspace_vector.size - 2];

		return workspace_vector_pop(&this->workspace_vector);
	}

	size_t bottom = this->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].next;

	((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 2]].next =
	this->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next;
	((sl_window_stack_mutable*)this)->data[this->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next].previous =
	this->workspace_vector.indexes[this->workspace_vector.size - 2];

	((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->workspace_vector.size - 1]].next = bottom;
	((sl_window_stack_mutable*)this)->data[bottom].previous = this->workspace_vector.indexes[this->workspace_vector.size - 1];

	return workspace_vector_pop(&this->workspace_vector);
}

void sl_window_stack_set_to_top (sl_window_stack* restrict this, size_t index) {
	((sl_window_stack_mutable*)this)->data[this->data[index].previous].next = this->data[index].next;
	((sl_window_stack_mutable*)this)->data[this->data[index].next].previous = this->data[index].previous;

	((sl_window_stack_mutable*)this)->data[this->workspace_vector.indexes[this->current_workspace]].next = index;
	((sl_window_stack_mutable*)this)->data[index].previous = this->workspace_vector.indexes[this->current_workspace];

	size_t bottom = this->data[this->workspace_vector.indexes[this->current_workspace]].next;

	((sl_window_stack_mutable*)this)->data[index].next = bottom;
	((sl_window_stack_mutable*)this)->data[bottom].previous = index;
}

void set_focused_window (sl_window_stack* restrict this, size_t index) { ((sl_window_stack_mutable*)this)->focused_window_index = index; }

sl_window* get_focused_window (sl_window_stack* restrict this) {
	if (this->focused_window_index == M_invalid_index) return NULL;

	if (this->data[this->focused_window_index].flagged_for_deletion) return NULL;

	return (sl_window*)&this->data[this->focused_window_index].window;
}
