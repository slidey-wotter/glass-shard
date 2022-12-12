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

#include "display.h"

#include "compiler-differences.h"
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

#include "window.h"

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a > b) ? b : a)

typedef struct sl_display_mutable {
	Display* x_display;
	Window root;
	Cursor cursor;
	sl_vector* windows;
	sl_vector* unmanaged_windows;
	Atom atoms[atoms_size];
	sl_window_dimensions dimensions;
	size_t focused_window_index, raised_window_index;
	uint numlockmask;

	struct sl_u32_position mouse;

	workspace_type current_workspace, workspaces_size;
	bool user_input_since_last_workspace_change;
} sl_display_mutable;

static uint get_numlock_mask (Display* display) {
	XModifierKeymap* modmap = XGetModifierMapping(display);
	for (u8 i = 0; i < 8; i++) {
		for (i32 j = 0; j < modmap->max_keypermod; j++) {
			if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(display, XK_Num_Lock)) {
				XFreeModifiermap(modmap);
				return 1 << i;
			}
		}
	}
	assert_not_reached();
}

sl_display* sl_display_create (Display* x_display) {
	if (!x_display) {
		warn_log("input of display_create is null");
		return NULL;
	}

	sl_display_mutable* display = malloc(sizeof(sl_display_mutable));
	if (!display) {
		warn_log("invalid allocation");
		return NULL;
	}

	display->x_display = x_display;
	display->root = DefaultRootWindow(x_display);
	display->cursor = XCreateFontCursor(display->x_display, XC_left_ptr);

	if (sl_type_from_string("sl_window") == (u32)-1) {
		sl_register_type((sl_type_register) {.name = "sl_window", .size = sizeof(sl_window)});
	}

	display->windows = sl_vector_create(0, sl_type_from_string("sl_window"));
	if (!display->windows) {
		warn_log("invalid allocation");
		free(display);
		return NULL;
	}

	display->unmanaged_windows = sl_vector_create(0, sl_type_from_string("sl_window"));
	if (!display->unmanaged_windows) {
		warn_log("invalid allocation");
		free(display);
		free(display->windows);
		return NULL;
	}

	XInternAtoms(x_display, (char**)atoms_string_list, atoms_size, false, display->atoms);

	display->raised_window_index = M_invalid_window_index;
	display->focused_window_index = M_invalid_window_index;
	display->dimensions =
	(sl_window_dimensions) {.x = 0, .y = 0, .width = XDisplayWidth(display->x_display, 0), .height = XDisplayHeight(display->x_display, 0)};
	display->workspaces_size = 4;

#ifdef D_debug
	XSynchronize(display->x_display, true);
#endif

	{
		XSetWindowAttributes attributes;
		attributes.cursor = display->cursor;
		attributes.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask | StructureNotifyMask |
		SubstructureNotifyMask | SubstructureRedirectMask | PropertyChangeMask;
		XChangeWindowAttributes(display->x_display, display->root, CWEventMask | CWCursor, &attributes);
	}

	sl_grab_keys((sl_display*)display);

	return (sl_display*)display;
}

void sl_display_delete (sl_display* display) {
	if (!display) {
		warn_log("input of display delete already null");
		return;
	}

	sl_vector_delete(display->windows);
	sl_vector_delete(display->unmanaged_windows);

	XFreeCursor(display->x_display, display->cursor);

	free(display);
}

void sl_notify_supported_atom (sl_display* display, i8 index) {
	XClientMessageEvent event = (XClientMessageEvent) {
	.type = ClientMessage,
	.serial = 0,
	.display = display->x_display,
	.window = display->root,
	.message_type = display->atoms[net_supported],
	.format = 32,
	.data.l[0] = display->atoms[index]};
	XSendEvent(display->x_display, display->root, false, SubstructureNotifyMask | SubstructureRedirectMask, (XEvent*)&event);
}

