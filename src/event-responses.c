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

static void button_press_or_release (sl_display* display, XEvent* event) {
	display->user_input_since_last_workspace_change = true;

	if (event->xbutton.window == display->root) {
		if (parse_mask(event->xbutton.state) == 0 || parse_mask(event->xbutton.state) == (Mod4Mask) || parse_mask(event->xbutton.state) == (Mod4Mask | ControlMask)) sl_focus_raised_window(display, event->xbutton.time);

		if (parse_mask(event->xbutton.state) == Mod4Mask || parse_mask(event->xbutton.state) == (Mod4Mask | ControlMask)) {
			display->mouse_x = event->xbutton.x_root;
			display->mouse_y = event->xbutton.y_root;

			return;
		}
	}

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->x_window == event->xbutton.window) {
			if (!window->mapped || window->workspace != display->current_workspace) return;

			if (parse_mask(event->xbutton.state) == 0 || parse_mask(event->xbutton.state) == (Mod4Mask) || parse_mask(event->xbutton.state) == (Mod4Mask | ControlMask)) sl_swap_window_with_raised_window(display, i, event->xbutton.time);

			if (parse_mask(event->xbutton.state) == Mod4Mask || parse_mask(event->xbutton.state) == (Mod4Mask | ControlMask)) {
				display->mouse_x = event->xbutton.x_root;
				display->mouse_y = event->xbutton.y_root;
			}
		}
	}
}

void sl_button_press (sl_display* display, XEvent* event) { return button_press_or_release(display, event); }

void sl_button_release (sl_display* display, XEvent* event) { return button_press_or_release(display, event); }

void sl_enter_notify (sl_display* display, XEvent* event) {
	/*
	  EnterNotify and LeaveNotify events are generated when the pointer moves from one
	  window to another window. Normal events are identified by XEnterWindowEvent or
	  XLeaveWindowEvent structures whose mode member is set to NotifyNormal.
	*/

	if (event->xcrossing.mode != NotifyNormal) return;

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->x_window == event->xcrossing.window)
			if (window->mapped && window->workspace == display->current_workspace) return sl_focus_window(display, i, CurrentTime);
	}
}

void sl_leave_notify (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {
	/*
	  EnterNotify and LeaveNotify events are generated when the pointer moves from one
	  window to another window. Normal events are identified by XEnterWindowEvent or
	  XLeaveWindowEvent structures whose mode member is set to NotifyNormal.
	*/
}

void sl_motion_notify (sl_display* display, XEvent* event) {
	display->user_input_since_last_workspace_change = true;

	if (!is_valid_window_index(display->raised_window_index)) return;

	sl_window* const raised_window = sl_raised_window(display);

	if (raised_window->fullscreen || raised_window->maximized) return;

	if (parse_mask(event->xmotion.state) == (Button1MotionMask | Mod4Mask)) {
		raised_window->saved_position_x += event->xmotion.x_root - display->mouse_x;
		raised_window->saved_position_y += event->xmotion.y_root - display->mouse_y;
		XMoveWindow(display->x_display, raised_window->x_window, raised_window->saved_position_x, raised_window->saved_position_y);
		display->mouse_x = event->xmotion.x_root;
		display->mouse_y = event->xmotion.y_root;

		XConfigureEvent configure_event = (XConfigureEvent) {.type = ConfigureNotify, .display = display->x_display, .event = raised_window->x_window, .window = raised_window->x_window, .x = raised_window->saved_position_x, .y = raised_window->saved_position_y, .width = raised_window->saved_width, .height = raised_window->saved_height, .override_redirect = false};
		XSendEvent(display->x_display, raised_window->x_window, false, StructureNotifyMask, (XEvent*)&configure_event);

		return;
	}

	if (parse_mask(event->xmotion.state) == (Button1MotionMask | Mod4Mask | ControlMask)) {
		raised_window->saved_width += event->xmotion.x_root - display->mouse_x;
		raised_window->saved_height += event->xmotion.y_root - display->mouse_y;
		XResizeWindow(display->x_display, raised_window->x_window, raised_window->saved_width, raised_window->saved_height);
		display->mouse_x = event->xmotion.x_root;
		display->mouse_y = event->xmotion.y_root;

		XConfigureEvent configure_event = (XConfigureEvent) {.type = ConfigureNotify, .display = display->x_display, .event = raised_window->x_window, .window = raised_window->x_window, .x = raised_window->saved_position_x, .y = raised_window->saved_position_y, .width = raised_window->saved_width, .height = raised_window->saved_height, .override_redirect = false};
		XSendEvent(display->x_display, raised_window->x_window, false, StructureNotifyMask, (XEvent*)&configure_event);
	}
}

void sl_circulate_notify (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {}

void sl_configure_notify (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {
	/*  "The X server can report ConfigureNotify events to clients wanting information about actual changes to a window's state, such as size, position, border, and stacking order. The X server generates this event type whenever one of the following configure window requests made by a client application actually completes:

	  A window's size, position, border, and/or stacking order is reconfigured by calling XConfigureWindow.

	  The window's position in the stacking order is changed by calling XLowerWindow, XRaiseWindow, or XRestackWindows.

	  A window is moved by calling XMoveWindow.

	  A window's size is changed by calling XResizeWindow.

	  A window's size and location is changed by calling XMoveResizeWindow.

	  A window is mapped and its position in the stacking order is changed by calling XMapRaised.

	  A window's border width is changed by calling XSetWindowBorderWidth." */

	// NOTE: this is possibly unreachable since we recieve ConfigureRequest events.
}

void sl_create_notify (sl_display* display, XEvent* event) {
	if (event->xcreatewindow.window == display->root) return; // do nothing

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->x_window == event->xcreatewindow.window) return;
	}

	sl_vector_push(display->windows, &(sl_window) {.x_window = event->xcreatewindow.window});
}

void sl_destroy_notify (sl_display* display, XEvent* event) {
	/*
	  The X server can report DestroyNotify events to clients wanting information about
	  which windows are destroyed. The X server generates this event whenever a client
	  application destroys a window by calling XDestroyWindow or XDestroySubwindows.
	*/

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->x_window == event->xdestroywindow.window) {
			if (is_valid_window_index(display->raised_window_index) && display->raised_window_index == i) {
				size_t const saved_index = display->raised_window_index;
				sl_cycle_windows_down(display, CurrentTime);

				if (is_valid_window_index(display->raised_window_index) && display->raised_window_index > saved_index) --display->raised_window_index;

				sl_vector_erase(display->windows, i);

				return;
			}

			if (is_valid_window_index(display->raised_window_index) && display->raised_window_index > i) --display->raised_window_index;

			sl_vector_erase(display->windows, i);

			return;
		}
	}
}

