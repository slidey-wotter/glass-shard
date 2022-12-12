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

#include "vector.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "message.h"

#define min(a, b) ((a < b) ? a : b)

typedef struct sl_vector_mutable {
	void* data;
	size_t size;
	size_t allocated_size;
	u32 type;
} sl_vector_mutable;

static void vector_set_new_size (sl_vector* vector, size_t new_size) {
	// note: should this be inline?

	void* new_data = malloc(new_size * sl_size_from_type(vector->type));
	if (!new_data) {
		warn_log_va("size of %lu is invalid for vector of type %s", new_size, sl_string_from_type(vector->type));
		return;
	}
	if (vector->size > 0) memcpy(new_data, vector->data, vector->size * sl_size_from_type(vector->type));
	if (vector->data) free(vector->data);
	((sl_vector_mutable*)vector)->data = new_data;
	((sl_vector_mutable*)vector)->allocated_size = new_size;
}

#define sl_vector_at_gen(M_type, M_c, M_sl_type) \
	static inline M_type* vector_at_##M_c##_impl(sl_vector* vector, size_t index) { return (M_type*)vector->data + index; } \
\
	M_type* sl_vector_at_##M_c(sl_vector* vector, size_t index) { \
		if (!vector) { \
			warn_log("input of vector_at is null"); \
			return NULL; \
		} \
\
		if (index >= vector->size) { \
			warn_log_va("index of %lu out of bounds for vector of size %lu", index, vector->size); \
			return NULL; \
		} \
\
		return vector_at_##M_c##_impl(vector, index); \
	}

sl_type_gen(sl_vector_at_gen) sl_vector_at_gen(sl_type_register, tr, T_type_register)
#define sl_vector_push_gen(M_type, M_c, M_sl_type) \
	M_type* sl_vector_push_##M_c(sl_vector* vector, M_type element) { \
		if (!vector) { \
			warn_log("input of vector_push is null"); \
			return NULL; \
		} \
\
		if (vector->size == vector->allocated_size) vector_set_new_size(vector, vector->allocated_size * 2); \
\
		++((sl_vector_mutable*)vector)->size; \
		M_type* const ret = sl_vector_at_##M_c(vector, vector->size - 1); \
		*ret = element; \
		return ret; \
	}
sl_type_gen(sl_vector_push_gen) sl_vector_push_gen(sl_type_register, tr, T_type_register)

#define sl_vector_set_gen(M_type, M_va_type, M_c, M_sl_type) \
	void sl_vector_set_##M_c(sl_vector* vector, ...) { \
		if (!vector) { \
			warn_log("input of vector_set is null"); \
			return; \
		} \
\
		if (vector->type != M_sl_type) { \
			warn_log_va("expected vector->type => %s and got %s", sl_string_from_type(M_sl_type), sl_string_from_type(vector->type)); \
			return; \
		} \
\
		va_list va; \
		va_start(va, vector); \
		for (size_t i = 0; i < vector->size; ++i) \
			*vector_at_##M_c##_impl(vector, i) = va_arg(va, M_va_type); \
		va_end(va); \
	}
sl_type_gen_for_va(sl_vector_set_gen)

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

#define sl_vector_sort_gen(M_type, M_c, M_sl_type) \
	void sl_vector_sort_##M_c(sl_vector* vector) { \
		if (!vector) { \
			warn_log("input for vector_sort is already null"); \
			return; \
		} \
\
		if (vector->type != M_sl_type) { \
			warn_log_va("expected vector->type => %s and got %s", sl_string_from_type(M_sl_type), sl_string_from_type(vector->type)); \
			return; \
		} \
\
		sl_vector* buffer = sl_vector_create(vector->size, vector->type); \
\
		merge_sort_##M_c((M_type*)vector->data, (M_type*)buffer->data, vector->size); \
\
		sl_vector_delete(buffer); \
	}
sl_type_gen(sl_vector_sort_gen)

sl_vector* sl_vector_create(size_t size, u32 type) {
	if (size == 0) {
		sl_vector_mutable* vector = malloc(sizeof(sl_vector_mutable));
		if (!vector) error_log("invalid allocation");

		*vector = (sl_vector_mutable) {.data = NULL, .size = 0, .allocated_size = 0, .type = type};
		return (sl_vector*)vector;
	}

	void* data = malloc(size * sl_size_from_type(type));
	if (!data) {
		warn_log_va("size %lu is invalid for vector of type %s", size, sl_string_from_type(type));
		return NULL;
	}

	sl_vector_mutable* vector = malloc(sizeof(sl_vector_mutable));
	if (!vector) error_log("invalid allocation");

	*vector = (sl_vector_mutable) {.data = data, .size = size, .allocated_size = size, .type = type};
	return (sl_vector*)vector;
}

void* sl_vector_at (sl_vector* vector, size_t index) {
	if (!vector) {
		warn_log("input of vector_at is null");
		return NULL;
	}

	if (index >= vector->size) {
		warn_log_va("index of %lu out of bounds for vector of size %lu", index, vector->size);
		return NULL;
	}

	return (void*)((u8*)vector->data + index * sl_size_from_type(vector->type));
}

