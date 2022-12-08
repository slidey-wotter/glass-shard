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

typedef struct sl_array {
	void* const data;
	size_t const size;
	u32 const type;
} sl_array;

#define sl_array_create_gen(M_type, M_c, M_sl_type) extern sl_array* sl_array_create_##M_c(size_t size);
#define sl_array_at_gen(M_type, M_c, M_sl_type)     extern M_type* sl_array_at_##M_c(sl_array* array, size_t index);
#define sl_array_set_gen(M_type, M_c, M_sl_type)    extern void sl_array_set_##M_c(sl_array* array, ...);
#define sl_array_sort_gen(M_type, M_c, M_sl_type)   extern void sl_array_sort_##M_c(sl_array* array);

sl_type_gen(sl_array_create_gen) sl_type_gen(sl_array_at_gen) sl_type_gen(sl_array_set_gen) sl_type_gen(sl_array_sort_gen)

#undef sl_array_create_gen
#undef sl_array_at_gen
#undef sl_array_set_gen
#undef sl_array_sort_gen

extern sl_array* sl_array_create(size_t size, u32 type);
extern void* sl_array_at (sl_array* array, size_t index);
extern void sl_array_set (sl_array* array, ...);
extern void sl_array_sort (sl_array* array, void(set)(void*, void*), bool(compare)(void*, void*));
extern void sl_array_delete (sl_array* array);
