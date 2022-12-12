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

#include "array.h"

#include <stdarg.h>
#include <stdlib.h>

#include "message.h"

#define min(a, b) ((a < b) ? a : b)

typedef struct sl_array_mutable {
	void* data;
	size_t size;
	u32 type;
} sl_array_mutable;

#define sl_array_create_gen(M_type, M_c, M_sl_type) \
	sl_array* sl_array_create_##M_c(size_t size) { \
		void* data = malloc(size * sizeof(M_type)); \
		if (!data) { \
			warn_log_va("size %lu is invalid for array of type %s", size, sl_string_from_type(M_sl_type)); \
			return NULL; \
		} \
\
		sl_array_mutable* array = malloc(sizeof(sl_array_mutable)); \
		if (!array) error_log("invalid allocation"); \
\
		*array = (sl_array_mutable) {.data = data, .size = size, .type = M_sl_type}; \
		return (sl_array*)array; \
	}

sl_type_gen(sl_array_create_gen)
#define sl_array_at_gen(M_type, M_c, M_sl_type) \
	M_type* sl_array_at_##M_c(sl_array* array, size_t index) { \
		if (!array) { \
			warn_log("input of array_at is null"); \
			return NULL; \
		} \
\
		if (array->type != M_sl_type) { \
			warn_log_va("expected array->type => %s and got %s", sl_string_from_type(M_sl_type), sl_string_from_type(array->type)); \
			return NULL; \
		} \
\
		if (index >= array->size) { \
			warn_log_va("index of %lu out of bounds for array of size %lu", index, array->size); \
			return NULL; \
		} \
\
		return (M_type*)array->data + index; \
	}
sl_type_gen(sl_array_at_gen)

#define sl_array_set_gen(M_type, M_va_type, M_c, M_sl_type) \
	void sl_array_set_##M_c(sl_array* array, ...) { \
		if (!array) { \
			warn_log("input of array_set is null"); \
			return; \
		} \
\
		if (array->type != M_sl_type) { \
			warn_log_va("expected array->type => %s and got %s", sl_string_from_type(M_sl_type), sl_string_from_type(array->type)); \
			return; \
		} \
\
		va_list va; \
		va_start(va, array); \
		for (size_t i = 0; i < array->size; ++i) \
			*sl_array_at_##M_c(array, i) = va_arg(va, M_va_type); \
		va_end(va); \
	}
sl_type_gen_for_va(sl_array_set_gen)

#define merge_gen(M_type, M_c, M_sl_type) \
	static void merge_##M_c(M_type* destination, M_type* source, size_t left, size_t right, size_t end) { \
		size_t i = left, j = right; \
		for (size_t k = left; k < end; ++k) { \
			if (i < right && (source[i] <= source[j] || j == end)) { \
				destination[k] = source[i]; \
				++i; \
			} else { \
				destination[k] = source[j]; \
				++j; \
			} \
		} \
	}
sl_type_gen(merge_gen)

#define merge_sort_gen(M_type, M_c, M_sl_type) \
	static void merge_sort_##M_c(M_type* target, M_type* buffer, size_t length) { \
		M_type* swap; \
		bool flag = false; \
\
		for (size_t width = 1; width < length; width <<= 1) { \
			for (size_t i = 0; i < length; i += width << 1) \
				merge_##M_c(buffer, target, i, min(i + width, length), min(i + (width << 1), length)); \
			swap = target; \
			target = buffer; \
			buffer = swap; \
			flag = !flag; \
		} \
\
		if (flag) \
			for (size_t i = 0; i < length; ++i) \
				buffer[i] = target[i]; \
	}
sl_type_gen(merge_sort_gen)

#define sl_array_sort_gen(M_type, M_c, M_sl_type) \
	void sl_array_sort_##M_c(sl_array* array) { \
		if (!array) { \
			warn_log("input for array_sort is already null"); \
			return; \
		} \
\
		if (array->type != M_sl_type) { \
			warn_log_va("expected array->type => %s and got %s", sl_string_from_type(M_sl_type), sl_string_from_type(array->type)); \
			return; \
		} \
\
		sl_array* buffer = sl_array_create(array->size, array->type); \
\
		merge_sort_##M_c((M_type*)array->data, (M_type*)buffer->data, array->size); \
\
		sl_array_delete(buffer); \
	}
sl_type_gen(sl_array_sort_gen)