void sl_grab_keys (sl_display* display) {
	Display* const x_display = display->x_display;
	Window const root = display->root;

	display->numlockmask = get_numlock_mask(x_display);
	uint const modifiers[] = {0, display->numlockmask, LockMask, display->numlockmask | LockMask};
	XUngrabKey(x_display, AnyKey, AnyModifier, root);
	for (u8 i = 0; i < 4; ++i) {
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_AudioLowerVolume), modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_AudioRaiseVolume), modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_AudioMute), modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_MonBrightnessDown), modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_MonBrightnessUp), modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Print), modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_AudioLowerVolume), ShiftMask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_AudioRaiseVolume), ShiftMask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_MonBrightnessDown), ShiftMask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_MonBrightnessUp), ShiftMask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_AudioLowerVolume), ControlMask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_AudioRaiseVolume), ControlMask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_MonBrightnessDown), ControlMask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XF86XK_MonBrightnessUp), ControlMask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_w), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_m), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_c), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Tab), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_t), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_d), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_f), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_e), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_g), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Right), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Left), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_0), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_1), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_2), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_3), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_4), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_5), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_6), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_7), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_8), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_9), Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_KP_Add), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_KP_Subtract), Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Tab), Mod1Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Tab), ShiftMask | Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_0), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_1), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_2), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_3), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_4), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_5), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_6), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_7), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_8), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_9), ControlMask | Mod4Mask | modifiers[1], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_m), ControlMask | Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Right), ControlMask | Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Left), ControlMask | Mod4Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);

		XGrabKey(x_display, XKeysymToKeycode(x_display, XK_Tab), ShiftMask | Mod1Mask | modifiers[i], root, true, GrabModeAsync, GrabModeAsync);
	}
}

sl_window* sl_focused_window (sl_display* display) { return sl_window_at(display, display->focused_window_index); }

sl_window* sl_raised_window (sl_display* display) { return sl_window_at(display, display->raised_window_index); }

sl_window* sl_window_at (sl_display* display, size_t index) { return sl_vector_at(display->windows, index); }

sl_window* sl_unmanaged_window_at (sl_display* display, size_t index) { return sl_vector_at(display->unmanaged_windows, index); }

static size_t window_index_up (sl_display* display, size_t index) { return (index + 1) % display->windows->size; }

static size_t window_index_down (sl_display* display, size_t index) {
	if (index == 0) return display->windows->size - 1;

	return index - 1;
}

static void cycle_workspace_up (sl_display* display) {
	++display->current_workspace;
	display->current_workspace %= display->workspaces_size;
}

static void cycle_workspace_down (sl_display* display) {
	if (display->current_workspace == 0)
		display->current_workspace = display->workspaces_size - 1;
	else
		--display->current_workspace;
}

#define direction_gen(M_func) M_func(up) M_func(down)

#define cycle_windows_gen(M_direction) \
	void sl_cycle_windows_##M_direction(sl_display* display, Time time) { \
		if (!is_valid_window_index(display->raised_window_index)) return; \
\
		for (size_t next_raised_window_index = window_index_##M_direction(display, display->raised_window_index);; \
		     next_raised_window_index = window_index_##M_direction(display, next_raised_window_index)) { \
			if (next_raised_window_index == display->raised_window_index) { \
				display->focused_window_index = M_invalid_window_index; \
				display->raised_window_index = M_invalid_window_index; \
				return; \
			} \
\
			sl_window* const window = sl_window_at(display, next_raised_window_index); \
\
			if (window->mapped && window->workspace == display->current_workspace) \
				return sl_focus_and_raise_window(display, next_raised_window_index, time); \
		} \
	}

direction_gen(cycle_windows_gen)

void sl_push_workspace(sl_display* display) {
	++display->workspaces_size;
}

