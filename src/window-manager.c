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

static void set_net_supported (sl_display* display) {
	/*
	  _NET_SUPPORTED, ATOM[]/32

	  This property MUST be set by the Window Manager to indicate which hints it supports. For example: considering _NET_WM_STATE both this atom and all
	  supported states e.g. _NET_WM_STATE_MODAL, _NET_WM_STATE_STICKY, would be listed. This assumes that backwards incompatible changes will not be
	  made to the hints (without being renamed).
	*/

	Atom data[] = {
	display->atoms[net_supported],
	display->atoms[net_wm_name],
	display->atoms[net_wm_visible_name],
	display->atoms[net_wm_icon_name],
	display->atoms[net_wm_visible_icon_name],
	display->atoms[net_wm_desktop],
	display->atoms[net_wm_window_type],
	display->atoms[net_wm_window_type_desktop],
	display->atoms[net_wm_window_type_dock],
	display->atoms[net_wm_window_type_toolbar],
	display->atoms[net_wm_window_type_menu],
	display->atoms[net_wm_window_type_utility],
	display->atoms[net_wm_window_type_splash],
	display->atoms[net_wm_window_type_dialog],
	display->atoms[net_wm_window_type_dropdown_menu],
	display->atoms[net_wm_window_type_popup_menu],
	display->atoms[net_wm_window_type_tooltip],
	display->atoms[net_wm_window_type_notification],
	display->atoms[net_wm_window_type_combo],
	display->atoms[net_wm_window_type_dnd],
	display->atoms[net_wm_window_type_normal],
	display->atoms[net_wm_state],
	display->atoms[net_wm_state_modal],
	display->atoms[net_wm_state_sticky],
	display->atoms[net_wm_state_maximized_vert],
	display->atoms[net_wm_state_maximized_horz],
	display->atoms[net_wm_state_shaded],
	display->atoms[net_wm_state_skip_taskbar],
	display->atoms[net_wm_state_skip_pager],
	display->atoms[net_wm_state_hidden],
	display->atoms[net_wm_state_fullscreen],
	display->atoms[net_wm_state_above],
	display->atoms[net_wm_state_below],
	display->atoms[net_wm_state_demands_attention],
	display->atoms[net_wm_state_focused],
	display->atoms[net_wm_allowed_actions],
	display->atoms[net_wm_action_move],
	display->atoms[net_wm_action_resize],
	display->atoms[net_wm_action_minimize],
	display->atoms[net_wm_action_shade],
	display->atoms[net_wm_action_stick],
	display->atoms[net_wm_action_maximize_horz],
	display->atoms[net_wm_action_maximize_vert],
	display->atoms[net_wm_action_fullscreen],
	display->atoms[net_wm_action_change_desktop],
	display->atoms[net_wm_action_close],
	display->atoms[net_wm_action_above],
	display->atoms[net_wm_action_below],
	display->atoms[net_wm_strut],
	display->atoms[net_wm_strut_partial],
	display->atoms[net_wm_icon_geometry],
	display->atoms[net_wm_icon],
	display->atoms[net_wm_pid],
	display->atoms[net_wm_handled_icons],
	display->atoms[net_wm_user_time],
	display->atoms[net_wm_user_time_window],
	display->atoms[net_frame_extents],
	display->atoms[net_wm_opaque_region],
	display->atoms[net_wm_bypass_compositor]};

	XChangeProperty(
	display->x_display, display->root, display->atoms[net_supported], XA_ATOM, 32, PropModeReplace, (uchar*)data, sizeof(data) / sizeof(Atom)
	);
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

	set_net_supported(display);

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
