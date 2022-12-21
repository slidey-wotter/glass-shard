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

sl_window_manager* window_manager () {
	static sl_window_manager manager;
	return &manager;
}

static void elapse_event (sl_display* display, XEvent* event) {
	switch (event->type) {
	// ButtonPressMask
	case ButtonPress: return sl_button_press(display, &event->xbutton);

	// ButtonReleaseMask
	case ButtonRelease: return sl_button_release(display, &event->xbutton);

	// EnterWindowMask
	case EnterNotify: return sl_enter_notify(display, &event->xcrossing);

	// LeaveWindowMask
	case LeaveNotify: return sl_leave_notify(display, &event->xcrossing);

	// PointerMotionMask
	case MotionNotify: return sl_motion_notify(display, &event->xmotion);

	// StructureNotifyMask and SubstructureNotifyMask
	case CirculateNotify: return sl_circulate_notify(display, &event->xcirculate);
	case ConfigureNotify: return sl_configure_notify(display, &event->xconfigure);
	case CreateNotify: return sl_create_notify(display, &event->xcreatewindow);
	case DestroyNotify: return sl_destroy_notify(display, &event->xdestroywindow);
	case GravityNotify: return sl_gravity_notify(display, &event->xgravity);
	case MapNotify: return sl_map_notify(display, &event->xmap);
	case ReparentNotify: return sl_reparent_notify(display, &event->xreparent);
	case UnmapNotify: return sl_unmap_notify(display, &event->xunmap);

	// SubstructureRedirectMask
	case CirculateRequest: return sl_circulate_request(display, &event->xcirculaterequest);
	case ConfigureRequest: return sl_configure_request(display, &event->xconfigurerequest);
	case MapRequest: return sl_map_request(display, &event->xmaprequest);

	// ResizeRedirectMask
	case ResizeRequest: return sl_resize_request(display, &event->xresizerequest);

	// PropertyChangeMask
	case PropertyNotify: return sl_property_notify(display, &event->xproperty);

	// empty mask events
	case ClientMessage: return sl_client_message(display, &event->xclient);
	case MappingNotify: return sl_mapping_notify(display, &event->xmapping);
	case SelectionClear: return sl_selection_clear(display, &event->xselectionclear);
	case SelectionRequest: return sl_selection_request(display, &event->xselectionrequest);
	case SelectionNotify: return sl_selection_notify(display, &event->xselection);

	// FocusChangeMask
	case FocusIn: return sl_focus_in(display, &event->xfocus);
	case FocusOut: return sl_focus_out(display, &event->xfocus);

	// KeyPressMask
	case KeyPress: return sl_key_press(display, &event->xkey);
	case KeyRelease: return sl_key_release(display, &event->xkey);

	default:
		warn_log("default case reached at the event loop switch");
		warn_log("todo: log the display number as well");
		assert_not_reached();
		break;
	}
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

	warn_log("todo: this should be where we create threads for every display and handle each individually");
	for (XEvent event; !XNextEvent(display->x_display, &event);) {
		elapse_event(display, &event);

		if (window_manager()->logout) {
			for (size_t i = 0; i < display->window_stack.size; ++i) {
				if (!(display->window_stack.data[i].flagged_for_deletion | !sl_window_stack_is_valid_index(display->window_stack.data[i].next))) goto out;
			}
			log_message("successfuly waited for all window to delete themselves\nexiting...\n");
			sl_display_delete(display);
			return 0;
		}
	out:;
	}
}