void sl_pop_workspace (sl_display* display, Time time) {
	if (display->workspaces_size < 2) return;

	if (display->current_workspace == (display->workspaces_size - 1)) {
		display->user_input_since_last_workspace_change = false;
		--display->current_workspace;
		sl_map_windows_for_current_workspace(display);
	} else if (display->current_workspace == (display->workspaces_size - 2)) {
		display->user_input_since_last_workspace_change = false;
		sl_map_windows_for_next_workspace(display);
	}

	for (size_t i = display->windows->size - 1;; --i) {
		sl_window* const window = sl_window_at(display, i);

		if (i > display->raised_window_index && window->mapped && window->workspace == (display->workspaces_size - 1)) {
			size_t saved_index = display->raised_window_index;
			display->raised_window_index = i;
			sl_swap_window_with_raised_window(display, saved_index, time);
		}

		if (window->workspace == (display->workspaces_size - 1)) {
			--window->workspace;
		}

		if (i == 0) break;
	}

	--display->workspaces_size;
}

#define direction_gen_next_prev(M_func) M_func(next, up) M_func(previous, down)

#define sl_direction_workspace_gen(M_fdir, M_dir) \
	void sl_##M_fdir##_workspace(sl_display* display, Time time) { \
		if (display->workspaces_size < 2) return; \
\
		warn_log("cycle_workspace"); \
\
		if (display->windows->size == 0) return cycle_workspace_##M_dir(display); \
\
		if (is_valid_window_index(display->focused_window_index)) { \
			uint const modifiers[] = {0, display->numlockmask, LockMask, display->numlockmask | LockMask}; \
			sl_window* const focused_window = sl_focused_window(display); \
			for (size_t i = 0; i < 4; ++i) \
				XGrabButton( \
				display->x_display, Button1, modifiers[i], focused_window->x_window, false, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None \
				); \
			display->focused_window_index = M_invalid_window_index; \
		} \
\
		sl_unmap_windows_for_current_workspace(display); \
		cycle_workspace_##M_dir(display); \
		sl_map_windows_for_current_workspace(display); \
\
		for (size_t next_raised_window_index = display->windows->size - 1;; --next_raised_window_index) { \
			sl_window* const window = sl_window_at(display, next_raised_window_index); \
\
			if (window->mapped && window->workspace == display->current_workspace) { \
				display->raised_window_index = next_raised_window_index; \
				sl_focus_raised_window(display, time); \
				return; \
			} \
\
			if (next_raised_window_index == 0) { \
				display->raised_window_index = M_invalid_window_index; \
				return; \
			} \
		} \
	}

direction_gen_next_prev(sl_direction_workspace_gen)

extern void sl_switch_to_workspace(sl_display* display, size_t index, Time time) {
	if (index == display->current_workspace) return;
	if (index >= display->workspaces_size) return;
	if (display->workspaces_size < 2) return;

	display->user_input_since_last_workspace_change = false;

	if (display->windows->size == 0) {
		display->current_workspace = index;
		return;
	}

	sl_unmap_windows_for_current_workspace(display);
	display->current_workspace = index;
	sl_map_windows_for_current_workspace(display);

	for (size_t next_raised_window_index = display->windows->size - 1;; --next_raised_window_index) {
		sl_window* const window = sl_window_at(display, next_raised_window_index);

		if (window->mapped && window->workspace == display->current_workspace) {
			display->raised_window_index = next_raised_window_index;
			return sl_focus_raised_window(display, time);
		}

		if (next_raised_window_index == 0) {
			display->raised_window_index = M_invalid_window_index;
			return;
		}
	}
}

#define sl_direction_workspace_with_raised_window_gen(M_fdir, M_dir) \
	void sl_##M_fdir##_workspace_with_raised_window(sl_display* display, Time time) { \
		if (display->workspaces_size < 2) return; \
\
		if (!is_valid_window_index(display->raised_window_index)) return sl_##M_fdir##_workspace(display, time); \
\
		if (display->windows->size == 0) return cycle_workspace_##M_dir(display); \