sl_array* sl_array_create(size_t size, u32 type) {
	if (size == 0) {
		warn_log("array size of 0 is invalid");
		return NULL;
	}

	void* data = malloc(size * sl_size_from_type(type));
	if (!data) {
		warn_log_va("size %lu is invalid for array of type %s", size, sl_string_from_type(type));
		return NULL;
	}

	sl_array_mutable* array = malloc(sizeof(sl_array_mutable));
	if (!array) error_log("invalid allocation");

	*array = (sl_array_mutable) {.data = data, .size = size, .type = type};
	return (sl_array*)array;
}

void* sl_array_at (sl_array* array, size_t index) {
	if (!array) {
		warn_log("input of array_at is null");
		return NULL;
	}

	if (index >= array->size) {
		warn_log_va("index of %lu out of bounds for array of size %lu", index, array->size);
		return NULL;
	}

	return (void*)((u8*)array->data + index * sl_size_from_type(array->type));
}

void sl_array_set (sl_array* array, ...) {
	if (!array) {
		warn_log("input of array_set is null");
		return;
	}

	va_list va;
	va_start(va, array);
	for (size_t i = 0; i < array->size; ++i) {
		switch (array->type) {
		case T_i8: *sl_array_at_i8(array, i) = va_arg(va, int); break;
		case T_i16: *sl_array_at_i16(array, i) = va_arg(va, int); break;
		case T_i32: *sl_array_at_i32(array, i) = va_arg(va, i32); break;
		case T_i64: *sl_array_at_i64(array, i) = va_arg(va, i64); break;
		case T_u8: *sl_array_at_u8(array, i) = va_arg(va, int); break;
		case T_u16: *sl_array_at_u16(array, i) = va_arg(va, int); break;
		case T_u32: *sl_array_at_u32(array, i) = va_arg(va, u32); break;
		case T_u64: *sl_array_at_u64(array, i) = va_arg(va, u64); break;
		case T_int: *sl_array_at_i(array, i) = va_arg(va, int); break;
		case T_uint: *sl_array_at_u(array, i) = va_arg(va, uint); break;
		case T_char: *sl_array_at_c(array, i) = va_arg(va, int); break;
		case T_wchar: *sl_array_at_wc(array, i) = va_arg(va, wchar); break;
		case T_string: *sl_array_at_s(array, i) = va_arg(va, char*); break;
		case T_wstring: *sl_array_at_ws(array, i) = va_arg(va, wchar*); break;
		case T_float: *sl_array_at_f(array, i) = va_arg(va, double); break;
		case T_double: *sl_array_at_d(array, i) = va_arg(va, double); break;
		case T_ldouble: *sl_array_at_ld(array, i) = va_arg(va, ldouble); break;
		default: return;
		}
	}
	va_end(va);
}

static void
merge (void(set)(void*, void*), bool(compare)(void*, void*), u8* destination, u8* source, size_t left, size_t right, size_t end, size_t size) {
	size_t i = left, j = right;
	for (size_t k = left; k < end; ++k) {
		if (i < right && (compare((void*)source + i * size, (void*)source + j * size) || j == end)) {
			set((void*)destination + k * size, (void*)source + i * size);
			++i;
		} else {
			set((void*)destination + k * size, (void*)source + j * size);
			++j;
		}
	}
}

static void merge_sort (void(set)(void*, void*), bool(compare)(void*, void*), u8* target, u8* buffer, size_t size, size_t length) {
	u8* swap;
	bool flag = false;

	for (size_t width = 1; width < length; width <<= 1) {
		for (size_t i = 0; i < length; i += width << 1)
			merge(set, compare, buffer, target, i, min(i + width, length), min(i + (width << 1), length), size);
		swap = target;
		target = buffer;
		buffer = swap;
		flag = !flag;
	}

	if (flag)
		for (size_t i = 0; i < length; ++i)
			set((void*)buffer + i * size, (void*)target + i * size);
}

void sl_array_sort (sl_array* array, void(set)(void*, void*), bool(compare)(void*, void*)) {
	if (!array) {
		warn_log("input for array_sort is already null");
		return;
	}

	sl_array* buffer = sl_array_create(array->size, array->type);

	merge_sort(set, compare, array->data, buffer->data, sl_size_from_type(array->type), array->size);

	sl_array_delete(buffer);
}

void sl_array_delete (sl_array* array) {
	if (!array) {
		warn_log("input for array_delete is already null");
		return;
	}

	free(array->data);
	free(array);
}
