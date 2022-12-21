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

#include <stdlib.h>

#ifdef D_quiet

#	define perror(M_message)

#	define log_message(M_message) \
		{}
#	define log(M_message, ...) \
		{}
#	define log_bool(M_message, M_value) \
		{}
#	define log_parsed_2(M_message, M_value, M_parse0, M_parse1) \
		{}
#	define log_parsed_3(M_message, M_value, M_parse0, M_parse1, M_parse2) \
		{}
#	define log_parsed_4(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3) \
		{}
#	define log_parsed_5(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3, M_parse4) \
		{}
#	define log_parsed_6(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3, M_parse4, M_parse5) \
		{}
#	define log_parsed_7(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3, M_parse4, M_parse5, M_parse6) \
		{}
#	define log_parsed_8(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3, M_parse4, M_parse5, M_parse6, M_parse7) \
		{}

#	define warn_log(M_message) \
		{}
#	define warn_log_va(M_message, ...) \
		{}

#	define error_log(M_message, ...)    exit(0)
#	define error_log_va(M_message, ...) exit(0)

#	define assert(M_condition) \
		{ \
			if (!(M_condition)) exit(-1); \
		}
#	define assert_not_reached() exit(0)

#else

#	include <stdio.h>

extern FILE* output_file ();

#	if defined(D_release)

#		define log_message(M_message) fprintf(output_file(), " M_message "\n, __LINE__)
#		define log(M_message, ...)    fprintf(output_file(), " M_message "\n, __LINE__, __VA_ARGS__)

#		define warn_log(M_message)         fprintf(output_file(), "[warning] " M_message "\n")
#		define warn_log_va(M_message, ...) fprintf(output_file(), "[warning] " M_message "\n", __VA_ARGS__)

#		define error_log(M_message) \
			{ \
				fprintf(output_file(), "[error] " M_message "\n"); \
				exit(0); \
			}
#		define error_log_va(M_message, ...) \
			{ \
				fprintf(output_file(), "[error] " M_message "\n", __VA_ARGS__); \
				exit(0); \
			}

#		define assert(M_condition) \
			{ \
				if (!(M_condition)) exit(-1); \
			}
#		define assert_not_reached() exit(-1)

#	elif defined(D_debug)

#		define log_message(M_message) fprintf(output_file(), __FILE__ ":%u: " M_message "\n", __LINE__)
#		define log(M_message, ...)    fprintf(output_file(), __FILE__ ":%u: " M_message "\n", __LINE__, __VA_ARGS__)

#		define warn_log(M_message)         fprintf(output_file(), "[warning] " __FILE__ ":%u: " M_message "\n", __LINE__)
#		define warn_log_va(M_message, ...) fprintf(output_file(), "[warning] " __FILE__ ":%u: " M_message "\n", __LINE__, __VA_ARGS__)

#		define error_log(M_message) \
			{ \
				fprintf(output_file(), "[error] " __FILE__ ":%u: " M_message "\n", __LINE__); \
				exit(-1); \
			}
#		define error_log_va(M_message, ...) \
			{ \
				fprintf(output_file(), "[error] " __FILE__ ":%u: " M_message "\n", __LINE__, __VA_ARGS__); \
				exit(-1); \
			}

#		define assert(M_condition) \
			{ \
				if (!(M_condition)) { \
					fprintf(output_file(), "[assert] " __FILE__ ":%u: assertion M_condititon reached", __LINE__); \
					exit(-1); \
				} \
			}
#		define assert_not_reached() \
			{ \
				fprintf(output_file(), "[assert_not_reached] " __FILE__ ":%u", __LINE__); \
				exit(-1); \
			}

#	endif

#	define log_bool(M_message, M_value) log(M_message, M_value ? "true" : "false")

#	define log_parsed_2(M_message, M_value, M_parse0, M_parse1) \
		log(M_message, M_value == M_parse0 ? #M_parse0 : M_value == M_parse1 ? #M_parse1 : "undefined")
#	define log_parsed_3(M_message, M_value, M_parse0, M_parse1, M_parse2) \
		log(M_message, M_value == M_parse0 ? #M_parse0 : M_value == M_parse1 ? #M_parse1 : M_value == M_parse2 ? #M_parse2 : "undefined")
#	define log_parsed_4(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3) \
		log( \
		M_message, \
		M_value == M_parse0 ? #M_parse0 : \
		M_value == M_parse1 ? #M_parse1 : \
		M_value == M_parse2 ? #M_parse2 : \
		M_value == M_parse3 ? #M_parse3 : \
		                      "undefined" \
		)
#	define log_parsed_5(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3, M_parse4) \
		log( \
		M_message, \
		M_value == M_parse0 ? #M_parse0 : \
		M_value == M_parse1 ? #M_parse1 : \
		M_value == M_parse2 ? #M_parse2 : \
		M_value == M_parse3 ? #M_parse3 : \
		M_value == M_parse4 ? #M_parse4 : \
		                      "undefined" \
		)
#	define log_parsed_6(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3, M_parse4, M_parse5) \
		log( \
		M_message, \
		M_value == M_parse0 ? #M_parse0 : \
		M_value == M_parse1 ? #M_parse1 : \
		M_value == M_parse2 ? #M_parse2 : \
		M_value == M_parse3 ? #M_parse3 : \
		M_value == M_parse4 ? #M_parse4 : \
		M_value == M_parse5 ? #M_parse5 : \
		                      "undefined" \
		)
#	define log_parsed_7(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3, M_parse4, M_parse5, M_parse6) \
		log( \
		M_message, \
		M_value == M_parse0 ? #M_parse0 : \
		M_value == M_parse1 ? #M_parse1 : \
		M_value == M_parse2 ? #M_parse2 : \
		M_value == M_parse3 ? #M_parse3 : \
		M_value == M_parse4 ? #M_parse4 : \
		M_value == M_parse5 ? #M_parse5 : \
		M_value == M_parse6 ? #M_parse6 : \
		                      "undefined" \
		)
#	define log_parsed_8(M_message, M_value, M_parse0, M_parse1, M_parse2, M_parse3, M_parse4, M_parse5, M_parse6, M_parse7) \
		log( \
		M_message, \
		M_value == M_parse0 ? #M_parse0 : \
		M_value == M_parse1 ? #M_parse1 : \
		M_value == M_parse2 ? #M_parse2 : \
		M_value == M_parse3 ? #M_parse3 : \
		M_value == M_parse4 ? #M_parse4 : \
		M_value == M_parse5 ? #M_parse5 : \
		M_value == M_parse6 ? #M_parse6 : \
		M_value == M_parse7 ? #M_parse7 : \
		                      "undefined" \
		)

#endif