\
		sl_unmap_windows_for_current_workspace_except_raised_window(display); \
		cycle_workspace_##M_dir(display); \
		sl_map_windows_for_current_workspace_except_raised_window(display); \
\
		sl_raised_window(display)->workspace = display->current_workspace; \
\
		for (size_t topmost_window_index = display->windows->size - 1;; --topmost_window_index) { \
			sl_window* const window = sl_window_at(display, topmost_window_index); \
\
			if (window->mapped && window->workspace == display->current_workspace) { \
				size_t saved_index = display->raised_window_index; \
				display->raised_window_index = topmost_window_index; \
				sl_swap_window_with_raised_window(display, saved_index, time); \
				return; \
			} \
\
			if (topmost_window_index == 0) { \
				sl_focus_raised_window(display, time); \
				return; \
			} \
		} \
	}

direction_gen_next_prev(sl_direction_workspace_with_raised_window_gen)

static void map_window(sl_display* display, sl_window* window) {
	window->mapped = true;
	XMapWindow(display->x_display, window->x_window);
}

static void unmap_window (sl_display* display, sl_window* window) {
	window->mapped = false;
	XUnmapWindow(display->x_display, window->x_window);
}

void sl_map_windows_for_current_workspace (sl_display* display) {
	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->started && window->workspace == display->current_workspace) map_window(display, window);
	}
}

void sl_unmap_windows_for_current_workspace (sl_display* display) {
	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->started && window->workspace == display->current_workspace) unmap_window(display, window);
	}
}

void sl_map_windows_for_next_workspace (sl_display* display) {
	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->started && window->workspace == display->current_workspace + 1) map_window(display, window);
	}
}

void sl_map_windows_for_current_workspace_except_raised_window (sl_display* display) {
	for (size_t i = 0; i < display->windows->size; ++i) {
		if (is_valid_window_index(display->raised_window_index) && i == display->raised_window_index) continue;

		sl_window* const window = sl_window_at(display, i);

		if (window->started && window->workspace == display->current_workspace) map_window(display, window);
	}
}

void sl_unmap_windows_for_current_workspace_except_raised_window (sl_display* display) {
	for (size_t i = 0; i < display->windows->size; ++i) {
		if (is_valid_window_index(display->raised_window_index) && i == display->raised_window_index) continue;

		sl_window* const window = sl_window_at(display, i);

		if (window->started && window->workspace == display->current_workspace) unmap_window(display, window);
	}
}

static void focus_window_impl (sl_display* display, sl_window* window, Time time) {
	if (!window->hints.input) {
		// the window must focus itself
		return;
	}

	if (!window->have_protocols.take_focus) {
		XSetInputFocus(display->x_display, window->x_window, RevertToPointerRoot, time);
		return;
	}

	warn_log("todo: serial");
	if (time == CurrentTime) warn_log("icccm says that data[1] should never de CurrentTime");

	XClientMessageEvent event = (XClientMessageEvent) {
	.type = ClientMessage,
	.serial = 0,
	.send_event = true,
	.display = display->x_display,
	.window = window->x_window,
	.message_type = display->atoms[wm_protocols],
	.format = 32,
	.data.l[0] = display->atoms[wm_take_focus],
	.data.l[1] = time};

	XSendEvent(display->x_display, window->x_window, false, 0, (XEvent*)&event);
}

static void delete_window_impl (sl_display* display, sl_window* window, Time time) {
	if (!window->have_protocols.delete_window) {
		XKillClient(display->x_display, window->x_window);
		return;
	}

	warn_log("todo: serial");
	if (time == CurrentTime) warn_log("icccm says that data[1] should never de CurrentTime");

	XClientMessageEvent event = (XClientMessageEvent) {
	.type = ClientMessage,
	.serial = 0,
	.send_event = true,
	.display = display->x_display,
	.window = window->x_window,
	.message_type = display->atoms[wm_protocols],
	.format = 32,
	.data.l[0] = display->atoms[wm_delete_window],
	.data.l[1] = time};

	XSendEvent(display->x_display, window->x_window, false, 0, (XEvent*)&event);
}

