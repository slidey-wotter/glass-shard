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

#include "types.h"

typedef struct sl_vector {
	void* const data;
	size_t const size;
	size_t const allocated_size;
	u32 const type;
} sl_vector;

#define sl_vector_at_gen(M_type, M_c, M_sl_type)   extern M_type* sl_vector_at_##M_c(sl_vector* vector, size_t index);
#define sl_vector_push_gen(M_type, M_c, M_sl_type) extern M_type* sl_vector_push_##M_c(sl_vector* vector, M_type element);
#define sl_vector_set_gen(M_type, M_c, M_sl_type)  extern void sl_vector_set_##M_c(sl_vector* vector, ...);
#define sl_vector_sort_gen(M_type, M_c, M_sl_type) extern void sl_vector_sort_##M_c(sl_vector* vector);

sl_type_gen(sl_vector_at_gen) sl_vector_at_gen(sl_type_register, tr, T_type_register) sl_type_gen(sl_vector_push_gen)
sl_vector_push_gen(sl_type_register, tr, T_type_register) sl_type_gen(sl_vector_set_gen) sl_type_gen(sl_vector_sort_gen)

#undef sl_vector_at_gen
#undef sl_vector_push_gen
#undef sl_vector_set_gen
#undef sl_vector_sort_gen

extern sl_vector* sl_vector_create(size_t size, u32 type);
extern void* sl_vector_at (sl_vector* vector, size_t index);
extern void* sl_vector_push (sl_vector* vector, void* element);
extern void sl_vector_pop (sl_vector* vector);
extern void sl_vector_erase (sl_vector* vector, size_t index);
extern void sl_vector_set (sl_vector* vector, ...);
extern void sl_vector_sort (sl_vector* vector, void(set)(void*, void*), bool(compare)(void*, void*));
extern void sl_vector_resize (sl_vector* vector, size_t size);
extern void sl_vector_delete (sl_vector* vector);
