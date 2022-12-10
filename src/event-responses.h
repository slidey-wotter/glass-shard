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

#include <X11/Xlib.h>
typedef struct sl_display sl_display; // foward declaration

extern void sl_button_press (sl_display* display, XButtonPressedEvent* event);
extern void sl_button_release (sl_display* display, XButtonReleasedEvent* event);
extern void sl_enter_notify (sl_display* display, XEnterWindowEvent* event);
extern void sl_leave_notify (sl_display* display, XLeaveWindowEvent* event);
extern void sl_motion_notify (sl_display* display, XPointerMovedEvent* event);
extern void sl_circulate_notify (sl_display* display, XCirculateEvent* event);
extern void sl_configure_notify (sl_display* display, XConfigureEvent* event);
extern void sl_create_notify (sl_display* display, XCreateWindowEvent* event);
extern void sl_destroy_notify (sl_display* display, XDestroyWindowEvent* event);
extern void sl_gravity_notify (sl_display* display, XGravityEvent* event);
extern void sl_map_notify (sl_display* display, XMapEvent* event);
extern void sl_reparent_notify (sl_display* display, XReparentEvent* event);
extern void sl_unmap_notify (sl_display* display, XUnmapEvent* event);
extern void sl_circulate_request (sl_display* display, XCirculateRequestEvent* event);
extern void sl_configure_request (sl_display* display, XConfigureRequestEvent* event);
extern void sl_map_request (sl_display* display, XMapRequestEvent* event);
extern void sl_resize_request (sl_display* display, XResizeRequestEvent* event);
extern void sl_property_notify (sl_display* display, XPropertyEvent* event);
extern void sl_client_message (sl_display* display, XClientMessageEvent* event);
extern void sl_mapping_notify (sl_display* display, XMappingEvent* event);
extern void sl_selection_clear (sl_display* display, XSelectionClearEvent* event);
extern void sl_selection_request (sl_display* display, XSelectionRequestEvent* event);
extern void sl_selection_notify (sl_display* display, XSelectionEvent* event);
extern void sl_focus_in (sl_display* display, XFocusInEvent* event);
extern void sl_focus_out (sl_display* display, XFocusOutEvent* event);
extern void sl_key_press (sl_display* display, XKeyPressedEvent* event);
extern void sl_key_release (sl_display* display, XKeyReleasedEvent* event);