void sl_focus_window (sl_display* display, size_t index, Time time) {
	if (is_valid_window_index(display->focused_window_index) && index == display->focused_window_index) return;

	uint const modifiers[] = {0, display->numlockmask, LockMask, display->numlockmask | LockMask};

	if (is_valid_window_index(display->focused_window_index)) {
		sl_window* const focused_window = sl_focused_window(display);
		for (size_t i = 0; i < 4; ++i)
			XGrabButton(
			display->x_display, Button1, modifiers[i], focused_window->x_window, false, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None
			);
	}

	display->focused_window_index = index;

	sl_window* const focused_window = sl_focused_window(display);
	focus_window_impl(display, focused_window, time);

	for (size_t i = 0; i < 4; ++i)
		XUngrabButton(display->x_display, Button1, modifiers[i], focused_window->x_window);
}

static void raise_window_impl (sl_display* display, sl_window* window, M_maybe_unused Time time) {
	XRaiseWindow(display->x_display, window->x_window);
}

void sl_raise_window (sl_display* display, size_t index, Time time) {
	if (is_valid_window_index(display->raised_window_index) && index == display->raised_window_index) return;

	display->raised_window_index = index;
	sl_window* const raised_window = sl_raised_window(display);
	raise_window_impl(display, raised_window, time);

	for (size_t i = 0; i < display->unmanaged_windows->size; ++i) {
		sl_window* const window = (sl_window*)display->unmanaged_windows->data + i;

		raise_window_impl(display, window, time);
	}
}

void sl_focus_and_raise_window (sl_display* display, size_t index, Time time) {
	sl_focus_window(display, index, time);
	sl_raise_window(display, index, time);
}

void sl_focus_raised_window (sl_display* display, Time time) {
	if (is_valid_window_index(display->raised_window_index)) return sl_focus_window(display, display->raised_window_index, time);
}

void sl_swap_window_with_raised_window (sl_display* display, size_t index, Time time) {
	if (is_valid_window_index(display->raised_window_index) && display->raised_window_index == index) return sl_focus_raised_window(display, time);

	if (is_valid_window_index(display->raised_window_index)) {
		sl_window* const raised_window = sl_raised_window(display);
		sl_window* const window = sl_window_at(display, index);

		sl_window_swap(window, raised_window);

		size_t saved_index = display->raised_window_index;
		if (is_valid_window_index(display->focused_window_index) && display->focused_window_index == display->raised_window_index)
			display->focused_window_index = index;
		display->raised_window_index = index;

		sl_focus_and_raise_window(display, saved_index, time);

		return;
	}

	sl_focus_and_raise_window(display, index, time);
}

void sl_focus_and_raise_unmanaged_window (sl_display* display, size_t index, Time time) {
	warn_log("should this even exist?");

	sl_window* const unmanaged_window = sl_unmanaged_window_at(display, index);

	focus_window_impl(display, unmanaged_window, time);
	raise_window_impl(display, unmanaged_window, time);
}