void sl_gravity_notify (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) { /* "The X server can report GravityNotify events to clients wanting information about when a window is moved because of a change in the size of its parent. The X server generates this event whenever a client application actually moves a child window as a result of resizing its parent by calling XConfigureWindow, XMoveResizeWindow, or XResizeWindow." */
}

void sl_map_notify (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) { /* "The X server can report MapNotify events to clients wanting information about which windows are mapped. The X server generates this event type whenever a client application changes the window's state from unmapped to mapped by calling XMapWindow, XMapRaised, XMapSubwindows, XReparentWindow, or as a result of save-set processing." */
}

void sl_reparent_notify (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {}

void sl_unmap_notify (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {
	/*
	  The X server can report UnmapNotify events to clients wanting information about
	  which windows are unmapped. The X server generates this event type whenever a
	  client application changes the window's state from mapped to unmapped.
	*/

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->x_window == event->xunmap.window) {
			window->mapped = false;

			if (is_valid_window_index(display->focused_window_index) && display->focused_window_index == i) display->focused_window_index = M_invalid_window_index;
			if (is_valid_window_index(display->raised_window_index) && display->raised_window_index == i) return sl_cycle_windows_down(display, CurrentTime);
		}
	}
}

void sl_circulate_request (sl_display* display, XEvent* event) {
	/*
	  The X server can report CirculateRequest events to clients wanting
	  information about when another client initiates a circulate window request
	  on a specified window. The X server generates this event type whenever a
	  client initiates a circulate window request on a window and a subwindow
	  actually needs to be restacked. The client initiates a circulate window request
	  on the window by calling XCirculateSubwindows, XCirculateSubwindowsUp, or
	  XCirculateSubwindowsDown.
	*/

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->x_window == event->xcirculaterequest.window) {
			if (!window->mapped || window->workspace != display->current_workspace) return;

			if (event->xcirculaterequest.place == PlaceOnTop) {
				sl_focus_and_raise_window(display, i, CurrentTime);

				return;
			}

			warn_log("implement PlaceOnBottom"); // TODO

			return;
		}
	}

	for (size_t i = 0; i < display->unmanaged_windows->size; ++i) {
		sl_window* const unmanaged_window = sl_unmanaged_window_at(display, i);

		if (unmanaged_window->x_window == event->xcirculaterequest.window)
			if (event->xcirculaterequest.place == PlaceOnTop) return sl_focus_and_raise_unmanaged_window(display, i, CurrentTime);
	}
}