void* sl_vector_push (sl_vector* vector, void* element) {
	if (!vector) {
		warn_log("input of vector_push is null");
		return NULL;
	}

	if (!vector->data)
		vector_set_new_size(vector, 4);

	else if (vector->size == vector->allocated_size)
		vector_set_new_size(vector, vector->allocated_size * 2);

	++((sl_vector_mutable*)vector)->size;
	memcpy((u8*)vector->data + (vector->size - 1) * sl_size_from_type(vector->type), element, sl_size_from_type(vector->type));
	return (void*)((u8*)vector->data + (vector->size - 1) * sl_size_from_type(vector->type));
}

inline static void vector_pop_impl (sl_vector* vector) {
	if (vector->size - 1 < vector->allocated_size * .5) vector_set_new_size(vector, vector->allocated_size * .5);

	--((sl_vector_mutable*)vector)->size;
}

void sl_vector_pop (sl_vector* vector) {
	if (!vector) {
		warn_log("input of vector_pop is null");
		return;
	}

	if (vector->size == 0) return;

	vector_pop_impl(vector);
}

extern void sl_vector_erase (sl_vector* vector, size_t index) {
	if (!vector) {
		warn_log("input of vector_erase is null");
		return;
	}

	if (index >= vector->size) {
		warn_log_va("index of %lu out of bounds for vector of size %lu", index, vector->size);
		return;
	}

	for (; index < vector->size - 1; ++index) {
		memcpy(
		vector->data + index * sl_size_from_type(vector->type), vector->data + (index + 1) * sl_size_from_type(vector->type),
		sl_size_from_type(vector->type)
		);
	}

	vector_pop_impl(vector);
}

void sl_vector_set (sl_vector* vector, ...) {
	if (!vector) {
		warn_log("input of vector_set is null");
		return;
	}

	va_list va;
	va_start(va, vector);
	for (size_t i = 0; i < vector->size; ++i) {
		switch (vector->type) {
		case T_i8: *vector_at_i8_impl(vector, i) = va_arg(va, int); break;
		case T_i16: *vector_at_i16_impl(vector, i) = va_arg(va, int); break;
		case T_i32: *vector_at_i32_impl(vector, i) = va_arg(va, i32); break;
		case T_i64: *vector_at_i64_impl(vector, i) = va_arg(va, i64); break;
		case T_u8: *vector_at_u8_impl(vector, i) = va_arg(va, int); break;
		case T_u16: *vector_at_u16_impl(vector, i) = va_arg(va, int); break;
		case T_u32: *vector_at_u32_impl(vector, i) = va_arg(va, u32); break;
		case T_u64: *vector_at_u64_impl(vector, i) = va_arg(va, u64); break;
		case T_int: *vector_at_i_impl(vector, i) = va_arg(va, int); break;
		case T_uint: *vector_at_u_impl(vector, i) = va_arg(va, uint); break;
		case T_char: *vector_at_c_impl(vector, i) = va_arg(va, int); break;
		case T_wchar: *vector_at_wc_impl(vector, i) = va_arg(va, wchar); break;
		case T_string: *vector_at_s_impl(vector, i) = va_arg(va, char*); break;
		case T_wstring: *vector_at_ws_impl(vector, i) = va_arg(va, wchar*); break;
		case T_float: *vector_at_f_impl(vector, i) = va_arg(va, double); break;
		case T_double: *vector_at_d_impl(vector, i) = va_arg(va, double); break;
		case T_ldouble: *vector_at_ld_impl(vector, i) = va_arg(va, ldouble); break;
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

void sl_vector_sort (sl_vector* vector, void(set)(void*, void*), bool(compare)(void*, void*)) {
	if (!vector) {
		warn_log("input for vector_sort is already null");
		return;
	}

	sl_vector* buffer = sl_vector_create(vector->size, vector->type);

	merge_sort(set, compare, vector->data, buffer->data, sl_size_from_type(vector->type), vector->size);

	sl_vector_delete(buffer);
}

void sl_vector_resize (sl_vector* vector, size_t size) {
	if (!vector) {
		warn_log("input of vector_resize is null");
		return;
	}

	if (size == 0) {
		free(vector->data);
		((sl_vector_mutable*)vector)->data = NULL;
		((sl_vector_mutable*)vector)->size = 0;
		((sl_vector_mutable*)vector)->allocated_size = 0;
		return;
	}

	if (vector->data == NULL) {
		void* new_data = malloc(size * sl_size_from_type(vector->type));
		if (!new_data) warn_log_va("size of %lu is invalid for vector of type %s", size, sl_string_from_type(vector->type));
		((sl_vector_mutable*)vector)->data = new_data;
		((sl_vector_mutable*)vector)->size = size;
		((sl_vector_mutable*)vector)->allocated_size = size;
	}

	if (size < vector->allocated_size * .5) {
		size_t new_size = vector->allocated_size;
		while (size > new_size * .5)
			new_size *= .5;
		vector_set_new_size(vector, new_size);
		return;
	}

	if (size > vector->allocated_size) {
		size_t new_size = vector->allocated_size;
		while (size > new_size)
			new_size *= 2;
		vector_set_new_size(vector, new_size);
	}
}

void sl_vector_delete (sl_vector* vector) {
	if (!vector) {
		warn_log("input for vector_delete is already null");
		return;
	}

	if (vector->data) free((void*)vector->data);
	free(vector);
}