void send_new_dimensions_to_window (sl_display* display, sl_window* window) {
	/*
	  Inter-Client Communication Conventions Manual: Chapter 4. Client-to-Window-Manager Communication: Client Responses to Window Manager Actions:

	  Window Move:

	  If the window manager moves a top-level window without changing its size, the
	  client will receive a synthetic ConfigureNotify event following the move that
	  describes the new location in terms of the root coordinate space. Clients must not
	  respond to being moved by attempting to move themselves to a better location.

	  Any real ConfigureNotify event on a top-level window implies that the window's
	  position on the root may have changed, even though the event reports that the
	  window's position in its parent is unchanged because the window may have been
	  reparented. Note that the coordinates in the event will not, in this case, be directly
	  useful.

	  The window manager will send these events by using a SendEvent request with the
	  following arguments:

	  Argument    Value
	  destination The client's window
	  propagate   False
	  event-mask  StructureNotify
	*/

	/*
	  Inter-Client Communication Conventions Manual: Chapter 4. Client-to-Window-Manager Communication: Client Responses to Window Manager Actions:

	  Window Resize:

	  The client can elect to receive notification of being resized by selecting for
	  StructureNotify events on its top-level windows. It will receive a ConfigureNotify
	  event. The size information in the event will be correct, but the location will be in
	  the parent window (which may not be the root).

	  The response of the client to being resized should be to accept the size it has
	  been given and to do its best with it. Clients must not respond to being resized by
	  attempting to resize themselves to a better size. If the size is impossible to work
	  with, clients are free to request to change to the Iconic state.
	*/

	warn_log("todo: send x and y coordinates correctly for window resize");

	XConfigureEvent configure_event = (XConfigureEvent) {
	.type = ConfigureNotify,
	.display = display->x_display,
	.event = window->x_window,
	.window = window->x_window,
	.x = window->dimensions.x,
	.y = window->dimensions.y,
	.width = window->dimensions.width,
	.height = window->dimensions.height,
	.override_redirect = false};

	XSendEvent(display->x_display, window->x_window, false, StructureNotifyMask, (XEvent*)&configure_event);
}

void sl_move_window (sl_display* display, sl_window* window, i16 x, i16 y) {
	if (window->type & window_type_splash_bit) return;
	if (window->dimensions.x == x && window->dimensions.y == y) return;

	window->dimensions.x = x;
	window->dimensions.y = y;

	XMoveWindow(display->x_display, window->x_window, window->dimensions.x, window->dimensions.y);

	send_new_dimensions_to_window(display, window);
}

void sl_resize_window (sl_display* display, sl_window* window, u16 width, u16 height) {
	if (window->type & window_type_splash_bit) return;

	if (window->normal_hints.min_width != 0) {
		if (window->normal_hints.max_width != 0) {
			width = min(window->normal_hints.max_width, max(window->normal_hints.min_width, width));
		} else {
			width = max(window->normal_hints.min_width, width);
		}
	} else {
		if (window->normal_hints.max_width != 0) {
			width = min(window->normal_hints.max_width, width);
		}
	}

	if (window->normal_hints.min_height != 0) {
		if (window->normal_hints.max_height != 0) {
			height = min(window->normal_hints.max_height, max(window->normal_hints.min_height, height));
		} else {
			height = max(window->normal_hints.min_height, height);
		}
	} else {
		if (window->normal_hints.max_height != 0) {
			height = min(window->normal_hints.max_height, height);
		}
	}

	if (window->dimensions.width == width && window->dimensions.height == height) return;

	window->dimensions.width = width;
	window->dimensions.height = height;

	XResizeWindow(display->x_display, window->x_window, window->dimensions.width, window->dimensions.height);

	send_new_dimensions_to_window(display, window);
}

void sl_move_and_resize_window (sl_display* display, sl_window* window, sl_window_dimensions dimensions) {
	if (window->type & window_type_splash_bit) return;
	if (window->dimensions.x == dimensions.x && window->dimensions.y == dimensions.y && window->dimensions.width == dimensions.width && window->dimensions.height == dimensions.height)
		return;

	window->dimensions = dimensions;

	XMoveResizeWindow(
	display->x_display, window->x_window, window->dimensions.x, window->dimensions.y, window->dimensions.width, window->dimensions.height
	);

	send_new_dimensions_to_window(display, window);
}

void sl_configure_window (sl_display* display, sl_window* window, uint value_mask, XWindowChanges window_changes) {
	if (value_mask & CWX) window->dimensions.x = window_changes.x;
	if (value_mask & CWY) window->dimensions.y = window_changes.y;
	if (value_mask & CWWidth) window->dimensions.width = window_changes.width;
	if (value_mask & CWHeight) window->dimensions.height = window_changes.height;

	if (value_mask) XConfigureWindow(display->x_display, window->x_window, value_mask, &window_changes);
}

