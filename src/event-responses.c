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

#include "event-responses.h"

#include <stdio.h>
#include <string.h>

#include <time.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "compiler-differences.h"
#include "display.h"
#include "types.h"
#include "util.h"
#include "window-manager.h"
#include "window.h"

#define parse_mask(m)      (m & ~(display->numlockmask | LockMask))
#define parse_mask_long(m) (m & ~(display->numlockmask | LockMask) & (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask))

#define window_handle_start \
	for (size_t i = 0; i < display->windows->size; ++i) { \
		sl_window* const window = sl_window_at(display, i); \
		if (window->x_window == event->window)

#define window_handle_end }

#define unmanaged_window_handle_start \
	for (size_t i = 0; i < display->unmanaged_windows->size; ++i) { \
		sl_window* const unmanaged_windows = sl_window_at(display, i); \
		if (unmanaged_windows->x_window == event->window)

#define unmanaged_window_handle_end }

static void button_press_or_release (sl_display* display, XButtonPressedEvent* event) {
	display->user_input_since_last_workspace_change = true;

	if (event->window == display->root) {
		if (parse_mask(event->state) == 0 || parse_mask(event->state) == (Mod4Mask) || parse_mask(event->state) == (Mod4Mask | ControlMask)) sl_focus_raised_window(display, event->time);

		if (parse_mask(event->state) == Mod4Mask || parse_mask(event->state) == (Mod4Mask | ControlMask)) {
			display->mouse.x = event->x_root;
			display->mouse.y = event->y_root;

			return;
		}
	}

	window_handle_start {
		if (!window->mapped || window->workspace != display->current_workspace) return;

		if (parse_mask(event->state) == 0 || parse_mask(event->state) == (Mod4Mask) || parse_mask(event->state) == (Mod4Mask | ControlMask)) sl_swap_window_with_raised_window(display, i, event->time);

		if (parse_mask(event->state) == Mod4Mask || parse_mask(event->state) == (Mod4Mask | ControlMask)) {
			display->mouse.x = event->x_root;
			display->mouse.y = event->y_root;
		}

		return;
	}
	window_handle_end
}

void sl_button_press (sl_display* display, XButtonPressedEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Keyboard and Pointer Events:

	  Pointer Button Events:

	  The following describes the event processing that occurs when a pointer button
	  press is processed with the pointer in some window w and when no active pointer
	  grab is in progress.

	  The X server searches the ancestors of w from the root down, looking for a passive
	  grab to activate. If no matching passive grab on the button exists, the X server
	  automatically starts an active grab for the client receiving the event and sets the
	  last-pointer-grab time to the current server time. The effect is essentially equivalent
	  to an XGrabButton with these client passed arguments:

	  Argument      | Value
	  w             | The event window
	  event_mask    | The client's selected pointer events on the event window
	  pointer_mode  | GrabModeAsync
	  keyboard_mode | GrabModeAsync
	  owner_events  | True, if the client has selected OwnerGrabButtonMask on the
	                | event window, otherwise False
	  confine_to    | None
	  cursor        | None

	  The active grab is automatically terminated when the logical state of the pointer has
	  all buttons released. Clients can modify the active grab by calling XUngrabPointer
	  and XChangeActivePointerGrab.
	*/

	return button_press_or_release(display, event);
}

void sl_button_release (sl_display* display, XButtonPressedEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Keyboard and Pointer Events:

	  Pointer Button Events:

	  The following describes the event processing that occurs when a pointer button
	  press is processed with the pointer in some window w and when no active pointer
	  grab is in progress.

	  The X server searches the ancestors of w from the root down, looking for a passive
	  grab to activate. If no matching passive grab on the button exists, the X server
	  automatically starts an active grab for the client receiving the event and sets the
	  last-pointer-grab time to the current server time. The effect is essentially equivalent
	  to an XGrabButton with these client passed arguments:

	  Argument      | Value
	  w             | The event window
	  event_mask    | The client's selected pointer events on the event window
	  pointer_mode  | GrabModeAsync
	  keyboard_mode | GrabModeAsync
	  owner_events  | True, if the client has selected OwnerGrabButtonMask on the
	                | event window, otherwise False
	  confine_to    | None
	  cursor        | None

	  The active grab is automatically terminated when the logical state of the pointer has
	  all buttons released. Clients can modify the active grab by calling XUngrabPointer
	  and XChangeActivePointerGrab.
	*/

	return button_press_or_release(display, event);
}

void sl_enter_notify (sl_display* display, XEnterWindowEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window Entry/Exit Events:

	  Normal Entry/Exit Events:

	  EnterNotify and LeaveNotify events are generated when the pointer moves from one
	  window to another window. Normal events are identified by XEnterWindowEvent or
	  XLeaveWindowEvent structures whose mode member is set to NotifyNormal.

	  Grab and Ungrab Entry/Exit Events:

	  Pseudo-motion mode EnterNotify and LeaveNotify events are generated when a
	  pointer grab activates or deactivates. Events in which the pointer grab activates are
	  identified by XEnterWindowEvent or XLeaveWindowEvent structures whose mode
	  member is set to NotifyGrab. Events in which the pointer grab deactivates are
	  identified by XEnterWindowEvent or XLeaveWindowEvent structures whose mode
	  member is set to NotifyUngrab (see XGrabPointer).
	*/

	if (event->mode != NotifyNormal) return;

	window_handle_start {
		if (window->mapped && window->workspace == display->current_workspace) return sl_focus_window(display, i, CurrentTime);
	}
	window_handle_end
}

void sl_leave_notify (M_maybe_unused sl_display* display, M_maybe_unused XLeaveWindowEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window Entry/Exit Events:

	  Normal Entry/Exit Events:

	  EnterNotify and LeaveNotify events are generated when the pointer moves from one
	  window to another window. Normal events are identified by XEnterWindowEvent or
	  XLeaveWindowEvent structures whose mode member is set to NotifyNormal.

	  Grab and Ungrab Entry/Exit Events:

	  Pseudo-motion mode EnterNotify and LeaveNotify events are generated when a
	  pointer grab activates or deactivates. Events in which the pointer grab activates are
	  identified by XEnterWindowEvent or XLeaveWindowEvent structures whose mode
	  member is set to NotifyGrab. Events in which the pointer grab deactivates are
	  identified by XEnterWindowEvent or XLeaveWindowEvent structures whose mode
	  member is set to NotifyUngrab (see XGrabPointer).
	*/
}

void sl_motion_notify (sl_display* display, XPointerMovedEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Keyboard and Pointer Events:

	  Pointer Button Events:

	  The following describes the event processing that occurs when a pointer button
	  press is processed with the pointer in some window w and when no active pointer
	  grab is in progress.

	  The X server searches the ancestors of w from the root down, looking for a passive
	  grab to activate. If no matching passive grab on the button exists, the X server
	  automatically starts an active grab for the client receiving the event and sets the
	  last-pointer-grab time to the current server time. The effect is essentially equivalent
	  to an XGrabButton with these client passed arguments:

	  Argument      | Value
	  w             | The event window
	  event_mask    | The client's selected pointer events on the event window
	  pointer_mode  | GrabModeAsync
	  keyboard_mode | GrabModeAsync
	  owner_events  | True, if the client has selected OwnerGrabButtonMask on the
	                | event window, otherwise False
	  confine_to    | None
	  cursor        | None

	  The active grab is automatically terminated when the logical state of the pointer has
	  all buttons released. Clients can modify the active grab by calling XUngrabPointer
	  and XChangeActivePointerGrab.
	*/

	display->user_input_since_last_workspace_change = true;

	if (!is_valid_window_index(display->raised_window_index)) return;

	sl_window* const raised_window = sl_raised_window(display);

	if (raised_window->fullscreen || raised_window->maximized) return;

	if (parse_mask(event->state) == (Button1MotionMask | Mod4Mask)) {
		raised_window->saved_dimensions.x += event->x_root - display->mouse.x;
		raised_window->saved_dimensions.y += event->y_root - display->mouse.y;
		display->mouse.x = event->x_root;
		display->mouse.y = event->y_root;

		return sl_move_window(display, raised_window, raised_window->saved_dimensions.x, raised_window->saved_dimensions.y);
	}

	if (parse_mask(event->state) == (Button1MotionMask | Mod4Mask | ControlMask)) {
		raised_window->saved_dimensions.width += event->x_root - display->mouse.x;
		raised_window->saved_dimensions.height += event->y_root - display->mouse.y;
		display->mouse.x = event->x_root;
		display->mouse.y = event->y_root;

		return sl_resize_window(display, raised_window, raised_window->saved_dimensions.width, raised_window->saved_dimensions.height);
	}
}

void sl_circulate_notify (M_maybe_unused sl_display* display, M_maybe_unused XCirculateEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  CirculateNotify Events:

	  The X server can report CirculateNotify events to clients wanting information
	  about when a window changes its position in the stack. The X server generates
	  this event type whenever a window is actually restacked as a result of a
	  client application calling XCirculateSubwindows, XCirculateSubwindowsUp, or
	  XCirculateSubwindowsDown.
	*/
}

void sl_configure_notify (M_maybe_unused sl_display* display, M_maybe_unused XConfigureEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  ConfigureNotify Events:

	  The X server can report ConfigureNotify events to clients wanting information
	  about actual changes to a window's state, such as size, position, border, and
	  stacking order. The X server generates this event type whenever one of the following
	  configure window requests made by a client application actually completes:

	  • A window's size, position, border, and/or stacking order is reconfigured by calling
	  XConfigureWindow.

	  • The window's position in the stacking order is changed by calling XLowerWindow,
	  XRaiseWindow, or XRestackWindows.

	  • A window is moved by calling XMoveWindow.
	  • A window's size is changed by calling XResizeWindow.
	  • A window's size and location is changed by calling XMoveResizeWindow.
	  • A window is mapped and its position in the stacking order is changed by calling
	  XMapRaised.

	  • A window's border width is changed by calling XSetWindowBorderWidth.
	*/
}

void sl_create_notify (sl_display* display, XCreateWindowEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  CreateNotify Events:

	  The X server can report CreateNotify events to clients wanting information
	  about creation of windows. The X server generates this event whenever a client
	  application creates a window by calling XCreateWindow or XCreateSimpleWindow.
	*/

	window_handle_start { return; }
	window_handle_end

	sl_vector_push(display->windows, &(sl_window) {.x_window = event->window});
}

void sl_destroy_notify (sl_display* display, XDestroyWindowEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  DestroyNotify Events:

	  The X server can report DestroyNotify events to clients wanting information about
	  which windows are destroyed. The X server generates this event whenever a client
	  application destroys a window by calling XDestroyWindow or XDestroySubwindows.
	*/

	window_handle_start { return sl_window_erase(display, i, CurrentTime); }
	window_handle_end
}

void sl_gravity_notify (M_maybe_unused sl_display* display, M_maybe_unused XGravityEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  GravityNotify Events:

	  The X server can report GravityNotify events to clients wanting information about
	  when a window is moved because of a change in the size of its parent. The X server
	  generates this event whenever a client application actually moves a child window
	  as a result of resizing its parent by calling XConfigureWindow, XMoveResizeWindow,
	  or XResizeWindow.
	*/
}

void sl_map_notify (M_maybe_unused sl_display* display, M_maybe_unused XMapEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  MapNotify Events:

	  The X server can report MapNotify events to clients wanting information about
	  which windows are mapped. The X server generates this event type whenever a
	  client application changes the window's state from unmapped to mapped by calling
	  XMapWindow, XMapRaised, XMapSubwindows, XReparentWindow, or as a result of save-
	  set processing.
	*/
}

void sl_reparent_notify (M_maybe_unused sl_display* display, M_maybe_unused XReparentEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  ReparentNotify Events:

	  The X server can report ReparentNotify events to clients wanting information about
	  changing a window's parent. The X server generates this event whenever a client
	  application calls XReparentWindow and the window is actually reparented.
	*/
}

void sl_unmap_notify (sl_display* display, XUnmapEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  UnmapNotify Events:

	  The X server can report UnmapNotify events to clients wanting information about
	  which windows are unmapped. The X server generates this event type whenever a
	  client application changes the window's state from mapped to unmapped.
	*/

	window_handle_start {
		window->mapped = false;

		if (is_valid_window_index(display->focused_window_index) && display->focused_window_index == i) display->focused_window_index = M_invalid_window_index;
		if (is_valid_window_index(display->raised_window_index) && display->raised_window_index == i) sl_cycle_windows_down(display, CurrentTime);

		return;
	}
	window_handle_end
}

void sl_circulate_request (sl_display* display, XCirculateRequestEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Structure Control Events:

	  CirculateRequest Events:

	  The X server can report CirculateRequest events to clients wanting
	  information about when another client initiates a circulate window request
	  on a specified window. The X server generates this event type whenever a
	  client initiates a circulate window request on a window and a subwindow
	  actually needs to be restacked. The client initiates a circulate window request
	  on the window by calling XCirculateSubwindows, XCirculateSubwindowsUp, or
	  XCirculateSubwindowsDown.
	*/

	window_handle_start {
		if (!window->mapped || window->workspace != display->current_workspace) return;

		if (event->place == PlaceOnTop) return sl_focus_and_raise_window(display, i, CurrentTime);

		warn_log("todo: implement PlaceOnBottom");
	}
	window_handle_end

	unmanaged_window_handle_start {
		if (event->place == PlaceOnTop) return sl_focus_and_raise_unmanaged_window(display, i, CurrentTime);

		warn_log("todo: implement PlaceOnBottom");
	}
	unmanaged_window_handle_end
}

void sl_configure_request (sl_display* display, XConfigureRequestEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Structure Control Events:

	  ConfigureRequest Events:

	  The X server can report ConfigureRequest events to clients wanting
	  information about when a different client initiates a configure window request
	  on any child of a specified window. The configure window request attempts
	  to reconfigure a window's size, position, border, and stacking order. The X
	  server generates this event whenever a different client initiates a configure
	  window request on a window by calling XConfigureWindow, XLowerWindow,
	  XRaiseWindow, XMapRaised, XMoveResizeWindow, XMoveWindow, XResizeWindow,
	  XRestackWindows, or XSetWindowBorderWidth.
	*/

	window_handle_start {
		if (window->maximized) return sl_configure_window(display, window, CWX | CWY | CWWidth | CWHeight, (XWindowChanges) {.x = display->dimensions.x, .y = display->dimensions.y, .width = display->dimensions.width, .height = display->dimensions.height});

		if (event->value_mask & (CWX | CWY | CWWidth | CWHeight)) {
			if (event->value_mask & CWX) window->saved_dimensions.x = event->x;
			if (event->value_mask & CWY) window->saved_dimensions.y = event->y;
			if (event->value_mask & CWWidth) window->saved_dimensions.width = event->width;
			if (event->value_mask & CWHeight) window->saved_dimensions.height = event->height;
			return sl_configure_window(display, window, event->value_mask & (CWX | CWY | CWWidth | CWHeight), (XWindowChanges) {.x = window->saved_dimensions.x, .y = window->saved_dimensions.y, .width = window->saved_dimensions.width, .height = window->saved_dimensions.height});
		}

		return;
	}
	window_handle_end
}

static void map_started_window (M_maybe_unused sl_display* display, M_maybe_unused size_t index) { warn_log("TODO: map_started_window"); }

static void map_unstarted_window (sl_display* display, size_t index) {
	sl_window* const window = sl_window_at(display, index);

	window->started = true;
	window->mapped = true;
	window->workspace = display->current_workspace;

	/*
	{
	  // shimeji support

	  XClassHint* hint = XAllocClassHint();
	  if (hint) {
	    XGetClassHint(display->x_display, event->xmaprequest.window, hint);
	    if (strcmp(hint->res_class, "com-group_finity-mascot-Main") == 0) {
	      sl_vector_push(display->unmanaged_windows, &(sl_window) {.x_window = event->xmaprequest.window});
	      XMapWindow(display->x_display, event->xmaprequest.window);
	      XFree(hint);
	      return;
	    }
	    XFree(hint);
	  }
	}
	*/

	XMapWindow(display->x_display, window->x_window);

	{
		XWindowAttributes attributes;

		XGetWindowAttributes(display->x_display, window->x_window, &attributes);
		if (attributes.x != 0) window->saved_dimensions.x = attributes.x;
		if (attributes.y != 0) window->saved_dimensions.y = attributes.y;
		if (attributes.width != 0) window->saved_dimensions.width = attributes.width;
		if (attributes.height != 0) window->saved_dimensions.height = attributes.height;
		sl_move_and_resize_window(display, window, window->saved_dimensions);
	}

	sl_window_set_all_properties(window, display);

	XSelectInput(display->x_display, window->x_window, EnterWindowMask | LeaveWindowMask | FocusChangeMask | PropertyChangeMask | ResizeRedirectMask | StructureNotifyMask);

	uint const modifiers[] = {0, LockMask, display->numlockmask, LockMask | display->numlockmask};
	for (unsigned i = 0; i < 4; ++i) {
		XGrabButton(display->x_display, Button1, modifiers[i], window->x_window, false, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
		XGrabButton(display->x_display, Button1, Mod4Mask | modifiers[i], window->x_window, false, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
		XGrabButton(display->x_display, Button1, Mod4Mask | ControlMask | modifiers[i], window->x_window, false, ButtonPressMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	}

	sl_focus_and_raise_window(display, index, CurrentTime);
}

void sl_map_request (sl_display* display, XMapRequestEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Structure Control Events:

	  MapRequest Events:

	  The X server can report MapRequest events to clients wanting information about
	  a different client's desire to map windows. A window is considered mapped when
	  a map window request completes. The X server generates this event whenever a
	  different client initiates a map window request on an unmapped window whose
	  override_redirect member is set to False. Clients initiate map window requests by
	  calling XMapWindow, XMapRaised, or XMapSubwindows.
	*/

	window_handle_start {
		if (window->started)
			return map_started_window(display, i);
		else
			return map_unstarted_window(display, i);
	}
	window_handle_end
}

void sl_resize_request (M_maybe_unused sl_display* display, M_maybe_unused XResizeRequestEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Structure Control Events:

	  ResizeRequest Events:

	  The X server can report ResizeRequest events to clients wanting information about
	  another client's attempts to change the size of a window. The X server generates
	  this event whenever some other client attempts to change the size of the specified
	  window by calling XConfigureWindow, XResizeWindow, or XMoveResizeWindow.
	*/
}

#ifdef D_notify_properties
#	define property_log(M_name) warn_log("Property: " M_name)
#else
#	define property_log(M_name)
#endif

void sl_property_notify (sl_display* display, XPropertyEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Client Communication Events:

	  PropertyNotify Events:

	  The X server can report PropertyNotify events to clients wanting information about
	  property changes for a specified window.

	  To receive PropertyNotify events, set the PropertyChangeMask bit in the event-
	  mask attribute of the window.

	  The structure for this event type contains:

	  typedef struct {
	  int           type;       PropertyNotify
	  unsigned long serial;     # of last request processed by server
	  Bool          send_event; true if this came from a SendEvent request
	  Display       *display;   Display the event was read from
	  Window        window;
	  Atom          atom;
	  Time          time;
	  int           state;      PropertyNewValue or PropertyDelete
	  } XPropertyEvent;

	  The window member is set to the window whose associated property was changed.
	  The atom member is set to the property's atom and indicates which property was
	  changed or desired. The time member is set to the server time when the property
	  was changed. The state member is set to indicate whether the property was changed
	  to a new value or deleted and can be PropertyNewValue or PropertyDelete. The
	  state member is set to PropertyNewValue when a property of the window is changed
	  using XChangeProperty or XRotateWindowProperties (even when adding zero-
	  length data using XChangeProperty) and when replacing all or part of a property
	  with identical data using XChangeProperty or XRotateWindowProperties. The state
	  member is set to PropertyDelete when a property of the window is deleted using
	  XDeleteProperty or, if the delete argument is True, XGetWindowProperty.
	*/

	if (event->state == PropertyDelete) {
		property_log("PropertyDelete");
		return;
	}

	window_handle_start {
		// start of icccm:

		if (event->atom == XA_WM_NAME) return sl_set_window_name(window, display);
		if (event->atom == XA_WM_ICON_NAME) return sl_set_window_icon_name(window, display);
		if (event->atom == XA_WM_NORMAL_HINTS) return sl_set_window_normal_hints(window, display);
		if (event->atom == XA_WM_HINTS) return sl_set_window_hints(window, display);
		if (event->atom == XA_WM_CLASS) return sl_set_window_class(window, display);
		if (event->atom == XA_WM_TRANSIENT_FOR) return sl_set_window_transient_for(window, display);
		if (event->atom == display->atoms[wm_protocols]) return sl_set_window_protocols(window, display);
		if (event->atom == display->atoms[wm_colormap_windows]) return sl_set_window_colormap_windows(window, display);
		if (event->atom == XA_WM_CLIENT_MACHINE) return sl_set_window_client_machine(window, display);

		// end

		// start of extended window manager hints:

		if (event->atom == display->atoms[net_wm_name]) return sl_window_set_net_wm_name(window, display);
		if (event->atom == display->atoms[net_wm_visible_name]) return sl_window_set_net_wm_visible_name(window, display);
		if (event->atom == display->atoms[net_wm_icon_name]) return sl_window_set_net_wm_icon_name(window, display);
		if (event->atom == display->atoms[net_wm_visible_icon_name]) return sl_window_set_net_wm_visible_icon_name(window, display);
		if (event->atom == display->atoms[net_wm_desktop]) return sl_window_set_net_wm_desktop(window, display);
		if (event->atom == display->atoms[net_wm_window_type]) return sl_window_set_net_wm_window_type(window, display);
		if (event->atom == display->atoms[net_wm_state]) return sl_window_set_net_wm_state(window, display);
		if (event->atom == display->atoms[net_wm_allowed_actions]) return sl_window_set_net_wm_allowed_actions(window, display);
		if (event->atom == display->atoms[net_wm_strut]) return sl_window_set_net_wm_strut(window, display);
		if (event->atom == display->atoms[net_wm_strut_partial]) return sl_window_set_net_wm_strut_partial(window, display);
		if (event->atom == display->atoms[net_wm_icon_geometry]) return sl_window_set_net_wm_icon_geometry(window, display);
		if (event->atom == display->atoms[net_wm_icon]) return sl_window_set_net_wm_icon(window, display);
		if (event->atom == display->atoms[net_wm_pid]) return sl_window_set_net_wm_pid(window, display);
		if (event->atom == display->atoms[net_wm_handled_icons]) return sl_window_set_net_wm_handled_icons(window, display);
		if (event->atom == display->atoms[net_wm_user_time]) return sl_window_set_net_wm_user_time(window, display);
		if (event->atom == display->atoms[net_wm_user_time_window]) return sl_window_set_net_wm_user_time_window(window, display);
		if (event->atom == display->atoms[net_frame_extents]) return sl_window_set_net_frame_extents(window, display);
		if (event->atom == display->atoms[net_wm_opaque_region]) return sl_window_set_net_wm_opaque_region(window, display);
		if (event->atom == display->atoms[net_wm_bypass_compositor]) return sl_window_set_net_wm_bypass_compositor(window, display);

		// end

		warn_log("unsupported property in ProperyNotify");
		return;
	}
	window_handle_end
}

// empty mask events
void sl_client_message (M_maybe_unused sl_display* display, M_maybe_unused XClientMessageEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Client Communication Events:

	  ClientMessage Events:

	  The X server generates ClientMessage events only when a client calls the function
	  XSendEvent.
	*/
}

void sl_mapping_notify (sl_display* display, XMappingEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Window State Change Events:

	  MappingNotify Events:

	  The X server reports MappingNotify events to all clients. There is no mechanism to
	  express disinterest in this event. The X server generates this event type whenever
	  a client application successfully calls:

	  • XSetModifierMapping to indicate which KeyCodes are to be used as modifiers
	  • XChangeKeyboardMapping to change the keyboard mapping
	  • XSetPointerMapping to set the pointer mapping
	*/

	XRefreshKeyboardMapping(event);
	if (event->request == MappingKeyboard) sl_grab_keys(display);
}

void sl_selection_clear (M_maybe_unused sl_display* display, M_maybe_unused XSelectionClearEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Client Communication Events:

	  SelectionClear Events:

	  The X server reports SelectionClear events to the client losing ownership of
	  a selection. The X server generates this event type when another client asserts
	  ownership of the selection by calling XSetSelectionOwner.
	*/
}

void sl_selection_request (M_maybe_unused sl_display* display, M_maybe_unused XSelectionRequestEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Client Communication Events:

	  SelectionRequest Events:

	  The X server reports SelectionRequest events to the owner of a selection. The X
	  server generates this event whenever a client requests a selection conversion by
	  calling XConvertSelection for the owned selection.
	*/
}

void sl_selection_notify (M_maybe_unused sl_display* display, M_maybe_unused XSelectionEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Client Communication Events:

	  SelectionNotify Events:

	  This event is generated by the X server in response to a ConvertSelection protocol
	  request when there is no owner for the selection. When there is an owner, it should
	  be generated by the owner of the selection by using XSendEvent. The owner of a
	  selection should send this event to a requestor when a selection has been converted
	  and stored as a property or when a selection conversion could not be performed
	  (which is indicated by setting the property member to None).

	  If None is specified as the property in the ConvertSelection protocol request,
	  the owner should choose a property name, store the result as that property on
	  the requestor window, and then send a SelectionNotify giving that actual property
	  name.
	*/
}

void sl_focus_in (M_maybe_unused sl_display* display, M_maybe_unused XFocusInEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events:

	  Input Focus Events:

	  This section describes the processing that occurs for the input focus events FocusIn
	  and FocusOut. The X server can report FocusIn or FocusOut events to clients
	  wanting information about when the input focus changes. The keyboard is always
	  attached to some window (typically, the root window or a top-level window), which is
	  called the focus window. The focus window and the position of the pointer determine
	  the window that receives keyboard input. Clients may need to know when the input
	  focus changes to control highlighting of areas on the screen.
	*/
}

void sl_focus_out (M_maybe_unused sl_display* display, M_maybe_unused XFocusOutEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events:

	  Input Focus Events:

	  This section describes the processing that occurs for the input focus events FocusIn
	  and FocusOut. The X server can report FocusIn or FocusOut events to clients
	  wanting information about when the input focus changes. The keyboard is always
	  attached to some window (typically, the root window or a top-level window), which is
	  called the focus window. The focus window and the position of the pointer determine
	  the window that receives keyboard input. Clients may need to know when the input
	  focus changes to control highlighting of areas on the screen.
	*/
}

// KeyPressMask
void sl_key_press (sl_display* display, XKeyPressedEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Keyboard and Pointer Events:

	  Keyboard and Pointer Events:

	  This section discusses the processing that occurs for the keyboard events
	  KeyPress and KeyRelease and the pointer events ButtonPress, ButtonRelease, and
	  MotionNotify. For information about the keyboard event-handling utilities, see
	  chapter 11.

	  The X server reports KeyPress or KeyRelease events to clients wanting information
	  about keys that logically change state. Note that these events are generated for
	  all keys, even those mapped to modifier bits. The X server reports ButtonPress
	  or ButtonRelease events to clients wanting information about buttons that logically
	  change state.

	  The X server reports MotionNotify events to clients wanting information about
	  when the pointer logically moves. The X server generates this event whenever
	  the pointer is moved and the pointer motion begins and ends in the window. The
	  granularity of MotionNotify events is not guaranteed, but a client that selects this
	  event type is guaranteed to receive at least one event when the pointer moves and
	  then rests.

	  The generation of the logical changes lags the physical changes if device event
	  processing is frozen.
	*/

	display->user_input_since_last_workspace_change = true;

	if (parse_mask_long(event->state) == 0) { // {k}
		switch (XLookupKeysym(event, 0)) {
		// desktop environment behavior
		case XF86XK_AudioLowerVolume: { // decrease volume
			char* const args[] = {"amixer", "-q", "sset", "Master", "5%-", 0};
			return exec_program(display->x_display, args);
		}
		case XF86XK_AudioRaiseVolume: { // increase volume
			char* const args[] = {"amixer", "-q", "sset", "Master", "5%+", 0};
			return exec_program(display->x_display, args);
		}
		case XF86XK_AudioMute: { // mute volume
			char* const args[] = {"amixer", "-q", "sset", "Master", "toggle", 0};
			return exec_program(display->x_display, args);
		}
		case XF86XK_MonBrightnessDown: { // increase display brightness
			char* const args[] = {"xbacklight", "-", "5", 0};
			return exec_program(display->x_display, args);
		}
		case XF86XK_MonBrightnessUp: { // decrease display brightness
			char* const args[] = {"xbacklight", "+", "5", 0};
			return exec_program(display->x_display, args);
		}
		case XK_Print: { // print the screen
			struct timespec timespec = {};
#ifdef D_gcc
			if (clock_gettime(CLOCK_REALTIME, &timespec) == -1) {
				perror("clock_gettime");
				assert_not_reached();
			}
#endif
			char strftime_string[64];
			struct tm* tm = localtime(&timespec.tv_sec);
			if (tm == NULL) {
				perror("localtime");
				assert_not_reached();
			}
			if (strftime(strftime_string, sizeof(strftime_string), "%Y.%m.%d-%P:%I:%M:%S", tm) == 0) {
				assert_not_reached();
			}
			char filename[128];
			sprintf(filename, "/home/slidey/media/screenshot/%s:%lu.webp", strftime_string, timespec.tv_nsec);
			warn_log("this only works for display 0");
			char* const args[] = {"ffmpeg", "-n", "-v", "0", "-f", "x11grab", "-i", ":0", "-frames:v", "1", "-lossless", "1", "-pix_fmt", "bgra", filename, 0};
			return exec_program(display->x_display, args);
		}

		default: assert_not_reached();
		}
	}

	if (parse_mask_long(event->state) == ShiftMask) { // shift + {k}
		switch (XLookupKeysym(event, 0)) {
		// volume manipulation
		case XF86XK_AudioLowerVolume: { // decrease volume (a little)
			char* const args[] = {"amixer", "-q", "sset", "Master", "1%-", 0};
			return exec_program(display->x_display, args);
		}
		case XF86XK_AudioRaiseVolume: { // increase volume (a little)
			char* const args[] = {"amixer", "-q", "sset", "Master", "1%+", 0};
			return exec_program(display->x_display, args);
		}

		// brightness manipulation
		case XF86XK_MonBrightnessDown: { // increase display brightness (a little)
			char* const args[] = {"xbacklight", "-", "1", 0};
			return exec_program(display->x_display, args);
		}
		case XF86XK_MonBrightnessUp: { // decrease display brightness (a little)
			char* const args[] = {"xbacklight", "+", "1", 0};
			return exec_program(display->x_display, args);
		}

		default: assert_not_reached();
		}
	}

	if (parse_mask_long(event->state) == ControlMask) { // control + {k}
		switch (XLookupKeysym(event, 0)) {
		// volume manipulation
		case XF86XK_AudioLowerVolume: { // decrease volume (a lot)
			char* const args[] = {"amixer", "-q", "sset", "Master", "10%-", 0};
			return exec_program(display->x_display, args);
		}
		case XF86XK_AudioRaiseVolume: { // increase volume (a lot)
			char* const args[] = {"amixer", "-q", "sset", "Master", "10%+", 0};
			return exec_program(display->x_display, args);
		}

		// brightness manipulation
		case XF86XK_MonBrightnessDown: { // increase display brightness (a lot)
			char* const args[] = {"xbacklight", "-", "10", 0};
			return exec_program(display->x_display, args);
		}
		case XF86XK_MonBrightnessUp: { // decrease display brightness (a lot)
			char* const args[] = {"xbacklight", "+", "10", 0};
			return exec_program(display->x_display, args);
		}

		default: assert_not_reached(); break;
		}
	}

	if (parse_mask_long(event->state) == Mod4Mask) { // super + {k}
		switch (XLookupKeysym(event, 0)) {
			// meta
		case XK_w: // logout
			if (window_manager()->logout) {
				warn_log("Meta: exiting before waiting for all windows to close themselves\n");
				exit(-1);
			}

			window_manager()->logout = true;

			return sl_delete_all_windows(display, event->time);

		// window manipulation
		case XK_m: // maximize
			return sl_maximize_raised_window(display);

		case XK_c: // close
			return sl_close_raised_window(display, event->time);

		case XK_Tab: // cycle the windows
			return sl_cycle_windows_up(display, event->time);

		// program execution shortcuts
		case XK_t: {
			char* const args[] = {"lxterminal", 0};
			return exec_program(display->x_display, args);
		}
		case XK_d: {
			char* const args[] = {"discord", 0};
			return exec_program(display->x_display, args);
		}
		case XK_f: {
			char* const args[] = {"thunar", 0};
			return exec_program(display->x_display, args);
		}
		case XK_e: {
			char* const args[] = {"firefox-bin", 0};
			return exec_program(display->x_display, args);
		}
		case XK_g: {
			char* const args[] = {"gimp", 0};
			return exec_program(display->x_display, args);
		}

		// workspace manipulation
		case XK_Right: // switch to workspace to the right
			return sl_next_workspace(display, event->time);

		case XK_Left: // switch to workspace to the left
			return sl_previous_workspace(display, event->time);

		case XK_0: // switch to workspace 10
			return sl_switch_to_workspace(display, 9, event->time);

		case XK_1: // switch to workspace 1
			return sl_switch_to_workspace(display, 0, event->time);

		case XK_2: // switch to workspace 2
			return sl_switch_to_workspace(display, 1, event->time);

		case XK_3: // switch to workspace 3
			return sl_switch_to_workspace(display, 2, event->time);

		case XK_4: // switch to workspace 4
			return sl_switch_to_workspace(display, 3, event->time);

		case XK_5: // switch to workspace 5
			return sl_switch_to_workspace(display, 4, event->time);

		case XK_6: // switch to workspace 6
			return sl_switch_to_workspace(display, 5, event->time);

		case XK_7: // switch to workspace 7
			return sl_switch_to_workspace(display, 6, event->time);

		case XK_8: // switch to workspace 8
			return sl_switch_to_workspace(display, 7, event->time);

		case XK_9: // switch to workspace 9
			return sl_switch_to_workspace(display, 8, event->time);

		case XK_KP_Add: // add a workspace
			return sl_push_workspace(display);

		case XK_KP_Subtract: // remove the last workspace
			return sl_pop_workspace(display, event->time);

		default: assert_not_reached(); return;
		}
	}

	if (parse_mask_long(event->state) == Mod1Mask) { // alt + {k}
		switch (XLookupKeysym(event, 0)) {
		case XK_Tab: // cycle the windows
			return sl_cycle_windows_up(display, event->time);

		default: assert_not_reached();
		}
	}

	if (parse_mask_long(event->state) == (ShiftMask | Mod4Mask)) { // shift + super + {k}
		switch (XLookupKeysym(event, 0)) {
		case XK_Tab: // cycle the windows (reverse)
			return sl_cycle_windows_down(display, event->time);

		default: assert_not_reached();
		}
	}

	if (parse_mask_long(event->state) == (Mod4Mask | ControlMask)) { // control + super + {k}
		switch (XLookupKeysym(event, 0)) {
		// volume manipulation
		case XK_0: { // set volume to 100% (not 0%)
			char* const args[] = {"amixer", "-q", "sset", "Master", "100%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_1: { // set volume to 10%
			char* const args[] = {"amixer", "-q", "sset", "Master", "10%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_2: { // set volume to 20%
			char* const args[] = {"amixer", "-q", "sset", "Master", "20%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_3: { // set volume to 30%
			char* const args[] = {"amixer", "-q", "sset", "Master", "30%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_4: { // set volume to 40%
			char* const args[] = {"amixer", "-q", "sset", "Master", "40%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_5: { // set volume to 50%
			char* const args[] = {"amixer", "-q", "sset", "Master", "50%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_6: { // set volume to 60%
			char* const args[] = {"amixer", "-q", "sset", "Master", "60%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_7: { // set volume to 70%
			char* const args[] = {"amixer", "-q", "sset", "Master", "70%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_8: { // set volume to 80%
			char* const args[] = {"amixer", "-q", "sset", "Master", "80%", 0};
			return exec_program(display->x_display, args);
		}
		case XK_9: { // set volume to 90%
			char* const args[] = {"amixer", "-q", "sset", "Master", "90%", 0};
			return exec_program(display->x_display, args);
		}

		// window manipulation
		case XK_m: // expand to max
			return sl_expand_raised_window_to_max(display);

		// workspace manipulation
		case XK_Right: // switch to workspace to the right while carrying the top window
			return sl_next_workspace_with_raised_window(display, event->time);
		case XK_Left: // switch to workspace to the left while carrying the top window
			return sl_previous_workspace_with_raised_window(display, event->time);

		default: assert_not_reached();
		}
	}

	if (parse_mask_long(event->state) == (ShiftMask | Mod1Mask)) { // shift + alt + {k}
		switch (XLookupKeysym(event, 0)) {
		case XK_Tab: // cycle the windows (reverse)
			return sl_cycle_windows_down(display, event->time);

		default: assert_not_reached();
		}
	}

	assert_not_reached();
}

void sl_key_release (M_maybe_unused sl_display* display, M_maybe_unused XKeyReleasedEvent* event) {
	/*
	  Xlib - C Language X Interface: Chapter 10. Events: Keyboard and Pointer Events:

	  Keyboard and Pointer Events:

	  This section discusses the processing that occurs for the keyboard events
	  KeyPress and KeyRelease and the pointer events ButtonPress, ButtonRelease, and
	  MotionNotify. For information about the keyboard event-handling utilities, see
	  chapter 11.

	  The X server reports KeyPress or KeyRelease events to clients wanting information
	  about keys that logically change state. Note that these events are generated for
	  all keys, even those mapped to modifier bits. The X server reports ButtonPress
	  or ButtonRelease events to clients wanting information about buttons that logically
	  change state.

	  The X server reports MotionNotify events to clients wanting information about
	  when the pointer logically moves. The X server generates this event whenever
	  the pointer is moved and the pointer motion begins and ends in the window. The
	  granularity of MotionNotify events is not guaranteed, but a client that selects this
	  event type is guaranteed to receive at least one event when the pointer moves and
	  then rests.

	  The generation of the logical changes lags the physical changes if device event
	  processing is frozen.
	*/
}
