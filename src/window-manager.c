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
#	define event_case(M_event_type, M_event_function, M_event_structure) \
	case M_event_type: { \
		warn_log_va("Event: " #M_event_type " [%lu]", M_event_structure.window); \
		M_event_function(display, &M_event_structure); \
	} break
#	define selection_event_case(M_event_type, M_event_function, M_event_structure) \
	case M_event_type: { \
		warn_log_va("Event: " #M_event_type " [%lu]", M_event_structure.requestor); \
		M_event_function(display, &M_event_structure); \
	} break
#else
#	define event_case(M_event_type, M_event_function, M_event_structure) \
	case M_event_type: { \
		M_event_function(display, &M_event_structure); \
	} break
#	define selection_event_case(M_event_type, M_event_function, M_event_structure) \
	case M_event_type: { \
		M_event_function(display, &M_event_structure); \
	} break
#endif

sl_window_manager* window_manager () {
	static sl_window_manager manager;
	return &manager;
}

static void notify_net_supported_impl (sl_display* display, size_t atom_index) {
	/*
	  Whenever this spec speaks about “sending a message to the root window”, it is understood that the client is supposed to create a ClientMessage event with the specified contents and send it by using a SendEvent request with the following arguments:

	  destination     root
	  propagate       False
	  event-mask      (SubstructureNotify|SubstructureRedirect)
	  event           the specified ClientMessage
	*/

	warn_log("todo: serial");

	XClientMessageEvent event = (XClientMessageEvent) {
	.type = ClientMessage,
	.serial = 0, /* # of last request processed by server */
	.send_event = true, /* true if this came from a SendEvent request */
	.display = display->x_display, /* Display the event was read from */
	.window = display->root,
	.message_type = display->atoms[net_supported],
	.format = 32,
	.data = {.l = {display->atoms[atom_index], 0, 0, 0, 0}}};

	XSendEvent(display->x_display, display->root, false, SubstructureNotifyMask | SubstructureRedirectMask, (XEvent*)&event);
}

static void notify_net_supported (sl_display* display) {
	/*
	  _NET_SUPPORTED, ATOM[]/32

	  This property MUST be set by the Window Manager to indicate which hints it supports. For example: considering _NET_WM_STATE both this atom and all supported states e.g. _NET_WM_STATE_MODAL, _NET_WM_STATE_STICKY, would be listed. This assumes that backwards incompatible changes will not be made to the hints (without being renamed).
	*/

	notify_net_supported_impl(display, net_wm_name);
	notify_net_supported_impl(display, net_wm_visible_name);
	notify_net_supported_impl(display, net_wm_icon_name);
	notify_net_supported_impl(display, net_wm_visible_icon_name);
	notify_net_supported_impl(display, net_wm_desktop);
	notify_net_supported_impl(display, net_wm_window_type);
	notify_net_supported_impl(display, net_wm_state);
	notify_net_supported_impl(display, net_wm_allowed_actions);
	notify_net_supported_impl(display, net_wm_strut);
	notify_net_supported_impl(display, net_wm_strut_partial);
	notify_net_supported_impl(display, net_wm_icon_geometry);
	notify_net_supported_impl(display, net_wm_icon);
	notify_net_supported_impl(display, net_wm_pid);
	notify_net_supported_impl(display, net_wm_handled_icons);
	notify_net_supported_impl(display, net_wm_user_time);
	notify_net_supported_impl(display, net_wm_user_time_window);
	notify_net_supported_impl(display, net_frame_extents);
	notify_net_supported_impl(display, net_wm_opaque_region);
	notify_net_supported_impl(display, net_wm_bypass_compositor);
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

	notify_net_supported(display);

	warn_log("todo: this should be where we create threads for every display and handle each individually");
	for (XEvent event; !XNextEvent(display->x_display, &event);) {
		switch (event.type) {
			// ButtonPressMask
			event_case(ButtonPress, sl_button_press, event.xbutton);

			// ButtonReleaseMask
			event_case(ButtonRelease, sl_button_release, event.xbutton);

			// EnterWindowMask
			event_case(EnterNotify, sl_enter_notify, event.xcrossing);

			// LeaveWindowMask
			event_case(LeaveNotify, sl_leave_notify, event.xcrossing);

			// PointerMotionMask
			event_case(MotionNotify, sl_motion_notify, event.xmotion);

			// StructureNotifyMask and SubstructureNotifyMask
			event_case(CirculateNotify, sl_circulate_notify, event.xcirculate);
			event_case(ConfigureNotify, sl_configure_notify, event.xconfigure);
			event_case(CreateNotify, sl_create_notify, event.xcreatewindow);
			event_case(DestroyNotify, sl_destroy_notify, event.xdestroywindow);
			event_case(GravityNotify, sl_gravity_notify, event.xgravity);
			event_case(MapNotify, sl_map_notify, event.xmap);
			event_case(ReparentNotify, sl_reparent_notify, event.xreparent);
			event_case(UnmapNotify, sl_unmap_notify, event.xunmap);

			// SubstructureRedirectMask
			event_case(CirculateRequest, sl_circulate_request, event.xcirculaterequest);
			event_case(ConfigureRequest, sl_configure_request, event.xconfigurerequest);
			event_case(MapRequest, sl_map_request, event.xmaprequest);

			// ResizeRedirectMask
			event_case(ResizeRequest, sl_resize_request, event.xresizerequest);

			// PropertyChangeMask
			event_case(PropertyNotify, sl_property_notify, event.xproperty);

			// empty mask events
			event_case(ClientMessage, sl_client_message, event.xclient);
			event_case(MappingNotify, sl_mapping_notify, event.xmapping);
			event_case(SelectionClear, sl_selection_clear, event.xselectionclear);
			selection_event_case(SelectionRequest, sl_selection_request, event.xselectionrequest);
			selection_event_case(SelectionNotify, sl_selection_notify, event.xselection);

			// FocusChangeMask
			event_case(FocusIn, sl_focus_in, event.xfocus);
			event_case(FocusOut, sl_focus_out, event.xfocus);

			// KeyPressMask
			event_case(KeyPress, sl_key_press, event.xkey);
			event_case(KeyRelease, sl_key_release, event.xkey);

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
