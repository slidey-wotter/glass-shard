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

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>

#include "compiler-differences.h"
#include "window.h"

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a > b) ? b : a)

typedef struct sl_display_mutable {
	Display* x_display;
	Window root;
	Cursor cursor;
	sl_window_stack window_stack;
	Atom atoms[atoms_size];
	sl_window_dimensions dimensions;

	uint numlockmask;

	struct sl_u32_position mouse;
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

static void set_net_supported (sl_display* restrict display) {
	/*
	  _NET_SUPPORTED, ATOM[]/32

	  This property MUST be set by the Window Manager to indicate which hints it supports. For example: considering _NET_WM_STATE both this atom and all
	  supported states e.g. _NET_WM_STATE_MODAL, _NET_WM_STATE_STICKY, would be listed. This assumes that backwards incompatible changes will not be
	  made to the hints (without being renamed).
	*/

	XChangeProperty(
	display->x_display, display->root, display->atoms[net_supported], XA_ATOM, 32, PropModeReplace, (uchar*)(display->atoms + net_supported),
	atoms_size - net_supported
	);
}

sl_display* sl_display_create (Display* restrict x_display) {
	sl_display_mutable* display = malloc(sizeof(sl_display_mutable));
	if (!display) {
		warn_log("invalid allocation");
		return NULL;
	}

	display->x_display = x_display;
	display->root = DefaultRootWindow(x_display);
	display->cursor = XCreateFontCursor(display->x_display, XC_left_ptr);

	sl_window_stack_create(&display->window_stack, 0);

	if (!display->window_stack.data) {
		free(display);
		return NULL;
	}

	XInternAtoms(x_display, (char**)atoms_string_list, atoms_size, false, display->atoms);

	display->dimensions =
	(sl_window_dimensions) {.x = 0, .y = 0, .width = XDisplayWidth(display->x_display, 0), .height = XDisplayHeight(display->x_display, 0)};

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
	set_net_supported((sl_display*)display);

	return (sl_display*)display;
}

void sl_display_delete (sl_display* restrict this) {
	sl_window_stack_delete((sl_window_stack*)&this->window_stack);

	XFreeCursor(this->x_display, this->cursor);

	free(this);
}

void sl_grab_keys (sl_display* restrict this) {
	Display* const x_display = this->x_display;
	Window const root = this->root;

	this->numlockmask = get_numlock_mask(x_display);
	uint const modifiers[] = {0, this->numlockmask, LockMask, this->numlockmask | LockMask};
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

static void focus_window_impl (sl_display* restrict this, sl_window* restrict window, Time time) {
	if (!window->hints.input) {
		// the window must focus itself
		return;
	}

	uint const modifiers[] = {0, this->numlockmask, LockMask, this->numlockmask | LockMask};
	for (size_t i = 0; i < 4; ++i)
		XUngrabButton(this->x_display, Button1, modifiers[i], window->x_window);

	{
		warn_log("todo: serial");
		XClientMessageEvent event = (XClientMessageEvent) {
		.type = ClientMessage,
		.serial = 0,
		.send_event = true,
		.display = this->x_display,
		.window = window->x_window,
		.message_type = this->atoms[net_wm_state],
		.format = 32,
		.data = {.l = {M_net_wm_state_add, this->atoms[net_wm_state_focused], 0, 0, 0}}};

		XSendEvent(this->x_display, this->root, false, 0, (XEvent*)&event);
	}

	if (!window->have_protocols.take_focus) {
		XSetInputFocus(this->x_display, window->x_window, RevertToPointerRoot, time);
		return;
	}

	warn_log("todo: serial");
	if (time == CurrentTime) warn_log("icccm says that data[1] should never de CurrentTime");

	XClientMessageEvent event = (XClientMessageEvent) {
	.type = ClientMessage,
	.serial = 0,
	.send_event = true,
	.display = this->x_display,
	.window = window->x_window,
	.message_type = this->atoms[wm_protocols],
	.format = 32,
	.data.l[0] = this->atoms[wm_take_focus],
	.data.l[1] = time};

	XSendEvent(this->x_display, window->x_window, false, 0, (XEvent*)&event);
}

static void unfocus_window_impl (sl_display* restrict this, sl_window* restrict window) {
	uint const modifiers[] = {0, this->numlockmask, LockMask, this->numlockmask | LockMask};
	for (size_t i = 0; i < 4; ++i)
		XGrabButton(this->x_display, Button1, modifiers[i], window->x_window, false, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

	warn_log("todo: serial");
	XClientMessageEvent event = (XClientMessageEvent) {
	.type = ClientMessage,
	.serial = 0,
	.send_event = true,
	.display = this->x_display,
	.window = window->x_window,
	.message_type = this->atoms[net_wm_state],
	.format = 32,
	.data = {.l = {M_net_wm_state_remove, this->atoms[net_wm_state_focused], 0, 0, 0}}};

	XSendEvent(this->x_display, this->root, false, 0, (XEvent*)&event);
}

static void raise_window_impl (sl_display* restrict this, sl_window* restrict window) { XRaiseWindow(this->x_display, window->x_window); }

static void delete_window_impl (sl_display* this, sl_window* restrict window, Time time) {
	if (!window->have_protocols.delete_window) {
		XKillClient(this->x_display, window->x_window);
		return;
	}

	warn_log("todo: serial");
	if (time == CurrentTime) warn_log("icccm says that data[1] should never de CurrentTime");

	XClientMessageEvent event = (XClientMessageEvent) {
	.type = ClientMessage,
	.serial = 0,
	.send_event = true,
	.display = this->x_display,
	.window = window->x_window,
	.message_type = this->atoms[wm_protocols],
	.format = 32,
	.data.l[0] = this->atoms[wm_delete_window],
	.data.l[1] = time};

	XSendEvent(this->x_display, window->x_window, false, 0, (XEvent*)&event);
}

void sl_cycle_windows_up (sl_display* restrict this, Time time) {
	sl_window_stack_cycle_up((sl_window_stack*)&this->window_stack);

	sl_window* window = sl_window_stack_get_raised_window((sl_window_stack*)&this->window_stack);

	if (window) {
		sl_focus_raised_window(this, time);
		raise_window_impl(this, window);
	}
}

void sl_cycle_windows_down (sl_display* restrict this, Time time) {
	sl_window_stack_cycle_down((sl_window_stack*)&this->window_stack);

	sl_window* window = sl_window_stack_get_raised_window((sl_window_stack*)&this->window_stack);

	if (window) {
		sl_focus_raised_window(this, time);
		raise_window_impl(this, window);
	}
}

static void map_windows_for_current_workspace (sl_display* restrict this) {
	if (!sl_window_stack_is_valid_index(sl_window_stack_get_raised_window_index((sl_window_stack*)&this->window_stack))) return;

	for (size_t i = this->window_stack.data[this->window_stack.workspace_vector.indexes[this->window_stack.current_workspace]].next;;
	     i = this->window_stack.data[i].next) {
		XMapWindow(this->x_display, this->window_stack.data[i].window.x_window);

		if (i == this->window_stack.workspace_vector.indexes[this->window_stack.current_workspace]) break;
	}
}

static void unmap_windows_for_current_workspace (sl_display* restrict this) {
	if (!sl_window_stack_is_valid_index(sl_window_stack_get_raised_window_index((sl_window_stack*)&this->window_stack))) return;

	for (size_t i = this->window_stack.data[this->window_stack.workspace_vector.indexes[this->window_stack.current_workspace]].next;;
	     i = this->window_stack.data[i].next) {
		XUnmapWindow(this->x_display, this->window_stack.data[i].window.x_window);

		if (i == this->window_stack.workspace_vector.indexes[this->window_stack.current_workspace]) break;
	}
}

void sl_next_workspace (sl_display* restrict this, Time time) {
	if (this->window_stack.workspace_vector.size == 1) return;

	unmap_windows_for_current_workspace(this);
	sl_window_stack_cycle_workspace_up((sl_window_stack*)&this->window_stack);
	map_windows_for_current_workspace(this);

	sl_focus_raised_window(this, time);
}

void sl_previous_workspace (sl_display* restrict this, Time time) {
	if (this->window_stack.workspace_vector.size == 1) return;

	unmap_windows_for_current_workspace(this);
	sl_window_stack_cycle_workspace_down((sl_window_stack*)&this->window_stack);
	map_windows_for_current_workspace(this);

	sl_focus_raised_window(this, time);
}

void sl_push_workspace (sl_display* restrict this) { return sl_window_stack_add_workspace((sl_window_stack*)&this->window_stack); }

void sl_pop_workspace (sl_display* restrict this, Time time) {
	if (this->window_stack.workspace_vector.size <= 1) return;

	if (this->window_stack.current_workspace == this->window_stack.workspace_vector.size - 1) {
		if (sl_window_stack_is_valid_index(this->window_stack.workspace_vector.indexes[this->window_stack.workspace_vector.size - 2]))
			for (size_t i = this->window_stack.data[this->window_stack.workspace_vector.indexes[this->window_stack.workspace_vector.size - 2]].next;;
			     i = this->window_stack.data[i].next) {
				XMapWindow(this->x_display, this->window_stack.data[i].window.x_window);

				if (i == this->window_stack.workspace_vector.indexes[this->window_stack.workspace_vector.size - 2]) break;
			}
	} else if (this->window_stack.current_workspace == this->window_stack.workspace_vector.size - 2) {
		if (sl_window_stack_is_valid_index(this->window_stack.workspace_vector.indexes[this->window_stack.workspace_vector.size - 1]))
			for (size_t i = this->window_stack.data[this->window_stack.workspace_vector.indexes[this->window_stack.workspace_vector.size - 1]].next;;
			     i = this->window_stack.data[i].next) {
				XMapWindow(this->x_display, this->window_stack.data[i].window.x_window);

				if (i == this->window_stack.workspace_vector.indexes[this->window_stack.workspace_vector.size - 1]) break;
			}
	}

	sl_window_stack_remove_workspace((sl_window_stack*)&this->window_stack);

	sl_focus_raised_window(this, time);
}

void sl_switch_to_workspace (sl_display* restrict this, workspace_type workspace, Time time) {
	if (workspace == this->window_stack.current_workspace) return;
	if (workspace >= this->window_stack.workspace_vector.size) return;
	if (this->window_stack.workspace_vector.size == 1) return;

	unmap_windows_for_current_workspace(this);
	sl_window_stack_set_current_workspace((sl_window_stack*)&this->window_stack, workspace);
	map_windows_for_current_workspace(this);

	sl_focus_raised_window(this, time);
}

void sl_next_workspace_with_raised_window (sl_display* restrict this) {
	if (this->window_stack.workspace_vector.size == 1) return;

	size_t index = this->window_stack.workspace_vector.indexes[this->window_stack.current_workspace];
	size_t focused_window_index = this->window_stack.focused_window_index;

	sl_window_stack_remove_window_from_its_workspace((sl_window_stack*)&this->window_stack, index);

	unmap_windows_for_current_workspace(this);
	sl_window_stack_cycle_workspace_up((sl_window_stack*)&this->window_stack);
	map_windows_for_current_workspace(this);

	sl_window_stack_add_window_to_current_workspace((sl_window_stack*)&this->window_stack, index);

	if (index == focused_window_index) sl_window_stack_set_raised_window_as_focused((sl_window_stack*)&this->window_stack);

	raise_window_impl(this, (sl_window*)&this->window_stack.data[index].window);
}

void sl_previous_workspace_with_raised_window (sl_display* restrict this) {
	if (this->window_stack.workspace_vector.size == 1) return;

	size_t index = this->window_stack.workspace_vector.indexes[this->window_stack.current_workspace];
	size_t focused_window_index = this->window_stack.focused_window_index;

	sl_window_stack_remove_window_from_its_workspace((sl_window_stack*)&this->window_stack, index);

	unmap_windows_for_current_workspace(this);
	sl_window_stack_cycle_workspace_down((sl_window_stack*)&this->window_stack);
	map_windows_for_current_workspace(this);

	sl_window_stack_add_window_to_current_workspace((sl_window_stack*)&this->window_stack, index);

	if (index == focused_window_index) sl_window_stack_set_raised_window_as_focused((sl_window_stack*)&this->window_stack);

	raise_window_impl(this, (sl_window*)&this->window_stack.data[index].window);
}

void sl_focus_window (sl_display* restrict this, size_t index, Time time) {
	sl_window* window = (sl_window*)&this->window_stack.data[index].window;
	sl_window* focused_window = sl_window_stack_get_focused_window((sl_window_stack*)&this->window_stack);

	if (focused_window == window) return;

	if (focused_window) unfocus_window_impl(this, focused_window);

	sl_window_stack_set_focused_window((sl_window_stack*)&this->window_stack, index);

	focus_window_impl(this, window, time);
}

void sl_raise_window (sl_display* restrict this, size_t index) {
	sl_window* window = (sl_window*)&this->window_stack.data[index].window;
	sl_window* raised_window = sl_window_stack_get_raised_window((sl_window_stack*)&this->window_stack);

	if (raised_window == window) return;

	sl_window_stack_set_raised_window((sl_window_stack*)&this->window_stack, index);

	raise_window_impl(this, window);
}

void sl_focus_and_raise_window (sl_display* restrict display, size_t index, Time time) {
	sl_focus_window(display, index, time);
	sl_raise_window(display, index);
}

void sl_focus_raised_window (sl_display* restrict this, Time time) {
	sl_window* window = sl_window_stack_get_raised_window((sl_window_stack*)&this->window_stack);

	if (window) return sl_focus_window(this, this->window_stack.workspace_vector.indexes[this->window_stack.current_workspace], time);
}

static void send_new_dimensions_to_window (sl_display* restrict this, sl_window* restrict window) {
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
	.display = this->x_display,
	.event = window->x_window,
	.window = window->x_window,
	.x = window->dimensions.x,
	.y = window->dimensions.y,
	.width = window->dimensions.width,
	.height = window->dimensions.height,
	.override_redirect = false};

	XSendEvent(this->x_display, window->x_window, false, StructureNotifyMask, (XEvent*)&configure_event);
}

void sl_move_window (sl_display* restrict this, sl_window* restrict window, i16 x, i16 y) {
	if (window->type & window_type_splash_bit) return;
	if (window->dimensions.x == x && window->dimensions.y == y) return;

	window->dimensions.x = x;
	window->dimensions.y = y;

	XMoveWindow(this->x_display, window->x_window, window->dimensions.x, window->dimensions.y);

	send_new_dimensions_to_window(this, window);
}

void sl_resize_window (sl_display* restrict this, sl_window* restrict window, u16 width, u16 height) {
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

	XResizeWindow(this->x_display, window->x_window, window->dimensions.width, window->dimensions.height);

	send_new_dimensions_to_window(this, window);
}

void sl_move_and_resize_window (sl_display* restrict this, sl_window* restrict window, sl_window_dimensions dimensions) {
	if (window->type & window_type_splash_bit) return;
	if (window->dimensions.x == dimensions.x && window->dimensions.y == dimensions.y && window->dimensions.width == dimensions.width && window->dimensions.height == dimensions.height)
		return;

	window->dimensions = dimensions;

	XMoveResizeWindow(
	this->x_display, window->x_window, window->dimensions.x, window->dimensions.y, window->dimensions.width, window->dimensions.height
	);

	send_new_dimensions_to_window(this, window);
}

void sl_configure_window (sl_display* restrict this, sl_window* restrict window, uint value_mask, XWindowChanges window_changes) {
	if (value_mask & CWX) window->dimensions.x = window_changes.x;
	if (value_mask & CWY) window->dimensions.y = window_changes.y;
	if (value_mask & CWWidth) window->dimensions.width = window_changes.width;
	if (value_mask & CWHeight) window->dimensions.height = window_changes.height;

	if (value_mask) XConfigureWindow(this->x_display, window->x_window, value_mask, &window_changes);
}

void sl_window_fullscreen_change_response (sl_display* restrict this, sl_window* restrict window) {
	if ((window->state & window_state_fullscreen_bit) != 0)
		sl_move_and_resize_window(this, window, this->dimensions);
	else
		sl_move_and_resize_window(this, window, window->saved_dimensions);
}

void sl_window_maximized_change_response (sl_display* restrict this, sl_window* restrict window) {
	if ((window->state & window_state_fullscreen_bit) != 0) return;

	if (window->state & window_state_maximized_horz_bit) {
		if (window->state & window_state_maximized_vert_bit) return sl_move_and_resize_window(this, window, this->dimensions);

		return sl_move_and_resize_window(
		this, window,
		(sl_window_dimensions
		) {.x = this->dimensions.x, .y = window->saved_dimensions.y, .width = this->dimensions.width, .height = window->saved_dimensions.height}
		);
	}

	if (window->state & window_state_maximized_vert_bit)
		return sl_move_and_resize_window(
		this, window,
		(sl_window_dimensions
		) {.x = window->saved_dimensions.x, .y = this->dimensions.y, .width = window->saved_dimensions.width, .height = this->dimensions.height}
		);

	sl_move_and_resize_window(this, window, window->saved_dimensions);
}

void sl_maximize_raised_window (sl_display* restrict this) {
	sl_window* window = sl_window_stack_get_raised_window((sl_window_stack*)&this->window_stack);

	if (!window) return;

	sl_window_toggle_maximized(window, this);
	sl_window_maximized_change_response(this, window);
}

void sl_expand_raised_window_to_max (sl_display* restrict this) {
	sl_window* window = sl_window_stack_get_raised_window((sl_window_stack*)&this->window_stack);

	if (!window) return;

	if (window->state & window_state_fullscreen_bit) return; // do nothing

	window->saved_dimensions = this->dimensions;

	return sl_move_and_resize_window(this, window, this->dimensions);
}

void sl_close_raised_window (sl_display* restrict this, Time time) { return sl_delete_raised_window(this, time); }

void sl_delete_window (sl_display* restrict this, size_t index, Time time) {
	sl_window* window = (sl_window*)&this->window_stack.data[index].window;

	if (window->started) delete_window_impl(this, window, time);
}

void sl_delete_raised_window (sl_display* restrict this, Time time) {
	sl_window* window = sl_window_stack_get_raised_window((sl_window_stack*)&this->window_stack);

	if (!window) return;

	delete_window_impl(this, window, time);
}

void sl_delete_all_windows (sl_display* restrict this, Time time) {
	for (size_t i = 0; i < this->window_stack.size; ++i)
		if (!this->window_stack.data[i].flagged_for_deletion && sl_window_stack_is_valid_index(this->window_stack.data[i].next))
			delete_window_impl(this, (sl_window*)&this->window_stack.data[i].window, time);
}