void sl_configure_request (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {
	/*
	  The X server can report ConfigureRequest events to clients wanting
	  information about when a different client initiates a configure window request
	  on any child of a specified window. The configure window request attempts
	  to reconfigure a window's size, position, border, and stacking order. The X
	  server generates this event whenever a different client initiates a configure
	  window request on a window by calling XConfigureWindow, XLowerWindow,
	  XRaiseWindow, XMapRaised, XMoveResizeWindow, XMoveWindow, XResizeWindow,
	  XRestackWindows, or XSetWindowBorderWidth.
	*/

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->x_window == event->xconfigurerequest.window) {
			if (event->xconfigurerequest.value_mask & (CWWidth | CWHeight) && ((ulong)event->xconfigurerequest.width >= display->width || (ulong)event->xconfigurerequest.height >= display->height)) {
				window->fullscreen = true;
				XWindowChanges window_changes = (XWindowChanges) {.x = 0, .y = 0, .width = display->width, .height = display->height};
				XConfigureWindow(display->x_display, window->x_window, CWX | CWY | CWWidth | CWHeight, &window_changes);
				return;
			}

			if (window->maximized) {
				XWindowChanges window_changes = (XWindowChanges) {.x = 0, .y = 0, .width = display->width, .height = display->height};
				XConfigureWindow(display->x_display, window->x_window, CWX | CWY | CWWidth | CWHeight, &window_changes);
				return;
			}

			if (event->xconfigurerequest.value_mask & (CWX | CWY | CWWidth | CWHeight)) {
				window->fullscreen = false;
				if (event->xconfigurerequest.value_mask & CWX) window->saved_position_x = event->xconfigurerequest.x;
				if (event->xconfigurerequest.value_mask & CWY) window->saved_position_y = event->xconfigurerequest.y;
				if (event->xconfigurerequest.value_mask & CWWidth) window->saved_width = event->xconfigurerequest.width;
				if (event->xconfigurerequest.value_mask & CWHeight) window->saved_height = event->xconfigurerequest.height;
				XWindowChanges window_changes = (XWindowChanges) {.x = window->saved_position_x, .y = window->saved_position_y, .width = window->saved_width, .height = window->saved_height};
				XConfigureWindow(display->x_display, window->x_window, event->xconfigurerequest.value_mask & (CWX | CWY | CWWidth | CWHeight), &window_changes);
				return;
			}
		}
	}
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
		if (attributes.x != 0 || attributes.y != 0 || attributes.width != 0 || attributes.height != 0) {
			if (attributes.x != 0) window->saved_position_x = attributes.x;
			if (attributes.y != 0) window->saved_position_y = attributes.y;
			if (attributes.width != 0) window->saved_width = attributes.width;
			if (attributes.height != 0) window->saved_height = attributes.height;
			XMoveResizeWindow(display->x_display, window->x_window, window->saved_position_x, window->saved_position_y, window->saved_width, window->saved_height);
		}
	}

	XSelectInput(display->x_display, window->x_window, EnterWindowMask | LeaveWindowMask | FocusChangeMask | PropertyChangeMask | ResizeRedirectMask | StructureNotifyMask);

	uint const modifiers[] = {0, LockMask, display->numlockmask, LockMask | display->numlockmask};
	for (unsigned i = 0; i < 4; ++i) {
		XGrabButton(display->x_display, Button1, modifiers[i], window->x_window, false, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
		XGrabButton(display->x_display, Button1, Mod4Mask | modifiers[i], window->x_window, false, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
		XGrabButton(display->x_display, Button1, Mod4Mask | ControlMask | modifiers[i], window->x_window, false, ButtonPressMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	}

	sl_focus_and_raise_window(display, index, CurrentTime);
}

void sl_map_request (sl_display* display, XEvent* event) {
	/*
	 "The X server can report MapRequest events to clients wanting information about
	  a different client's desire to map windows. A window is considered mapped when
	  a map window request completes. The X server generates this event whenever a
	  different client initiates a map window request on an unmapped window whose
	  override_redirect member is set to False. Clients initiate map window requests by
	  calling XMapWindow, XMapRaised, or XMapSubwindows."
	*/

	for (size_t i = 0; i < display->windows->size; ++i) {
		sl_window* const window = sl_window_at(display, i);

		if (window->x_window == event->xmaprequest.window) {
			if (window->started)
				return map_started_window(display, i);
			else
				return map_unstarted_window(display, i);
		}
	}
}

void sl_resize_request (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {
	/*
	 "The X server can report ResizeRequest events to clients wanting information about
	  another client's attempts to change the size of a window. The X server generates
	  this event whenever some other client attempts to change the size of the specified
	  window by calling XConfigureWindow, XResizeWindow, or XMoveResizeWindow."
	*/
}

#ifdef D_notify_properties
#	define property_log(M_name) warn_log("Property: " M_name)
#else
#	define property_log(M_name)
#endif

void sl_property_notify (sl_display* display, XEvent* event) {
	if (event->xproperty.state == PropertyDelete) {
		property_log("delete");
		return;
	}

	if (event->xproperty.window == display->root) return; // do nothing

	sl_window* window = NULL;

	for (size_t i = 0;; ++i) {
		sl_window* const test_window = sl_window_at(display, i);

		if (test_window->x_window == event->xproperty.window) {
			window = test_window;
			break;
		}

		if (i == display->windows->size - 1) return;
	}

	// start of icccm spec:
	if (event->xproperty.atom == XA_WM_NAME) {
		property_log("WM_NAME");
		/*
		  The WM_NAME property is an uninterpreted string that the client wants the window manager to display in association with the window (for example, in a window headline bar).]
		*/
	} else if (event->xproperty.atom == XA_WM_ICON_NAME) {
		property_log("WM_ICON_NAME");
		/* The WM_ICON_NAME property is an uninterpreted string that the client wants to be displayed in association with the window when it is iconified (for example, in an icon label). In other respects, including the type, it is similar to WM_NAME. For obvious geometric reasons, fewer characters will normally be visible in WM_ICON_NAME than WM_NAME.
		 */
	} else if (event->xproperty.atom == XA_WM_NORMAL_HINTS) {
		property_log("WM_NORMAL_HINTS");
		XSizeHints hints;
		long supplied_return = 0;
		warn_log("ignoring the return value of a function that returns Status");
		XGetWMNormalHints(display->x_display, window->x_window, &hints, &supplied_return);

		/*
		  The WM_SIZE_HINTS.flags bit definitions are as follows:

		    USPosition   User-specified x, y
		    USSize       User-specified width, height
		    PPosition    Program-specified position
		    PSize        Program-specified size
		    PMinSize     Program-specified minimum size
		    PMaxSize     Program-specified maximum size
		    PResizeInc   Program-specified resize increments
		    PAspect      Program-specified min and max aspect ratios
		    PBaseSize    Program-specified base size
		    PWinGravity  Program-specified window gravity
		*/

		if (hints.flags & USPosition) {
			warn_log("TODO: USPosition");
		}
		if (hints.flags & USSize) {
			warn_log("TODO: USSize");
		}
		if (hints.flags & PPosition) {
			warn_log("ignoring obsolete functionality");
		}
		if (hints.flags & PSize) {
			warn_log("ignoring obsolete functionality");
		}
		if (hints.flags & PMinSize) {
			warn_log("TODO: PMinSize");
		}
		if (hints.flags & PMaxSize) {
			warn_log("TODO: PMaxSize");
		}
		if (hints.flags & PResizeInc) {
			warn_log("TODO: PResizeInc");
		}
		if (hints.flags & PAspect) {
			warn_log("TODO: PAspect");
		}
		if (hints.flags & PBaseSize) {
			warn_log("TODO: PBaseSize");
		}
		if (hints.flags & PWinGravity) {
			warn_log("TODO: PWinGravity");
		}

		// Extended Window Manager Hints:
		/* Windows can indicate that they are non-resizable by setting minheight = maxheight and minwidth = maxwidth in the ICCCM WM_NORMAL_HINTS property. The Window Manager MAY decorate such windows differently.
		 */
	} else if (event->xproperty.atom == XA_WM_HINTS) {
		property_log("WM_HINTS");
		/*
		  The WM_HINTS.flags bit definitions are as follows:
		    InputHint         input
		    StateHint         initial_state
		    IconPixmapHint    icon_pixmap
		    IconWindowHint    icon_window
		    IconPositionHint  icon_x & icon_y
		    IconMaskHint      icon_mask
		    WindowGroupHint  	window_group
		    MessageHint       (this bit is obsolete)
		    UrgencyHint       urgency
		*/

		// Extended Window Manager Hints:
		/* Windows expecting immediate user action should indicate this using the urgency bit in the WM_HINTS.flags property, as defined in the ICCCM. */

	} else if (event->xproperty.atom == XA_WM_CLASS) {
		property_log("WM_CLASS");
		/*
		  The WM_CLASS property (of type STRING without control characters) contains two consecutive null-terminated strings. These specify the Instance and Class names to be used by both the client and the window manager for looking up resources for the application or as identifying information. This property must be present when the window leaves the Withdrawn state and may be changed only while the window is in the Withdrawn state. Window managers may examine the property only when they start up
		  and when the window leaves the Withdrawn state, but there should be no need for a client to change its state dynamically.
		*/
	} else if (event->xproperty.atom == XA_WM_TRANSIENT_FOR) {
		property_log("WM_TRANSIENT_FOR");
		/*
		  The WM_TRANSIENT_FOR property (of type WINDOW) contains the ID of another top-level window. The implication is that this window is a pop-up on behalf of the named window, and window managers may decide not to decorate transient windows or may treat them differently in other ways. In particular, window managers should present newly mapped WM_TRANSIENT_FOR windows without requiring any user interaction, even if mapping top-level windows normally does require interaction. Dialogue boxes, for
		  example, are an example of windows that should have WM_TRANSIENT_FOR set.
		*/
	} else if (event->xproperty.atom == display->atoms[wm_protocols]) {
		property_log("WM_PROTOCOLS");
		/* The WM_PROTOCOLS property (of type ATOM) is a list of atoms. Each atom identifies a communication protocol between the client and the window manager in which the client is willing to participate. Atoms can identify both standard protocols and private protocols specific to individual window managers. */

		// NOTE: WM_PROTOCOLS and WM_COLORMAP_WINDOWS are not predefined for _some_ reason
	} else if (event->xproperty.atom == display->atoms[wm_colormap_windows]) {
		property_log("WM_COLORMAP_WINDOWS");
		/* The WM_COLORMAP_WINDOWS property (of type WINDOW) on a top-level window is a list of the IDs of windows that may need colormaps installed that differ from the colormap of the top-level window. The window manager will watch this list of windows for changes in their colormap attributes. The top-level window is always (implicitly or explicitly) on the watch list. For the details of this mechanism, see Colormaps */

		// NOTE: WM_PROTOCOLS and WM_COLORMAP_WINDOWS are not predefined for _some_ reason
	} else if (event->xproperty.atom == XA_WM_CLIENT_MACHINE) {
		property_log("WM_CLIENT_MACHINE");
		/* The client should set the WM_CLIENT_MACHINE property (of one of the TEXT types) to a string that forms the name of the machine running the client as seen from the machine running the server. */
	}

	// start of extended window manager hints:

	else if (event->xproperty.atom == display->atoms[net_wm_name]) {
		property_log("_NET_WM_NAME");
		/* The Client SHOULD set this to the title of the window in UTF-8 encoding. If set, the Window Manager should use this in preference to WM_NAME. */
	} else if (event->xproperty.atom == display->atoms[net_wm_icon_name]) {
		property_log("_NET_WM_ICON_NAME");
		/* The Client SHOULD set this to the title of the icon for this window in UTF-8 encoding. If set, the Window Manager should use this in preference to WM_ICON_NAME. */
	} else if (event->xproperty.atom == display->atoms[net_wm_desktop]) {
		property_log("_NET_WM_DESKTOP");
		/* Cardinal to determine the desktop the window is in (or wants to be) starting with 0 for the first desktop. A Client MAY choose not to set this property, in which case the Window Manager SHOULD place it as it wishes. 0xFFFFFFFF indicates that the window SHOULD appear on all desktops. */
	} else if (event->xproperty.atom == display->atoms[net_wm_window_type]) {
		property_log("_NET_WM_WINDOW_TYPE");
		/*
		  his SHOULD be set by the Client before mapping to a list of atoms indicating the functional type of the window. This property SHOULD be used by the window manager in determining the decoration, stacking position and other behavior of the window. The Client SHOULD specify window types in order of preference (the first being most preferable) but MUST include at least one of the basic window type atoms from the list below. This is to allow for extension of the list of types whilst providing
		  default behavior for Window Managers that do not recognize the extensions.
		*/
	} else if (event->xproperty.atom == display->atoms[net_wm_state]) {
		property_log("_NET_WM_STATE");
		/* A list of hints describing the window state. Atoms present in the list MUST be considered set, atoms not present in the list MUST be considered not set. The Window Manager SHOULD honor _NET_WM_STATE whenever a withdrawn window requests to be mapped. A Client wishing to change the state of a window MUST send a _NET_WM_STATE client message to the display->root window (see below). The Window Manager MUST keep this property updated to reflect the current state of the window. */

		int di = 0;
		ulong dl = 0;
		uchar* p = NULL;
		Atom da = None, atom = None;

		XGetWindowProperty(display->x_display, event->xproperty.window, display->atoms[net_wm_state], 0, sizeof(Atom), false, XA_ATOM, &da, &di, &dl, &dl, &p);
		if (!p) return;
		atom = *(Atom*)p;
		XFree(p);

		/*
		  Possible atoms are:

		    _NET_WM_STATE_MODAL, ATOM
		    _NET_WM_STATE_STICKY, ATOM
		    _NET_WM_STATE_MAXIMIZED_VERT, ATOM
		    _NET_WM_STATE_MAXIMIZED_HORZ, ATOM
		    _NET_WM_STATE_SHADED, ATOM
		    _NET_WM_STATE_SKIP_TASKBAR, ATOM
		    _NET_WM_STATE_SKIP_PAGER, ATOM
		    _NET_WM_STATE_HIDDEN, ATOM
		    _NET_WM_STATE_FULLSCREEN, ATOM
		    _NET_WM_STATE_ABOVE, ATOM
		    _NET_WM_STATE_BELOW, ATOM
		    _NET_WM_STATE_DEMANDS_ATTENTION, ATOM
		    _NET_WM_STATE_FOCUSED, ATOM
		*/

		if (atom == display->atoms[net_wm_state_modal]) {
			// _NET_WM_STATE_MODAL indicates that this is a modal dialog box. If the WM_TRANSIENT_FOR hint is set to another toplevel window, the dialog is modal for that window; if WM_TRANSIENT_FOR is not set or set to the display->root window the dialog is modal for its window group.
		} else if (atom == display->atoms[net_wm_state_sticky]) {
			// _NET_WM_STATE_STICKY indicates that the Window Manager SHOULD keep the window's position fixed on the screen, even when the virtual desktop scrolls.
		} else if (atom == display->atoms[net_wm_state_maximized_vert]) {
			// _NET_WM_STATE_MAXIMIZED_VERT indicates that the window is vertically maximized.
		} else if (atom == display->atoms[net_wm_state_maximized_horz]) {
			// _NET_WM_STATE_MAXIMIZED_HORZ indicates that the window is horizontally maximized.
		} else if (atom == display->atoms[net_wm_state_shaded]) {
			// _NET_WM_STATE_SHADED indicates that the window is shaded.
		} else if (atom == display->atoms[net_wm_state_skip_taskbar]) {
			// _NET_WM_STATE_SKIP_TASKBAR indicates that the window should not be included on a taskbar. This hint should be requested by the application, i.e. it indicates that the window by nature is never in the taskbar. Applications should not set this hint if _NET_WM_WINDOW_TYPE already conveys the exact nature of the window.
		} else if (atom == display->atoms[net_wm_state_skip_pager]) {
			// _NET_WM_STATE_SKIP_PAGER indicates that the window should not be included on a Pager. This hint should be requested by the application, i.e. it indicates that the window by nature is never in the Pager. Applications should not set this hint if _NET_WM_WINDOW_TYPE already conveys the exact nature of the window.
		} else if (atom == display->atoms[net_wm_state_hidden]) {
			// _NET_WM_STATE_HIDDEN should be set by the Window Manager to indicate that a window would not be visible on the screen if its desktop/viewport were active and its coordinates were within the screen bounds. The canonical example is that minimized windows should be in the _NET_WM_STATE_HIDDEN state. Pagers and similar applications should use _NET_WM_STATE_HIDDEN instead of WM_STATE to decide whether to display a window in miniature representations of the windows on a desktop.

			// Implementation note: if an Application asks to toggle _NET_WM_STATE_HIDDEN the Window Manager should probably just ignore the request, since _NET_WM_STATE_HIDDEN is a function of some other aspect of the window such as minimization, rather than an independent state.
		} else if (atom == display->atoms[net_wm_state_fullscreen]) {
			// _NET_WM_STATE_FULLSCREEN indicates that the window should fill the entire screen and have no window decorations. Additionally the Window Manager is responsible for restoring the original geometry after a switch from fullscreen back to normal window. For example, a presentation program would use this hint.
			window->fullscreen = true;
			window->saved_position_x = 0;
			window->saved_position_y = 0;
			window->saved_width = display->width;
			window->saved_height = display->height;
		} else if (atom == display->atoms[net_wm_state_above]) {
			// _NET_WM_STATE_ABOVE indicates that the window should be on top of most windows (see the section called “Stacking order” for details).

			// _NET_WM_STATE_ABOVE and _NET_WM_STATE_BELOW are mainly meant for user preferences and should not be used by applications e.g. for drawing attention to their dialogs (the Urgency hint should be used in that case, see the section called “Urgency”).'
		} else if (atom == display->atoms[net_wm_state_below]) {
			// _NET_WM_STATE_BELOW indicates that the window should be below most windows (see the section called “Stacking order” for details).

			// _NET_WM_STATE_ABOVE and _NET_WM_STATE_BELOW are mainly meant for user preferences and should not be used by applications e.g. for drawing attention to their dialogs (the Urgency hint should be used in that case, see the section called “Urgency”).'
		} else if (atom == display->atoms[net_wm_state_demands_attention]) {
			// _NET_WM_STATE_DEMANDS_ATTENTION indicates that some action in or with the window happened. For example, it may be set by the Window Manager if the window requested activation but the Window Manager refused it, or the application may set it if it finished some work. This state may be set by both the Client and the Window Manager. It should be unset by the Window Manager when it decides the window got the required attention (usually, that it got activated).
		} else if (atom == display->atoms[net_wm_state_focused]) {
			// _NET_WM_STATE_FOCUSED indicates whether the window's decorations are drawn in an active state. Clients MUST regard it as a read-only hint. It cannot be set at map time or changed via a _NET_WM_STATE client message. The window given by _NET_ACTIVE_WINDOW will usually have this hint, but at times other windows may as well, if they have a strong association with the active window and will be considered as a unit with it by the user. Clients that modify the appearance of internal
			// elements when a toplevel has keyboard focus SHOULD check for the availability of this state in _NET_SUPPORTED and, if it is available, use it in preference to tracking focus via FocusIn events. By doing so they will match the window decorations and accurately reflect the intentions of the Window Manager.
		}
	} else if (event->xproperty.atom == display->atoms[net_wm_strut]) {
		property_log("_NET_WM_STRUT");
		// This property is equivalent to a _NET_WM_STRUT_PARTIAL property where all start values are 0 and all end values are the height or width of the logical screen. _NET_WM_STRUT_PARTIAL was introduced later than _NET_WM_STRUT, however, so clients MAY set this property in addition to _NET_WM_STRUT_PARTIAL to ensure backward compatibility with Window Managers supporting older versions of the Specification.
	} else if (event->xproperty.atom == display->atoms[net_wm_strut_partial]) {
		property_log("_NET_WM_STRUT_PARTIAL");
		// This property MUST be set by the Client if the window is to reserve space at the edge of the screen. The property contains 4 cardinals specifying the width of the reserved area at each border of the screen, and an additional 8 cardinals specifying the beginning and end corresponding to each of the four struts. The order of the values is left, right, top, bottom, left_start_y, left_end_y, right_start_y, right_end_y, top_start_x, top_end_x, bottom_start_x, bottom_end_x. All coordinates
		// are display->root window coordinates. The client MAY change this property at any time, therefore the Window Manager MUST watch for property notify events if the Window Manager uses this property to assign special semantics to the window.
	} else if (event->xproperty.atom == display->atoms[net_wm_icon_geometry]) {
		property_log("_NET_WM_ICON_GEOMETRY");
		// This optional property MAY be set by stand alone tools like a taskbar or an iconbox. It specifies the geometry of a possible icon in case the window is iconified.
	} else if (event->xproperty.atom == display->atoms[net_wm_icon]) {
		property_log("_NET_WM_ICON");
		// This is an array of possible icons for the client. This specification does not stipulate what size these icons should be, but individual desktop environments or toolkits may do so. The Window Manager MAY scale any of these icons to an appropriate size.
	} else if (event->xproperty.atom == display->atoms[net_wm_pid]) {
		property_log("_NET_WM_PID");
		// If set, this property MUST contain the process ID of the client owning this window. This MAY be used by the Window Manager to kill windows which do not respond to the _NET_WM_PING protocol.
	} else if (event->xproperty.atom == display->atoms[net_wm_allowed_actions]) {
		property_log("_NET_WM_ALLOWED_ACTIONS");
		// A list of atoms indicating user operations that the Window Manager supports for this window. Atoms present in the list indicate allowed actions, atoms not present in the list indicate actions that are not supported for this window. The Window Manager MUST keep this property updated to reflect the actions which are currently "active" or "sensitive" for a window. Taskbars, Pagers, and other tools use _NET_WM_ALLOWED_ACTIONS to decide which actions should be made available to the user.
	} else if (event->xproperty.atom == display->atoms[net_wm_handled_icons]) {
		property_log("_NET_WM_HANDLED_ICONS");
		// This property can be set by a Pager on one of its own toplevel windows to indicate that the Window Manager need not provide icons for iconified windows, for example if it is a taskbar and provides buttons for iconified windows.
	} else if (event->xproperty.atom == display->atoms[net_wm_user_time]) {
		property_log("_NET_WM_USER_TIME");
		// This property contains the XServer time at which last user activity in this window took place.

		// Clients should set this property on every new toplevel window (or on the window pointed out by the _NET_WM_USER_TIME_WINDOW property), before mapping the window, to the timestamp of the user interaction that caused the window to appear. A client that only deals with core events, might, for example, use the timestamp of the last KeyPress or ButtonPress event-> ButtonRelease and KeyRelease events should not generally be considered to be user interaction, because an application may
		// receive KeyRelease events from global keybindings, and generally release events may have later timestamp than actions that were triggered by the matching press events. Clients can obtain the timestamp that caused its first window to appear from the DESKTOP_STARTUP_ID environment variable, if the app was launched with startup notification. If the client does not know the timestamp of the user interaction that caused the first window to appear (e.g. because it was not launched with
		// startup notification), then it should not set the property for that window. The special value of zero on a newly mapped window can be used to request that the window not be initially focused when it is mapped.

		// If the client has the active window, it should also update this property on the window whenever there's user activity.

		// Rationale: This property allows a Window Manager to alter the focus, stacking, and/or placement behavior of windows when they are mapped depending on whether the new window was created by a user action or is a "pop-up" window activated by a timer or some other event->
	} else if (event->xproperty.atom == display->atoms[net_wm_user_time_window]) {
		property_log("_NET_WM_USER_TIME_WINDOW");
		// This property contains the XID of a window on which the client sets the _NET_WM_USER_TIME property. Clients should check whether the window manager supports _NET_WM_USER_TIME_WINDOW and fall back to setting the _NET_WM_USER_TIME property on the toplevel window if it doesn't.

		// Rationale: Storing the frequently changing _NET_WM_USER_TIME property on the toplevel window itx_window causes every application that is interested in any of the properties of that window to be woken up on every keypress, which is particularly bad for laptops running on battery power.
	} else if (event->xproperty.atom == display->atoms[net_wm_opaque_region]) {
		property_log("_NET_WM_OPAQUE_REGION");
		// The Client MAY set this property to a list of 4-tuples [x, y, width, height], each representing a rectangle in window coordinates where the pixels of the window's contents have a fully opaque alpha value. If the window is drawn by the compositor without adding any transparency, then such a rectangle will occlude whatever is drawn behind it. When the window has an RGB visual rather than an ARGB visual, this property is not typically useful, since the effective opaque region of a window
		// is exactly the bounding region of the window as set via the shape extension. For windows with an ARGB visual and also a bounding region set via the shape extension, the effective opaque region is given by the intersection of the region set by this property and the bounding region set via the shape extension. The compositing manager MAY ignore this hint.

		// Rationale: This gives the compositing manager more room for optimizations. For example, it can avoid drawing occluded portions behind the window.
	} else if (event->xproperty.atom == display->atoms[net_wm_bypass_compositor]) {
		property_log("_NET_WM_BYPASS_COMPOSITOR");
		// The Client MAY set this property to hint the compositor that the window would benefit from running uncomposited (i.e not redirected offscreen) or that the window might be hurt from being uncomposited. A value of 0 indicates no preference. A value of 1 hints the compositor to disabling compositing of this window. A value of 2 hints the compositor to not disabling compositing of this window. All other values are reserved and should be treated the same as a value of 0. The compositing
		// manager MAY bypass compositing for both fullscreen and non-fullscreen windows if bypassing is requested, but MUST NOT bypass if it would cause differences from the composited appearance.
	} else {
		warn_log("\"default case\" reached at the PropertyNotify \"switch\"");
	}
}

// empty mask events
void sl_client_message (sl_display* display, XEvent* event) {
	if (event->xclient.message_type == display->atoms[net_wm_state]) {
		if ((ulong)event->xclient.data.l[1] == display->atoms[net_wm_state_fullscreen] || (ulong)event->xclient.data.l[2] == display->atoms[net_wm_state_fullscreen]) {
#ifdef D_debug
			warn_log("Client Message: fullscreen\n");
#endif
			for (size_t i = 0; i < display->windows->size; ++i) {
				sl_window* const window = sl_window_at(display, i);

				if (window->x_window == event->xclient.window) {
					XConfigureEvent configure_event;
					if (!window->fullscreen && (event->xclient.data.l[0] == 1 || event->xclient.data.l[0] == 2)) {
						window->fullscreen = true;
						XChangeProperty(display->x_display, window->x_window, display->atoms[net_wm_state], XA_ATOM, 32, PropModeReplace, (uchar*)&display->atoms[net_wm_state_fullscreen], true);
						{
							XWindowAttributes attributes;
							XGetWindowAttributes(display->x_display, window->x_window, &attributes);
							window->saved_position_x = attributes.x;
							window->saved_position_y = attributes.y;
							window->saved_width = attributes.width;
							window->saved_height = attributes.height;
						}
						XMoveResizeWindow(display->x_display, window->x_window, 0, 0, display->width, display->height);
						XRaiseWindow(display->x_display, window->x_window);

						configure_event.type = ConfigureNotify;
						configure_event.display = display->x_display;
						configure_event.event = window->x_window;
						configure_event.window = window->x_window;
						configure_event.x = 0;
						configure_event.y = 0;
						configure_event.width = display->width;
						configure_event.height = display->height;
						// NOTE: no window decoration support currently
						warn_log("the sibling window?");
						configure_event.override_redirect = false;
						warn_log("what");
						sl_window* const raised_window = sl_raised_window(display);
						XSendEvent(display->x_display, raised_window->x_window, false, StructureNotifyMask, (XEvent*)&configure_event);
					} else if (window->fullscreen && (event->xclient.data.l[0] == 0 || event->xclient.data.l[0] == 2)) {
						window->fullscreen = false;
						XChangeProperty(display->x_display, window->x_window, display->atoms[net_wm_state], XA_ATOM, 32, PropModeReplace, (uchar*)&display->atoms[net_wm_state_fullscreen], false);
						XMoveResizeWindow(display->x_display, window->x_window, window->saved_position_x, window->saved_position_y, window->saved_width, window->saved_height);

						sl_window* const raised_window = sl_raised_window(display);
						configure_event.type = ConfigureNotify;
						configure_event.display = display->x_display;
						configure_event.event = raised_window->x_window;
						configure_event.window = raised_window->x_window;
						configure_event.x = raised_window->saved_position_x;
						configure_event.y = raised_window->saved_position_y;
						configure_event.width = raised_window->saved_width;
						configure_event.height = raised_window->saved_height;
						// NOTE: no window decoration support currently
						warn_log("the sibling window?");
						configure_event.override_redirect = false;
						warn_log("what");
						XSendEvent(display->x_display, raised_window->x_window, false, StructureNotifyMask, (XEvent*)&configure_event);
					}
					break;
				}
			}
		}
	} else if (event->xclient.message_type == display->atoms[net_active_window]) {
	}
}

void sl_mapping_notify (sl_display* display, XEvent* event) {
	XRefreshKeyboardMapping(&event->xmapping);
	if (event->xmapping.request == MappingKeyboard) sl_grab_keys(display);
}

void sl_selection_clear (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {}

void sl_selection_notify (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {}

void sl_selection_request (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {}

void sl_focus_in (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {
	/*
	  This section describes the processing that occurs for the input focus events FocusIn
	  and FocusOut. The X server can report FocusIn or FocusOut events to clients
	  wanting information about when the input focus changes. The keyboard is always
	  attached to some window (typically, the root window or a top-level window), which is
	  called the focus window. The focus window and the position of the pointer determine
	  the window that receives keyboard input. Clients may need to know when the input
	  focus changes to control highlighting of areas on the screen.
	*/
}

void sl_focus_out (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {
	/*
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
// NOTE: XGrabKey(...) implies KeyPressMask and (possibly) KeyReleaseMask
void sl_key_press (sl_display* display, XEvent* event) {
	display->user_input_since_last_workspace_change = true;

	if (parse_mask_long(event->xkey.state) == 0) { // {k}
		switch (XLookupKeysym(&event->xkey, 0)) {
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

	if (parse_mask_long(event->xkey.state) == ShiftMask) { // shift + {k}
		switch (XLookupKeysym(&event->xkey, 0)) {
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

	if (parse_mask_long(event->xkey.state) == ControlMask) { // control + {k}
		switch (XLookupKeysym(&event->xkey, 0)) {
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

	if (parse_mask_long(event->xkey.state) == Mod4Mask) { // super + {k}
		switch (XLookupKeysym(&event->xkey, 0)) {
			// meta
		case XK_w: // logout
			if (window_manager()->logout) {
				warn_log("Meta: exiting before waiting for all windows to close themselves\n");
				exit(-1);
			}

			window_manager()->logout = true;

			return sl_delete_all_windows(display, event->xkey.time);

		// window manipulation
		case XK_m: // maximize
			return sl_maximize_raised_window(display);

		case XK_c: // close
			return sl_close_raised_window(display, event->xkey.time);

		case XK_Tab: // cycle the windows
			return sl_cycle_windows_up(display, event->xkey.time);

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
			return sl_next_workspace(display, event->xkey.time);

		case XK_Left: // switch to workspace to the left
			return sl_previous_workspace(display, event->xkey.time);

		case XK_0: // switch to workspace 10
			return sl_switch_to_workspace(display, 9, event->xkey.time);

		case XK_1: // switch to workspace 1
			return sl_switch_to_workspace(display, 0, event->xkey.time);

		case XK_2: // switch to workspace 2
			return sl_switch_to_workspace(display, 1, event->xkey.time);

		case XK_3: // switch to workspace 3
			return sl_switch_to_workspace(display, 2, event->xkey.time);

		case XK_4: // switch to workspace 4
			return sl_switch_to_workspace(display, 3, event->xkey.time);

		case XK_5: // switch to workspace 5
			return sl_switch_to_workspace(display, 4, event->xkey.time);

		case XK_6: // switch to workspace 6
			return sl_switch_to_workspace(display, 5, event->xkey.time);

		case XK_7: // switch to workspace 7
			return sl_switch_to_workspace(display, 6, event->xkey.time);

		case XK_8: // switch to workspace 8
			return sl_switch_to_workspace(display, 7, event->xkey.time);

		case XK_9: // switch to workspace 9
			return sl_switch_to_workspace(display, 8, event->xkey.time);

		case XK_KP_Add: // add a workspace
			return sl_push_workspace(display);

		case XK_KP_Subtract: // remove the last workspace
			return sl_pop_workspace(display, event->xkey.time);

		default: assert_not_reached(); return;
		}
	}

	if (parse_mask_long(event->xkey.state) == Mod1Mask) { // alt + {k}
		switch (XLookupKeysym(&event->xkey, 0)) {
		case XK_Tab: // cycle the windows
			return sl_cycle_windows_up(display, event->xkey.time);

		default: assert_not_reached();
		}
	}

	if (parse_mask_long(event->xkey.state) == (ShiftMask | Mod4Mask)) { // shift + super + {k}
		switch (XLookupKeysym(&event->xkey, 0)) {
		case XK_Tab: // cycle the windows (reverse)
			return sl_cycle_windows_down(display, event->xkey.time);

		default: assert_not_reached();
		}
	}

	if (parse_mask_long(event->xkey.state) == (Mod4Mask | ControlMask)) { // control + super + {k}
		switch (XLookupKeysym(&event->xkey, 0)) {
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
			return sl_next_workspace_with_raised_window(display, event->xkey.time);
		case XK_Left: // switch to workspace to the left while carrying the top window
			return sl_previous_workspace_with_raised_window(display, event->xkey.time);

		default: assert_not_reached();
		}
	}

	if (parse_mask_long(event->xkey.state) == (ShiftMask | Mod1Mask)) { // shift + alt + {k}
		switch (XLookupKeysym(&event->xkey, 0)) {
		case XK_Tab: // cycle the windows (reverse)
			return sl_cycle_windows_down(display, event->xkey.time);

		default: assert_not_reached();
		}
	}

	assert_not_reached();
}

void sl_key_release (M_maybe_unused sl_display* display, M_maybe_unused XEvent* event) {}
