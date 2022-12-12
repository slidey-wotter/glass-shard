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

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef unsigned uint;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned char uchar;
typedef long double ldouble;

typedef wchar_t wchar;

typedef struct sl_type_register {
	char* name;
	size_t size;
} sl_type_register;

enum sl_type {
	T_i8,
	T_i16,
	T_i32,
	T_i64,
	T_u8,
	T_u16,
	T_u32,
	T_u64,
	T_int,
	T_uint,
	T_size_type,
	T_char,
	T_wchar,
	T_string,
	T_wstring,
	T_float,
	T_double,
	T_ldouble,
	T_type_register
};

#define M_last_embedded_type (size_t) T_type_register

extern void sl_register_type (sl_type_register type);
extern size_t sl_size_from_type (u8 type);
extern char* sl_string_from_type (u8 type);
extern u32 sl_type_from_string (char* typename);

#define sl_type_gen(M_func) \
	M_func(i8, i8, T_i8) M_func(i16, i16, T_i16) M_func(i32, i32, T_i32) M_func(i64, i64, T_i64) M_func(u8, u8, T_u8) M_func(u16, u16, T_u16) \
	M_func(u32, u32, T_u32) M_func(u64, u64, T_u64) M_func(int, i, T_int) M_func(uint, u, T_uint) M_func(size_t, st, T_size_type) \
	M_func(char, c, T_char) M_func(wchar, wc, T_wchar) M_func(char*, s, T_string) M_func(wchar*, ws, T_wstring) M_func(float, f, T_float) \
	M_func(double, d, T_double) M_func(ldouble, ld, T_ldouble)

#define sl_type_gen_for_va(M_func) \
	M_func(i8, int, i8, T_i8) M_func(i16, int, i16, T_i16) M_func(i32, i32, i32, T_i32) M_func(i64, i64, i64, T_i64) M_func(u8, int, u8, T_u8) \
	M_func(u16, int, u16, T_u16) M_func(u32, u32, u32, T_u32) M_func(u64, u64, u64, T_u64) M_func(int, int, i, T_int) M_func(uint, uint, u, T_uint) \
	M_func(size_t, size_t, st, T_size_type) M_func(char, int, c, T_char) M_func(wchar, wchar, wc, T_wchar) M_func(char*, char*, s, T_string) \
	M_func(wchar*, wchar*, ws, T_wstring) M_func(float, double, f, T_float) M_func(double, double, d, T_double) \
	M_func(ldouble, ldouble, ld, T_ldouble)
