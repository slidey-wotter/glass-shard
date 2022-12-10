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

#include "window-manager.h"

#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "display.h"
#include "event-responses.h"
#include "message.h"
#include "util.h"
#include "window.h"

#ifdef D_notify_events
#	define event_case(M_event_type, M_event_function) \
	case M_event_type: { \
		warn_log("Event: " #M_event_type); \
		M_event_function(display, &event); \
	} break
#else
#	define event_case(M_event_type, M_event_function) \
	case M_event_type: { \
		M_event_function(display, &event); \
	} break
#endif

sl_window_manager* window_manager () {
	static sl_window_manager manager;
	return &manager;
}

int main () {
	sl_display* display;

	XSetErrorHandler(xerror_handler);
	XSetIOErrorHandler(xio_error_handler);

#ifdef D_gcc
	{
		struct sigaction signal_action = (struct sigaction) {.sa_flags = SA_RESTART, .sa_handler = &signal_handler};
		if (sigaction(SIGCHLD, &signal_action, NULL) == -1) {
			perror("sigaction");
			assert_not_reached();
		}
	}
#endif

	{
		Display* x_display;
		assert(x_display = XOpenDisplay(NULL));
		display = sl_display_create(x_display);
	}

	// NOTE: we are sending each atom individually instead of 5 at a time for compatibility reasons
	sl_notify_supported_atom(display, net_wm_name);
	sl_notify_supported_atom(display, net_wm_icon_name);
	sl_notify_supported_atom(display, net_wm_desktop);
	sl_notify_supported_atom(display, net_wm_window_type);
	sl_notify_supported_atom(display, net_wm_state);
	sl_notify_supported_atom(display, net_wm_allowed_actions);
	sl_notify_supported_atom(display, net_wm_strut);
	sl_notify_supported_atom(display, net_wm_strut_partial);
	sl_notify_supported_atom(display, net_wm_icon_geometry);
	sl_notify_supported_atom(display, net_wm_icon);
	sl_notify_supported_atom(display, net_wm_pid);
	sl_notify_supported_atom(display, net_wm_handled_icons);
	sl_notify_supported_atom(display, net_wm_user_time);
	sl_notify_supported_atom(display, net_wm_user_time_window);
	sl_notify_supported_atom(display, net_wm_opaque_region);
	sl_notify_supported_atom(display, net_wm_bypass_compositor);

	warn_log("todo: this should be where we create threads for every display and handle each individually");
	for (XEvent event; !XNextEvent(display->x_display, &event);) {
		switch (event.type) {
			// ButtonPressMask
			event_case(ButtonPress, sl_button_press);

			// ButtonReleaseMask
			event_case(ButtonRelease, sl_button_release);

			// EnterWindowMask
			event_case(EnterNotify, sl_enter_notify);

			// LeaveWindowMask
			event_case(LeaveNotify, sl_leave_notify);

			// PointerMotionMask
			event_case(MotionNotify, sl_motion_notify);

			// StructureNotifyMask and SubstructureNotifyMask
			event_case(CirculateNotify, sl_circulate_notify);
			event_case(ConfigureNotify, sl_configure_notify);
			event_case(CreateNotify, sl_create_notify);
			event_case(DestroyNotify, sl_destroy_notify);
			event_case(GravityNotify, sl_gravity_notify);
			event_case(MapNotify, sl_map_notify);
			event_case(ReparentNotify, sl_reparent_notify);
			event_case(UnmapNotify, sl_unmap_notify);

			// SubstructureRedirectMask
			event_case(CirculateRequest, sl_circulate_request);
			event_case(ConfigureRequest, sl_configure_request);
			event_case(MapRequest, sl_map_request);

			// ResizeRedirectMask
			event_case(ResizeRequest, sl_resize_request);

			// PropertyChangeMask
			event_case(PropertyNotify, sl_property_notify);

			// empty mask events
			event_case(ClientMessage, sl_client_message);
			event_case(MappingNotify, sl_mapping_notify);
			event_case(SelectionClear, sl_selection_clear);
			event_case(SelectionRequest, sl_selection_request);
			event_case(SelectionNotify, sl_selection_notify);

			// FocusChangeMask
			event_case(FocusIn, sl_focus_in);
			event_case(FocusOut, sl_focus_out);

			// KeyPressMask
			event_case(KeyPress, sl_key_press);
			event_case(KeyRelease, sl_key_release);

		default:
			warn_log("default case reached at the event loop switch");
			warn_log("todo: log the display number as well");
			assert_not_reached();
			break;
		}

		if (window_manager()->logout && display->windows->size == 0 && display->unmanaged_windows->size == 0) {
			warn_log("Meta: Successfuly waited for all window to delete themselves\nexiting...\n");
			sl_display_delete(display);
			return 0;
		}
	}
}
