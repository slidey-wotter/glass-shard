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

#	ifdef D_release

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

#	endif

#	ifdef D_debug

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

#endif