void sl_maximize_raised_window (sl_display* display) {
	if (!is_valid_window_index(display->raised_window_index)) return;

	sl_window* const raised_window = sl_raised_window(display);

	if (raised_window->maximized) {
		raised_window->maximized = false;

		if (raised_window->fullscreen) return; // do nothing

		return sl_move_and_resize_window(display, raised_window, raised_window->saved_dimensions);
	}

	raised_window->maximized = true;
	if (raised_window->fullscreen) return; // do nothing

	return sl_move_and_resize_window(display, raised_window, display->dimensions);
}

void sl_expand_raised_window_to_max (sl_display* display) {
	if (!is_valid_window_index(display->raised_window_index)) return;

	sl_window* const raised_window = sl_raised_window(display);

	if (raised_window->fullscreen) return; // do nothing

	raised_window->saved_dimensions = display->dimensions;

	return sl_move_and_resize_window(display, raised_window, display->dimensions);
}

void sl_close_raised_window (sl_display* display, Time time) {
	if (!is_valid_window_index(display->raised_window_index)) return;

	sl_delete_raised_window(display, time);
}

void sl_delete_all_windows (sl_display* display, Time time) {
	display->focused_window_index = M_invalid_window_index;
	display->raised_window_index = M_invalid_window_index;

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);
		if (window->started) delete_window_impl(display, window, time);
	}

	sl_vector_resize(display->windows, 0);

	for (size_t i = 0; i < display->unmanaged_windows->size; ++i) {
		sl_window* const unmanaged_window = sl_unmanaged_window_at(display, i);
		if (unmanaged_window->started) delete_window_impl(display, unmanaged_window, time);
	}

	sl_vector_resize(display->unmanaged_windows, 0);
}

void sl_delete_window (sl_display* display, size_t index, Time time) {
	if (is_valid_window_index(display->raised_window_index) && display->raised_window_index == index) {
		delete_window_impl(display, sl_window_at(display, index), time);
		sl_raised_window_erase(display, time);
		return;
	}

	sl_window* const window = sl_window_at(display, index);

	if (window->started) delete_window_impl(display, window, time);

	sl_window_erase(display, index, time);
}

void sl_delete_raised_window (sl_display* display, Time time) {
	if (!is_valid_window_index(display->raised_window_index)) return;

	delete_window_impl(display, sl_raised_window(display), time);

	sl_raised_window_erase(display, time);
}

void sl_window_erase (sl_display* display, size_t index, Time time) {
	if (is_valid_window_index(display->focused_window_index) && display->focused_window_index == index)
		display->focused_window_index = M_invalid_window_index;
	if (is_valid_window_index(display->raised_window_index) && display->raised_window_index == index) return sl_raised_window_erase(display, time);

	if (is_valid_window_index(display->raised_window_index) && display->raised_window_index > index) --display->raised_window_index;
	if (is_valid_window_index(display->focused_window_index) && display->focused_window_index > index) --display->focused_window_index;

	sl_window_destroy(sl_window_at(display, index));
	sl_vector_erase(display->windows, index);
}

void sl_raised_window_erase (sl_display* display, Time time) {
	if (is_valid_window_index(display->focused_window_index) && display->focused_window_index == display->raised_window_index)
		display->focused_window_index = M_invalid_window_index;
	size_t const old_raised_window_index = display->raised_window_index;
	sl_cycle_windows_down(display, time);

	if (is_valid_window_index(display->raised_window_index) && display->raised_window_index > old_raised_window_index) --display->raised_window_index;
	if (is_valid_window_index(display->focused_window_index) && display->focused_window_index > old_raised_window_index)
		--display->focused_window_index;

	sl_window_destroy(sl_window_at(display, old_raised_window_index));
	sl_vector_erase(display->windows, old_raised_window_index);
}
