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

#include "types.h"

#include <string.h>

#include "vector.h"

static sl_vector* sl_non_trivial_types = NULL;

void set_tr (void* destination, void* source) { *(sl_type_register*)destination = *(sl_type_register*)source; }

bool compare_tr (void* lhs, void* rhs) { return strcmp(((sl_type_register*)lhs)->name, ((sl_type_register*)rhs)->name); }

void sl_register_type (sl_type_register type) {
	if (!sl_non_trivial_types) sl_non_trivial_types = sl_vector_create(0, T_type_register);

	sl_vector_push_tr(sl_non_trivial_types, type);
	sl_vector_sort(sl_non_trivial_types, set_tr, compare_tr);
}

size_t sl_size_from_type (u8 type) {
	switch (type) {
	case T_i8: return 1;
	case T_i16: return 2;
	case T_i32: return 4;
	case T_i64: return 8;
	case T_u8: return 1;
	case T_u16: return 2;
	case T_u32: return 3;
	case T_u64: return 4;
	case T_int: return sizeof(int);
	case T_uint: return sizeof(uint);
	case T_char: return sizeof(char);
	case T_wchar: return sizeof(wchar);
	case T_string: return sizeof(char*);
	case T_wstring: return sizeof(wchar*);
	case T_float: return sizeof(float);
	case T_double: return sizeof(double);
	case T_ldouble: return sizeof(ldouble);
	case T_type_register: return sizeof(sl_type_register);
	default:
		if (!sl_non_trivial_types) return 0;

		if (type - M_last_embedded_type - 1 > sl_non_trivial_types->size) return 0;

		return sl_vector_at_tr(sl_non_trivial_types, type - M_last_embedded_type - 1)->size;
	}
}

char* sl_string_from_type (u8 type) {
	switch (type) {
	case T_i8: return "i8";
	case T_i16: return "i16";
	case T_i32: return "i32";
	case T_i64: return "i64";
	case T_u8: return "u8";
	case T_u16: return "u16";
	case T_u32: return "u32";
	case T_u64: return "u64";
	case T_int: return "int";
	case T_uint: return "uint";
	case T_size_type: return "size_type";
	case T_char: return "char";
	case T_wchar: return "wchar";
	case T_string: return "string";
	case T_wstring: return "wstring";
	case T_float: return "float";
	case T_double: return "double";
	case T_ldouble: return "ldouble";
	case T_type_register: return "type_register";
	default:
		if (!sl_non_trivial_types) return "unknown";

		if (type - M_last_embedded_type - 1 > sl_non_trivial_types->size) return "unknown";

		return sl_vector_at_tr(sl_non_trivial_types, type - M_last_embedded_type - 1)->name;
	}
}

u32 sl_type_from_string (char* typename) {
	if (!sl_non_trivial_types) return -1;

	size_t l = 0;
	size_t r = sl_non_trivial_types->size - 1;
	size_t m = (l + r) >> 1;

	for (; l <= r;) {
		int result = strcmp(typename, sl_vector_at_tr(sl_non_trivial_types, m)->name);

		if (result < 0) {
			l = m + 1;
			continue;
		}

		if (result > 0) {
			r = m - 1;
			continue;
		}

		if (result == 0) return M_last_embedded_type + m + 1;
	}

	return -1;
}
