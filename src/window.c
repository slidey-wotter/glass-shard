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

#include "window.h"

#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "compiler-differences.h"
#include "display.h"
#include "window-mutable.h"

void sl_window_start (sl_window* window) { ((sl_window_mutable*)window)->started = true; }

void sl_window_destroy (sl_window* window) {
	if (window->name.data) free(((sl_window_mutable*)window)->name.data);
	if (window->icon_name.data) free(((sl_window_mutable*)window)->icon_name.data);
	if (window->net_wm_name.data) free(((sl_window_mutable*)window)->net_wm_name.data);
	if (window->net_wm_visible_name.data) free(((sl_window_mutable*)window)->net_wm_visible_name.data);
	if (window->net_wm_icon_name.data) free(((sl_window_mutable*)window)->net_wm_icon_name.data);
	if (window->net_wm_visible_icon_name.data) free(((sl_window_mutable*)window)->net_wm_visible_icon_name.data);
}

void sl_window_swap (sl_window* lhs, sl_window* rhs) {
	sl_window_mutable temp = *(sl_window_mutable*)lhs;
	*(sl_window_mutable*)lhs = *(sl_window_mutable*)rhs;
	*(sl_window_mutable*)rhs = temp;
}

/*
  Inter-Client Communication Conventions Manual: Chapter 4. Client-to-Window-Manager Communication: Client's Actions: Client Properties:

  Once the client has one or more top-level windows, it should place properties
  on those windows to inform the window manager of the behavior that the client
  desires. Window managers will assume values they find convenient for any of these
  properties that are not supplied; clients that depend on particular values must
  explicitly supply them. The window manager will not change properties written by
  the client.

  The window manager will examine the contents of these properties when the
  window makes the transition from the Withdrawn state and will monitor some
  properties for changes while the window is in the Iconic or Normal state. When the
  client changes one of these properties, it must use Replace mode to overwrite the
  entire property with new data; the window manager will retain no memory of the
  old value of the property. All fields of the property must be set to suitable values in a
  single Replace mode ChangeProperty request. This ensures that the full contents of
  the property will be available to a new window manager if the existing one crashes,
  if it is shut down and restarted, or if the session needs to be shut down and restarted
  by the session manager.

    Convention

    Clients writing or rewriting window manager properties must ensure
    that the entire content of each property remains valid at all times.

  Some of these properties may contain the IDs of resources, such as windows or
  pixmaps. Clients should ensure that these resources exist for at least as long as the
  window on which the property resides.

  If these properties are longer than expected, clients should ignore the remainder of
  the property. Extending these properties is reserved to the X Consortium; private
  extensions to them are forbidden. Private additional communication between
  clients and window managers should take place using separate properties. The
  only exception to this rule is the WM_PROTOCOLS property, which may be of
  arbitrary length and which may contain atoms representing private protocols (see
  WM_PROTOCOLS Property ).

  The next sections describe each of the properties the clients need to set, in turn.
  They are summarized in the table in Summary of Window Manager Property Types
*/

static void window_set_text_property (sl_window* window, sl_display* display, Atom atom, struct sl_sized_string_mutable* sized_string) {
	XTextProperty text_property;

	XGetTextProperty(display->x_display, window->x_window, &text_property, atom);

	if (sized_string->data) free(sized_string->data);

	warn_log("ignoring encoding and format");
	sized_string->size = text_property.nitems;
	sized_string->data = malloc(sizeof(uchar) * sized_string->size);
	memcpy(sized_string->data, text_property.value, sized_string->size);
}

void sl_set_window_name (sl_window* window, sl_display* display) {
	/*
	  The WM_NAME property is an uninterpreted string that the client wants the window
	  manager to display in association with the window (for example, in a window
	  headline bar).

	  The encoding used for this string (and all other uninterpreted string properties) is
	  implied by the type of the property. The type atoms to be used for this purpose are
	  described in TEXT Properties.

	  Window managers are expected to make an effort to display this information. Simply
	  ignoring WM_NAME is not acceptable behavior. Clients can assume that at least the
	  first part of this string is visible to the user and that if the information is not visible
	  to the user, it is because the user has taken an explicit action to make it invisible.

	  On the other hand, there is no guarantee that the user can see the WM_NAME string
	  even if the window manager supports window headlines. The user may have placed
	  the headline off-screen or have covered it by other windows. WM_NAME should not
	  be used for application-critical information or to announce asynchronous changes
	  of an application's state that require timely user response. The expected uses are
	  to permit the user to identify one of a number of instances of the same client and
	  to provide the user with noncritical state information.

	  Even window managers that support headline bars will place some limit on the
	  length of the WM_NAME string that can be visible; brevity here will pay dividends.
	*/
	window_set_text_property(window, display, XA_WM_NAME, (struct sl_sized_string_mutable*)&window->name);

	warn_log_va("[%lu] name: \"%.*s\"", window->x_window, (int)window->name.size, window->name.data);
}

void sl_set_window_icon_name (sl_window* window, sl_display* display) {
	/*
	  The WM_ICON_NAME property is an uninterpreted string that the client wants to be
	  displayed in association with the window when it is iconified (for example, in an icon
	  label). In other respects, including the type, it is similar to WM_NAME. For obvious
	  geometric reasons, fewer characters will normally be visible in WM_ICON_NAME
	  than WM_NAME.

	  Clients should not attempt to display this string in their icon pixmaps or windows;
	  rather, they should rely on the window manager to do so.
	*/
	window_set_text_property(window, display, XA_WM_ICON_NAME, (struct sl_sized_string_mutable*)&window->icon_name);

	warn_log_va("[%lu] icon_name: \"%.*s\"", window->x_window, (int)window->icon_name.size, window->icon_name.data);
}

void sl_set_window_normal_hints (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  The type of the WM_NORMAL_HINTS property is WM_SIZE_HINTS. Its contents
	  are as follows:

	  Field       | Type          | Comments
	  flags       | CARD32        | (see the next table)
	  pad         | 4*CARD32      | For backwards compatibility
	  min_width   | INT32         | If missing, assume base_width
	  min_height  | INT32         | If missing, assume base_height
	  max_width   | INT32         |
	  max_height  | INT32         |
	  width_inc   | INT32         |
	  height_inc  | INT32         |
	  min_aspect  | (INT32,INT32) |
	  max_aspect  | (INT32,INT32) |
	  base_width  | INT32         | If missing, assume min_width
	  base_height | INT32         | If missing, assume min_height
	  win_gravity | INT32         | If missing, assume NorthWest

	  The WM_SIZE_HINTS.flags bit definitions are as follows:

	  Name        | Value | Field
	  USPosition  | 1     | User-specified x, y
	  USSize      | 2     | User-specified width, height
	  PPosition   | 4     | Program-specified position
	  PSize       | 8     | Program-specified size
	  PMinSize    | 16    | Program-specified minimum size
	  PMaxSize    | 32    | Program-specified maximum size
	  PResizeInc  | 64    | Program-specified resize increments
	  PAspect     | 128   | Program-specified min and max aspect ratios
	  PBaseSize   | 256   | Program-specified base size
	  PWinGravity | 512   | Program-specified window gravity

	  To indicate that the size and position of the window (when a transition from
	  the Withdrawn state occurs) was specified by the user, the client should set the
	  USPosition and USSize flags, which allow a window manager to know that the user
	  specifically asked where the window should be placed or how the window should be
	  sized and that further interaction is superfluous. To indicate that it was specified by
	  the client without any user involvement, the client should set PPosition and PSize.

	  The size specifiers refer to the width and height of the client's window excluding
	  borders.

	  The win_gravity may be any of the values specified for WINGRAVITY in the core
	  protocol except for Unmap: NorthWest (1), North (2), NorthEast (3), West (4), Center
	  (5), East (6), SouthWest (7), South (8), and SouthEast (9). It specifies how and
	  whether the client window wants to be shifted to make room for the window
	  manager frame.

	  If the win_gravity is Static, the window manager frame is positioned so that the
	  inside border of the client window inside the frame is in the same position on
	  the screen as it was when the client requested the transition from Withdrawn
	  state. Other values of win_gravity specify a window reference point. For NorthWest,
	  NorthEast, SouthWest, and SouthEast the reference point is the specified outer
	  corner of the window (on the outside border edge). For North, South, East and West
	  the reference point is the center of the specified outer edge of the window border.
	  For Center the reference point is the center of the window. The reference point
	  of the window manager frame is placed at the location on the screen where the
	  reference point of the client window was when the client requested the transition
	  from Withdrawn state.

	  The min_width and min_height elements specify the minimum size that the window
	  can be for the client to be useful. The max_width and max_height elements specify
	  the maximum size. The base_width and base_height elements in conjunction with
	  width_inc and height_inc define an arithmetic progression of preferred window
	  widths and heights for non-negative integers i and j:

	  width = base_width + ( i x width_inc )

	  height = base_height + ( j x height_inc )

	  Window managers are encouraged to use i and j instead of width and height in
	  reporting window sizes to users. If a base size is not provided, the minimum size is
	  to be used in its place and vice versa.

	  The min_aspect and max_aspect fields are fractions with the numerator first and
	  the denominator second, and they allow a client to specify the range of aspect ratios
	  it prefers. Window managers that honor aspect ratios should take into account the
	  base size in determining the preferred window size. If a base size is provided along
	  with the aspect ratio fields, the base size should be subtracted from the window size
	  prior to checking that the aspect ratio falls in range. If a base size is not provided,
	  nothing should be subtracted from the window size. (The minimum size is not to be
	  used in place of the base size for this purpose.)
	*/

	XSizeHints size_hints;
	long user_supplied;

	XGetWMNormalHints(display->x_display, window->x_window, &size_hints, &user_supplied);

	warn_log("ignoring user supplied");

	((sl_window_mutable*)window)->normal_hints = (struct window_normal_hints) {
	0, 0, 0, 0, 0, 0, {0, 0},
        {0, 0},
        0, 0, 0
  };
	// ^^^^^^^ epic clang-format ^^^^^^^

	if (size_hints.flags & PMinSize && size_hints.flags & PBaseSize) {
		((sl_window_mutable*)window)->normal_hints.min_width = size_hints.min_width;
		((sl_window_mutable*)window)->normal_hints.min_height = size_hints.min_height;

		((sl_window_mutable*)window)->normal_hints.base_width = size_hints.base_width;
		((sl_window_mutable*)window)->normal_hints.base_height = size_hints.base_height;
	} else if (size_hints.flags & PMinSize) {
		((sl_window_mutable*)window)->normal_hints.min_width = size_hints.min_width;
		((sl_window_mutable*)window)->normal_hints.min_height = size_hints.min_height;

		((sl_window_mutable*)window)->normal_hints.base_width = size_hints.min_width;
		((sl_window_mutable*)window)->normal_hints.base_height = size_hints.min_height;
	} else if (size_hints.flags & PBaseSize) {
		((sl_window_mutable*)window)->normal_hints.min_width = size_hints.base_width;
		((sl_window_mutable*)window)->normal_hints.min_height = size_hints.base_height;

		((sl_window_mutable*)window)->normal_hints.base_width = size_hints.base_width;
		((sl_window_mutable*)window)->normal_hints.base_height = size_hints.base_height;
	}

	if (size_hints.flags & PMaxSize) {
		((sl_window_mutable*)window)->normal_hints.max_width = size_hints.max_width;
		((sl_window_mutable*)window)->normal_hints.max_height = size_hints.max_height;
	}

	if (size_hints.flags & PResizeInc) {
		((sl_window_mutable*)window)->normal_hints.width_inc = size_hints.width_inc;
		((sl_window_mutable*)window)->normal_hints.height_inc = size_hints.height_inc;
	}

	if (size_hints.flags & PAspect) {
		((sl_window_mutable*)window)->normal_hints.min_aspect =
		(struct window_normal_hints_aspect) {.numerator = size_hints.min_aspect.x, .denominator = size_hints.y};
		((sl_window_mutable*)window)->normal_hints.max_aspect =
		(struct window_normal_hints_aspect) {.numerator = size_hints.max_aspect.x, .denominator = size_hints.y};
	}

	if (size_hints.flags & PWinGravity) {
		((sl_window_mutable*)window)->normal_hints.gravity = size_hints.win_gravity;
	}

	warn_log_va(
	"[%lu] window normal hints: min_width %u, min_height %u, max_width %u, max_height %u, width_inc %u, height_inc %u, min_aspect %u/%u, max_aspect "
	"%u/%u, base_width %u, base_height %u, gravity %u",
	window->x_window, window->normal_hints.min_width, window->normal_hints.min_height, window->normal_hints.max_width, window->normal_hints.max_height,
	window->normal_hints.width_inc, window->normal_hints.height_inc, window->normal_hints.min_aspect.numerator,
	window->normal_hints.min_aspect.denominator, window->normal_hints.max_aspect.numerator, window->normal_hints.max_aspect.denominator,
	window->normal_hints.base_width, window->normal_hints.base_height, window->normal_hints.gravity
	);
}

void sl_set_window_hints (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  The WM_HINTS property (whose type is WM_HINTS) is used to communicate to the
	  window manager. It conveys the information the window manager needs other than
	  the window geometry, which is available from the window itself; the constraints
	  on that geometry, which is available from the WM_NORMAL_HINTS structure; and
	  various strings, which need separate properties, such as WM_NAME. The contents
	  of the properties are as follows:

	  Field         | Type   | Comments
	  flags         | CARD32 | (see the next table)
	  input         | CARD32 | The client's input model
	  initial_state | CARD32 | The state when first mapped
	  icon_pixmap   | PIXMAP | The pixmap for the icon image
	  icon_window   | WINDOW | The window for the icon image
	  icon_x        | INT32  | The icon location
	  icon_y        | INT32  |
	  icon_mask     | PIXMAP | The mask for the icon shape
	  window_group  | WINDOW | The ID of the group leader window

	  The WM_HINTS.flags bit definitions are as follows:

	  Name             | Value | Field
	  InputHint        | 1     | input
	  StateHint        | 2     | initial_state
	  IconPixmapHint   | 4     | icon_pixmap
	  IconWindowHint   | 8     | icon_window
	  IconPositionHint | 16    | icon_x & icon_y
	  IconMaskHint     | 32    | icon_mask
	  WindowGroupHint  | 64    | window_group
	  MessageHint      | 128   | (this bit is obsolete)
	  UrgencyHint      | 256   | urgency

	  Window managers are free to assume convenient values for all fields of the
	  WM_HINTS property if a window is mapped without one.

	  The input field is used to communicate to the window manager the input focus model
	  used by the client (see Input Focus ).

	  Clients with the Globally Active and No Input models should set the input flag to
	  False. Clients with the Passive and Locally Active models should set the input flag
	  to True.

	  From the client's point of view, the window manager will regard the client's top-
	  level window as being in one of three states:

	  • Normal
	  • Iconic
	  • Withdrawn

	  The semantics of these states are described in Changing Window State. Newly
	  created windows start in the Withdrawn state. Transitions between states happen
	  when a top-level window is mapped and unmapped and when the window manager
	  receives certain messages.

	  The value of the initial_state field determines the state the client wishes to be in
	  at the time the top-level window is mapped from the Withdrawn state, as shown in
	  the following table:

	  State       | Value | Comments
	  NormalState | 1     | The window is visible
	  IconicState | 3     | The icon is visible

	  The icon_pixmap field may specify a pixmap to be used as an icon. This pixmap
	  should be:

	  • One of the sizes specified in the WM_ICON_SIZE property on the root if it exists
	  (see WM_ICON_SIZE Property ).

	  • 1-bit deep. The window manager will select, through the defaults database,
	  suitable background (for the 0 bits) and foreground (for the 1 bits) colors. These
	  defaults can, of course, specify different colors for the icons of different clients.

	  The icon_mask specifies which pixels of the icon_pixmap should be used as the icon,
	  allowing for icons to appear nonrectangular.

	  The icon_window field is the ID of a window the client wants used as its icon. Most,
	  but not all, window managers will support icon windows. Those that do not are
	  likely to have a user interface in which small windows that behave like icons are
	  completely inappropriate. Clients should not attempt to remedy the omission by
	  working around it.

	  Clients that need more capabilities from the icons than a simple 2-color bitmap
	  should use icon windows. Rules for clients that do are set out in Icons.

	  The (icon_x,icon_y) coordinate is a hint to the window manager as to where it should
	  position the icon. The policies of the window manager control the positioning of
	  icons, so clients should not depend on attention being paid to this hint.

	  The window_group field lets the client specify that this window belongs to a group
	  of windows. An example is a single client manipulating multiple children of the root
	  window.

	    Conventions

	    • The window_group field should be set to the ID of the group leader.
	    The window group leader may be a window that exists only for that
	    purpose; a placeholder group leader of this kind would never be
	    mapped either by the client or by the window manager.

	    • The properties of the window group leader are those for the group
	    as a whole (for example, the icon to be shown when the entire group
	    is iconified).

	  Window managers may provide facilities for manipulating the group as a whole.
	  Clients, at present, have no way to operate on the group as a whole.

	  The messages bit, if set in the flags field, indicates that the client is using an obsolete
	  window manager communication protocol, [1] rather than the WM_PROTOCOLS
	  mechanism of WM_PROTOCOLS Property

	  [1] This obsolete protocol was described in the July 27, 1988, draft of the ICCCM. Windows using it can also be detected
	  because their WM_HINTS properties are 4 bytes longer than expected. Window managers are free to support clients
	  using the obsolete protocol in a backwards compatibility mode.

	  The UrgencyHint flag, if set in the flags field, indicates that the client deems the
	  window contents to be urgent, requiring the timely response of the user. The window
	  manager must make some effort to draw the user's attention to this window while
	  this flag is set. The window manager must also monitor the state of this flag for the
	  entire time the window is in the Normal or Iconic state and must take appropriate
	  action when the state of the flag changes. The flag is otherwise independent of the
	  window's state; in particular, the window manager is not required to deiconify the
	  window if the client sets the flag on an Iconic window. Clients must provide some
	  means by which the user can cause the UrgencyHint flag to be set to zero or the
	  window to be withdrawn. The user's action can either mitigate the actual condition
	  that made the window urgent, or it can merely shut off the alarm.

	    Rationale

	    This mechanism is useful for alarm dialog boxes or reminder
	    windows, in cases where mapping the window is not enough (e.g.,
	    in the presence of multi-workspace or virtual desktop window
	    managers), and where using an override-redirect window is too
	    intrusive. For example, the window manager may attract attention to
	    an urgent window by adding an indicator to its title bar or its icon.
	    Window managers may also take additional action for a window that
	    is newly urgent, such as by flashing its icon (if the window is iconic)
	    or by raising it to the top of the stack.
	*/

	((sl_window_mutable*)window)->hints = (struct window_hints) {.input = true, .state = NormalState, .urgent = false};

	XWMHints* hints = XGetWMHints(display->x_display, window->x_window);

	if (!hints) return;

	warn_log("ignoring some of the window's hints");

	if (hints->flags & InputHint) ((sl_window_mutable*)window)->hints.input = hints->input;
	if (hints->flags & StateHint) ((sl_window_mutable*)window)->hints.state = hints->initial_state;
	if (hints->flags & 256) ((sl_window_mutable*)window)->hints.urgent = true;

	XFree(hints);

	warn_log_va(
	"[%lu] window hints: input %s, state %s, urgent %s", window->x_window, window->hints.input ? "true" : "false",
	window->hints.state == NormalState ? "normal" :
	window->hints.state == IconicState ? "iconic" :
	                                     "undefined",
	window->hints.urgent ? "true" : "false"
	); // epic clang-format again
}

void sl_set_window_class (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  The WM_CLASS property (of type STRING without control characters) contains two
	  consecutive null-terminated strings. These specify the Instance and Class names to
	  be used by both the client and the window manager for looking up resources for
	  the application or as identifying information. This property must be present when
	  the window leaves the Withdrawn state and may be changed only while the window
	  is in the Withdrawn state. Window managers may examine the property only when
	  they start up and when the window leaves the Withdrawn state, but there should
	  be no need for a client to change its state dynamically.

	  The two strings, respectively, are:

	  • A string that names the particular instance of the application to which the client
	  that owns this window belongs. Resources that are specified by instance name
	  override any resources that are specified by class name. Instance names can
	  be specified by the user in an operating-system specific manner. On POSIX-
	  conformant systems, the following conventions are used:

	    • If "-name NAME" is given on the command line, NAME is used as the instance
	    name.

	    • Otherwise, if the environment variable RESOURCE_NAME is set, its value will
	    be used as the instance name.

	    • Otherwise, the trailing part of the name used to invoke the program (argv[0]
	    stripped of any directory names) is used as the instance name.

	  • A string that names the general class of applications to which the client that
	  owns this window belongs. Resources that are specified by class apply to all
	  applications that have the same class name. Class names are specified by the
	  application writer. Examples of commonly used class names include: "Emacs",
	  "XTerm", "XClock", "XLoad", and so on.

	  Note that WM_CLASS strings are null-terminated and, thus, differ from the general
	  conventions that STRING properties are null-separated. This inconsistency is
	  necessary for backwards compatibility.
	*/

	warn_log("todo: wm_window_class");
}

void sl_set_window_transient_for (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  The WM_TRANSIENT_FOR property (of type WINDOW) contains the ID of another
	  top-level window. The implication is that this window is a pop-up on behalf of
	  the named window, and window managers may decide not to decorate transient
	  windows or may treat them differently in other ways. In particular, window
	  managers should present newly mapped WM_TRANSIENT_FOR windows without
	  requiring any user interaction, even if mapping top-level windows normally does
	  require interaction. Dialogue boxes, for example, are an example of windows that
	  should have WM_TRANSIENT_FOR set.

	  It is important not to confuse WM_TRANSIENT_FOR with override-redirect.
	  WM_TRANSIENT_FOR should be used in those cases where the pointer is not
	  grabbed while the window is mapped (in other words, if other windows are allowed
	  to be active while the transient is up). If other windows must be prevented from
	  processing input (for example, when implementing pop-up menus), use override-
	  redirect and grab the pointer while the window is mapped.
	*/

	warn_log("todo: wm_transient_for");
}

void sl_set_window_protocols (sl_window* window, sl_display* display) {
	/*
	  The WM_PROTOCOLS property (of type ATOM) is a list of atoms. Each atom
	  identifies a communication protocol between the client and the window manager in
	  which the client is willing to participate. Atoms can identify both standard protocols
	  and private protocols specific to individual window managers.

	  All the protocols in which a client can volunteer to take part involve the
	  window manager sending the client a ClientMessage event and the client taking
	  appropriate action. For details of the contents of the event, see ClientMessage
	  Events In each case, the protocol transactions are initiated by the window manager.

	  The WM_PROTOCOLS property is not required. If it is not present, the client does
	  not want to participate in any window manager protocols.

	  The X Consortium will maintain a registry of protocols to avoid collisions in the
	  name space. The following table lists the protocols that have been defined to date.

	  Protocol         | Section         | Purpose
	  WM_TAKE_FOCUS    | Input           | Focus  Assignment of input focus
	  WM_SAVE_YOURSELF | Appendix C      | Save client state request (deprecated)
	  WM_DELETE_WINDOW | Window Deletion | Request to delete top-level window

	  It is expected that this table will grow over time.
	*/

	Atom* protocols = NULL;
	int n = 0;

	if (!XGetWMProtocols(display->x_display, window->x_window, &protocols, &n)) return;

	((sl_window_mutable*)window)->have_protocols = (struct window_protocols) {.take_focus = false, .delete_window = false};

	for (size_t i = 0; i <= (size_t)n; ++i) {
		if (protocols[i] == display->atoms[wm_take_focus]) {
			((sl_window_mutable*)window)->have_protocols.take_focus = true;
			continue;
		}

		if (protocols[i] == display->atoms[wm_delete_window]) {
			((sl_window_mutable*)window)->have_protocols.delete_window = true;
			continue;
		}
	}

	XFree(protocols);

	warn_log_va(
	"[%lu] window protocols: %s", window->x_window,
	window->have_protocols.take_focus    ? (window->have_protocols.delete_window ? "take focus and delete window" : "take focus") :
	window->have_protocols.delete_window ? "delete window" :
	                                       "none"
	);
}

void sl_set_window_colormap_windows (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  The WM_COLORMAP_WINDOWS property (of type WINDOW) on a top-level
	  window is a list of the IDs of windows that may need colormaps installed that differ
	  from the colormap of the top-level window. The window manager will watch this
	  list of windows for changes in their colormap attributes. The top-level window is
	  always (implicitly or explicitly) on the watch list. For the details of this mechanism,
	  see Colormaps
	*/

	warn_log("todo: wm_window_colormap_windows");
}

void sl_set_window_client_machine (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  The client should set the WM_CLIENT_MACHINE property (of one of the TEXT
	  types) to a string that forms the name of the machine running the client as seen
	  from the machine running the server.
	*/

	warn_log("todo: wm_client_machine");
}

/*
  Extended Window Manager Hints: Application Window Properties
*/

static void
window_set_net_utf8_string_property (sl_window* window, sl_display* display, size_t atom_index, struct sl_sized_string_mutable* sized_string) {
	Atom actual_type;
	int actual_format;
	ulong items_size;
	ulong bytes_after;
	uchar* prop = NULL;

	if (XGetWindowProperty(display->x_display, window->x_window, display->atoms[atom_index], 0, 1, false, display->atoms[type_utf8_string], &actual_type, &actual_format, &items_size, &bytes_after, &prop) != Success) {
		warn_log("XGetWindowProperty does not return Success");
		return;
	}

	/*
	  The XGetWindowProperty function returns the actual type of the property; the actual
	  format of the property; the number of 8-bit, 16-bit, or 32-bit items transferred; the
	  number of bytes remaining to be read in the property; and a pointer to the data
	  actually returned. XGetWindowProperty sets the return arguments as follows:
	*/

	/*
	  If the specified property does not exist for the specified window,
	  XGetWindowProperty returns None to actual_type_return and the value zero
	  to actual_format_return and bytes_after_return. The nitems_return argument is
	  empty. In this case, the delete argument is ignored.
	*/

	if (actual_type == None) {
		warn_log("empty property");
		XFree(prop);
		return;
	}

	/*
	  If the specified property exists but its type does not match the specified type,
	  XGetWindowProperty returns the actual property type to actual_type_return, the
	  actual property format (never zero) to actual_format_return, and the property
	  length in bytes (even if the actual_format_return is 16 or 32) to bytes_after_return.
	  It also ignores the delete argument. The nitems_return argument is empty.
	*/

	if (actual_type != display->atoms[type_utf8_string]) {
		warn_log("atom type mismatch");
		XFree(prop);
		return;
	}

	/*
	  If the specified property exists and either you assign AnyPropertyType to the
	  req_type argument or the specified type matches the actual property type,
	  XGetWindowProperty returns the actual property type to actual_type_return and
	  the actual property format (never zero) to actual_format_return. It also returns a
	  value to bytes_after_return and nitems_return, by defining the following values:

	  • N = actual length of the stored property in bytes (even if the format is 16 or 32)
	  I = 4 * long_offset T = N - I L = MINIMUM(T, 4 * long_length) A = N - (I + L)

	  • The returned value starts at byte index I in the property (indexing from zero),
	  and its length in bytes is L. If the value for long_offset causes L to be negative,
	  a BadValue error results. The value of bytes_after_return is A, giving the number
	  of trailing unread bytes in the stored property.
	*/

	XFree(prop);

	XGetWindowProperty(
	display->x_display, window->x_window, display->atoms[atom_index], 0, 2 + (bytes_after >> 2), false, display->atoms[type_utf8_string], &actual_type,
	&actual_format, &items_size, &bytes_after, &prop
	);

	sized_string->size = items_size;
	if (sized_string->data) free(sized_string->data);
	sized_string->data = malloc(sized_string->size);
	memcpy(sized_string->data, prop, sized_string->size);

	XFree(prop);
}

void sl_window_set_net_wm_name (sl_window* window, sl_display* display) {
	/*
	  _NET_WM_NAME, UTF8_STRING

	  The Client SHOULD set this to the title of the window in UTF-8 encoding. If set, the Window Manager should use this in preference to WM_NAME.
	*/

	window_set_net_utf8_string_property(window, display, net_wm_name, (struct sl_sized_string_mutable*)&window->net_wm_name);

	warn_log_va("[%lu] net_wm_name \"%.*s\"", window->x_window, (int)window->net_wm_name.size, window->net_wm_name.data);
}

void sl_window_set_net_wm_visible_name (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_VISIBLE_NAME, UTF8_STRING

	  If the Window Manager displays a window name other than _NET_WM_NAME the Window Manager MUST set this to the title displayed in UTF-8 encoding.

	  Rationale: This property is for Window Managers that display a title different from the _NET_WM_NAME or WM_NAME of the window (i.e. xterm <1>,
	  xterm <2>, ... is shown, but _NET_WM_NAME / WM_NAME is still xterm for each window) thereby allowing Pagers to display the same title as the
	  Window Manager.
	*/

	window_set_net_utf8_string_property(window, display, net_wm_visible_name, (struct sl_sized_string_mutable*)&window->net_wm_visible_name);

	warn_log_va("[%lu] net_wm_visible_name \"%.*s\"", window->x_window, (int)window->net_wm_visible_name.size, window->net_wm_visible_name.data);
}

void sl_window_set_net_wm_icon_name (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_ICON_NAME, UTF8_STRING

	  The Client SHOULD set this to the title of the icon for this window in UTF-8 encoding. If set, the Window Manager should use this in preference to
	  WM_ICON_NAME.
	*/

	window_set_net_utf8_string_property(window, display, net_wm_icon_name, (struct sl_sized_string_mutable*)&window->net_wm_icon_name);

	warn_log_va("[%lu] net_wm_icon_name \"%.*s\"", window->x_window, (int)window->net_wm_icon_name.size, window->net_wm_icon_name.data);
}

void sl_window_set_net_wm_visible_icon_name (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_VISIBLE_ICON_NAME, UTF8_STRING

	  If the Window Manager displays an icon name other than _NET_WM_ICON_NAME the Window Manager MUST set this to the title displayed in UTF-8
	  encoding.
	*/

	window_set_net_utf8_string_property(window, display, net_wm_visible_icon_name, (struct sl_sized_string_mutable*)&window->net_wm_visible_icon_name);

	warn_log_va(
	"[%lu] net_wm_visible_icon_name \"%.*s\"", window->x_window, (int)window->net_wm_visible_icon_name.size, window->net_wm_visible_icon_name.data
	);
}

void sl_window_set_net_wm_desktop (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_DESKTOP desktop, CARDINAL/32

	  Cardinal to determine the desktop the window is in (or wants to be) starting with 0 for the first desktop. A Client MAY choose not to set this
	  property, in which case the Window Manager SHOULD place it as it wishes. 0xFFFFFFFF indicates that the window SHOULD appear on all desktops.

	  The Window Manager should honor _NET_WM_DESKTOP whenever a withdrawn window requests to be mapped.

	  The Window Manager should remove the property whenever a window is withdrawn but it should leave the property in place when it is shutting down,
	  e.g. in response to losing ownership of the WM_Sn manager selection.

	  Rationale: Removing the property upon window withdrawal helps legacy applications which want to reuse withdrawn windows. Not removing the property
	  upon shutdown allows the next Window Manager to restore windows to their previous desktops.

	  A Client can request a change of desktop for a non-withdrawn window by sending a _NET_WM_DESKTOP client message to the root window:

	  _NET_WM_DESKTOP
	  window  = the respective client window
	  message_type = _NET_WM_DESKTOP
	  format = 32
	  data.l[0] = new_desktop
	  data.l[1] = source indication
	  other data.l[] elements = 0

	  See the section called “Source indication in requests” for details on the source indication. The Window Manager MUST keep this property updated on
	  all windows.
	*/

	warn_log("todo: _net_wm_desktop");
}

static int get_net_atom_list (sl_window* window, sl_display* display, size_t atom_index, uchar** prop, ulong* items_size) {
	Atom actual_type;
	int actual_format;
	ulong bytes_after;

	if (XGetWindowProperty(display->x_display, window->x_window, display->atoms[atom_index], 0, 1, false, XA_ATOM, &actual_type, &actual_format, items_size, &bytes_after, prop) != Success) {
		warn_log("XGetWindowProperty does not return Success");
		return -1;
	}

	/*
	  The XGetWindowProperty function returns the actual type of the property; the actual
	  format of the property; the number of 8-bit, 16-bit, or 32-bit items transferred; the
	  number of bytes remaining to be read in the property; and a pointer to the data
	  actually returned. XGetWindowProperty sets the return arguments as follows:
	*/

	/*
	  If the specified property does not exist for the specified window,
	  XGetWindowProperty returns None to actual_type_return and the value zero
	  to actual_format_return and bytes_after_return. The nitems_return argument is
	  empty. In this case, the delete argument is ignored.
	*/

	if (actual_type == None) {
		warn_log("empty property");
		XFree(*prop);
		return -1;
	}

	/*
	  If the specified property exists but its type does not match the specified type,
	  XGetWindowProperty returns the actual property type to actual_type_return, the
	  actual property format (never zero) to actual_format_return, and the property
	  length in bytes (even if the actual_format_return is 16 or 32) to bytes_after_return.
	  It also ignores the delete argument. The nitems_return argument is empty.
	*/

	if (actual_type != XA_ATOM) {
		warn_log("atom type mismatch");
		XFree(*prop);
		return -1;
	}

	/*
	  If the specified property exists and either you assign AnyPropertyType to the
	  req_type argument or the specified type matches the actual property type,
	  XGetWindowProperty returns the actual property type to actual_type_return and
	  the actual property format (never zero) to actual_format_return. It also returns a
	  value to bytes_after_return and nitems_return, by defining the following values:

	  • N = actual length of the stored property in bytes (even if the format is 16 or 32)
	  I = 4 * long_offset T = N - I L = MINIMUM(T, 4 * long_length) A = N - (I + L)

	  • The returned value starts at byte index I in the property (indexing from zero),
	  and its length in bytes is L. If the value for long_offset causes L to be negative,
	  a BadValue error results. The value of bytes_after_return is A, giving the number
	  of trailing unread bytes in the stored property.
	*/

	XGetWindowProperty(
	display->x_display, window->x_window, display->atoms[atom_index], 0, 2 + (bytes_after >> 2), false, XA_ATOM, &actual_type, &actual_format,
	items_size, &bytes_after, prop
	);
	return 0;
}

void sl_window_set_net_wm_window_type (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_WINDOW_TYPE, ATOM[]/32

	  This SHOULD be set by the Client before mapping to a list of atoms indicating the functional type of the window. This property SHOULD be used by
	  the window manager in determining the decoration, stacking position and other behavior of the window. The Client SHOULD specify window types in
	  order of preference (the first being most preferable) but MUST include at least one of the basic window type atoms from the list below. This is to
	  allow for extension of the list of types whilst providing default behavior for Window Managers that do not recognize the extensions.

	  This hint SHOULD also be set for override-redirect windows to allow compositing managers to apply consistent decorations to menus, tooltips etc.

	  Rationale: This hint is intended to replace the MOTIF hints. One of the objections to the MOTIF hints is that they are a purely visual description
	  of the window decoration. By describing the function of the window, the Window Manager can apply consistent decoration and behavior to windows of
	  the same type. Possible examples of behavior include keeping dock/panels on top or allowing pinnable menus / toolbars to only be hidden when
	  another window has focus (NextStep style).

	  _NET_WM_WINDOW_TYPE_DESKTOP, ATOM
	  _NET_WM_WINDOW_TYPE_DOCK, ATOM
	  _NET_WM_WINDOW_TYPE_TOOLBAR, ATOM
	  _NET_WM_WINDOW_TYPE_MENU, ATOM
	  _NET_WM_WINDOW_TYPE_UTILITY, ATOM
	  _NET_WM_WINDOW_TYPE_SPLASH, ATOM
	  _NET_WM_WINDOW_TYPE_DIALOG, ATOM
	  _NET_WM_WINDOW_TYPE_DROPDOWN_MENU, ATOM
	  _NET_WM_WINDOW_TYPE_POPUP_MENU, ATOM
	  _NET_WM_WINDOW_TYPE_TOOLTIP, ATOM
	  _NET_WM_WINDOW_TYPE_NOTIFICATION, ATOM
	  _NET_WM_WINDOW_TYPE_COMBO, ATOM
	  _NET_WM_WINDOW_TYPE_DND, ATOM
	  _NET_WM_WINDOW_TYPE_NORMAL, ATOM

	  _NET_WM_WINDOW_TYPE_DESKTOP indicates a desktop feature. This can include a single window containing desktop icons with the same dimensions as the
	  screen, allowing the desktop environment to have full control of the desktop, without the need for proxying root window clicks.

	  _NET_WM_WINDOW_TYPE_DOCK indicates a dock or panel feature. Typically a Window Manager would keep such windows on top of all other windows.

	  _NET_WM_WINDOW_TYPE_TOOLBAR and _NET_WM_WINDOW_TYPE_MENU indicate toolbar and pinnable menu windows, respectively (i.e. toolbars and menus "torn
	  off" from the main application). Windows of this type may set the WM_TRANSIENT_FOR hint indicating the main application window. Note that the
	  _NET_WM_WINDOW_TYPE_MENU should be set on torn-off managed windows, where _NET_WM_WINDOW_TYPE_DROPDOWN_MENU and _NET_WM_WINDOW_TYPE_POPUP_MENU are
	  typically used on override-redirect windows.

	  _NET_WM_WINDOW_TYPE_UTILITY indicates a small persistent utility window, such as a palette or toolbox. It is distinct from type TOOLBAR because it
	  does not correspond to a toolbar torn off from the main application. It's distinct from type DIALOG because it isn't a transient dialog, the user
	  will probably keep it open while they're working. Windows of this type may set the WM_TRANSIENT_FOR hint indicating the main application window.

	  _NET_WM_WINDOW_TYPE_SPLASH indicates that the window is a splash screen displayed as an application is starting up.

	  _NET_WM_WINDOW_TYPE_DIALOG indicates that this is a dialog window. If _NET_WM_WINDOW_TYPE is not set, then managed windows with WM_TRANSIENT_FOR
	  set MUST be taken as this type. Override-redirect windows with WM_TRANSIENT_FOR, but without _NET_WM_WINDOW_TYPE must be taken as
	  _NET_WM_WINDOW_TYPE_NORMAL.

	  _NET_WM_WINDOW_TYPE_DROPDOWN_MENU indicates that the window in question is a dropdown menu, ie., the kind of menu that typically appears when the
	  user clicks on a menubar, as opposed to a popup menu which typically appears when the user right-clicks on an object. This property is typically
	  used on override-redirect windows.

	  _NET_WM_WINDOW_TYPE_POPUP_MENU indicates that the window in question is a popup menu, ie., the kind of menu that typically appears when the user
	  right clicks on an object, as opposed to a dropdown menu which typically appears when the user clicks on a menubar. This property is typically
	  used on override-redirect windows.

	  _NET_WM_WINDOW_TYPE_TOOLTIP indicates that the window in question is a tooltip, ie., a short piece of explanatory text that typically appear after
	  the mouse cursor hovers over an object for a while. This property is typically used on override-redirect windows.

	  _NET_WM_WINDOW_TYPE_NOTIFICATION indicates a notification. An example of a notification would be a bubble appearing with informative text such as
	  "Your laptop is running out of power" etc. This property is typically used on override-redirect windows.

	  _NET_WM_WINDOW_TYPE_COMBO should be used on the windows that are popped up by combo boxes. An example is a window that appears below a text field
	  with a list of suggested completions. This property is typically used on override-redirect windows.

	  _NET_WM_WINDOW_TYPE_DND indicates that the window is being dragged. Clients should set this hint when the window in question contains a
	  representation of an object being dragged from one place to another. An example would be a window containing an icon that is being dragged from
	  one file manager window to another. This property is typically used on override-redirect windows.

	  _NET_WM_WINDOW_TYPE_NORMAL indicates that this is a normal, top-level window, either managed or override-redirect. Managed windows with neither
	  _NET_WM_WINDOW_TYPE nor WM_TRANSIENT_FOR set MUST be taken as this type. Override-redirect windows without _NET_WM_WINDOW_TYPE, must be taken as
	  this type, whether or not they have WM_TRANSIENT_FOR set.
	*/

	Atom* prop = NULL;
	ulong items_size;

	if (get_net_atom_list(window, display, net_wm_window_type, (uchar**)&prop, &items_size) != 0) return;

	((sl_window_mutable*)window)->type = 0;

	for (size_t i = 0; i < items_size; ++i) {
		if (prop[i] == display->atoms[net_wm_window_type_desktop]) ((sl_window_mutable*)window)->type |= window_type_desktop_bit;
		if (prop[i] == display->atoms[net_wm_window_type_dock]) ((sl_window_mutable*)window)->type |= window_type_dock_bit;
		if (prop[i] == display->atoms[net_wm_window_type_toolbar]) ((sl_window_mutable*)window)->type |= window_type_toolbar_bit;
		if (prop[i] == display->atoms[net_wm_window_type_menu]) ((sl_window_mutable*)window)->type |= window_type_menu_bit;
		if (prop[i] == display->atoms[net_wm_window_type_utility]) ((sl_window_mutable*)window)->type |= window_type_utility_bit;
		if (prop[i] == display->atoms[net_wm_window_type_splash]) ((sl_window_mutable*)window)->type |= window_type_splash_bit;
		if (prop[i] == display->atoms[net_wm_window_type_dialog]) ((sl_window_mutable*)window)->type |= window_type_dialog_bit;
		if (prop[i] == display->atoms[net_wm_window_type_dropdown_menu]) ((sl_window_mutable*)window)->type |= window_type_dropdown_menu_bit;
		if (prop[i] == display->atoms[net_wm_window_type_popup_menu]) ((sl_window_mutable*)window)->type |= window_type_popup_menu_bit;
		if (prop[i] == display->atoms[net_wm_window_type_tooltip]) ((sl_window_mutable*)window)->type |= window_type_tooltip_bit;
		if (prop[i] == display->atoms[net_wm_window_type_notification]) ((sl_window_mutable*)window)->type |= window_type_notification_bit;
		if (prop[i] == display->atoms[net_wm_window_type_combo]) ((sl_window_mutable*)window)->type |= window_type_combo_bit;
		if (prop[i] == display->atoms[net_wm_window_type_dnd]) ((sl_window_mutable*)window)->type |= window_type_dnd_bit;
		if (prop[i] == display->atoms[net_wm_window_type_normal]) ((sl_window_mutable*)window)->type |= window_type_normal_bit;
	}

	XFree(prop);

	char buffer[256] = "";

	if (window->type & window_type_desktop_bit) strcat(buffer, "desktop ");
	if (window->type & window_type_dock_bit) strcat(buffer, "dock ");
	if (window->type & window_type_toolbar_bit) strcat(buffer, "toolbar ");
	if (window->type & window_type_menu_bit) strcat(buffer, "menu ");
	if (window->type & window_type_utility_bit) strcat(buffer, "utility ");
	if (window->type & window_type_splash_bit) strcat(buffer, "splash ");
	if (window->type & window_type_dialog_bit) strcat(buffer, "dialog ");
	if (window->type & window_type_dropdown_menu_bit) strcat(buffer, "dropdown_menu ");
	if (window->type & window_type_popup_menu_bit) strcat(buffer, "popup_menu ");
	if (window->type & window_type_tooltip_bit) strcat(buffer, "tooltip ");
	if (window->type & window_type_notification_bit) strcat(buffer, "notification ");
	if (window->type & window_type_combo_bit) strcat(buffer, "combo ");
	if (window->type & window_type_dnd_bit) strcat(buffer, "dnd ");
	if (window->type & window_type_normal_bit) strcat(buffer, "normal ");

	warn_log_va("[%lu] window type: %s", window->x_window, buffer);
}

void sl_window_set_net_wm_state (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_STATE, ATOM[]

	  A list of hints describing the window state. Atoms present in the list MUST be considered set, atoms not present in the list MUST be considered
	  not set. The Window Manager SHOULD honor _NET_WM_STATE whenever a withdrawn window requests to be mapped. A Client wishing to change the state of
	  a window MUST send a _NET_WM_STATE client message to the root window (see below). The Window Manager MUST keep this property updated to reflect
	  the current state of the window.

	  The Window Manager should remove the property whenever a window is withdrawn, but it should leave the property in place when it is shutting down,
	  e.g. in response to losing ownership of the WM_Sn manager selection.

	  Rationale: Removing the property upon window withdrawal helps legacy applications which want to reuse withdrawn windows. Not removing the property
	  upon shutdown allows the next Window Manager to restore windows to their previous state.

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

	  An implementation MAY add new atoms to this list. Implementations without extensions MUST ignore any unknown atoms, effectively removing them from
	  the list. These extension atoms MUST NOT start with the prefix _NET.

	  _NET_WM_STATE_MODAL indicates that this is a modal dialog box. If the WM_TRANSIENT_FOR hint is set to another toplevel window, the dialog is modal
	  for that window; if WM_TRANSIENT_FOR is not set or set to the root window the dialog is modal for its window group.

	  _NET_WM_STATE_STICKY indicates that the Window Manager SHOULD keep the window's position fixed on the screen, even when the virtual desktop
	  scrolls.

	  _NET_WM_STATE_MAXIMIZED_{VERT,HORZ} indicates that the window is {vertically,horizontally} maximized.

	  _NET_WM_STATE_SHADED indicates that the window is shaded.

	  _NET_WM_STATE_SKIP_TASKBAR indicates that the window should not be included on a taskbar. This hint should be requested by the application, i.e.
	  it indicates that the window by nature is never in the taskbar. Applications should not set this hint if _NET_WM_WINDOW_TYPE already conveys the
	  exact nature of the window.

	  _NET_WM_STATE_SKIP_PAGER indicates that the window should not be included on a Pager. This hint should be requested by the application, i.e. it
	  indicates that the window by nature is never in the Pager. Applications should not set this hint if _NET_WM_WINDOW_TYPE already conveys the exact
	  nature of the window.

	  _NET_WM_STATE_HIDDEN should be set by the Window Manager to indicate that a window would not be visible on the screen if its desktop/viewport were
	  active and its coordinates were within the screen bounds. The canonical example is that minimized windows should be in the _NET_WM_STATE_HIDDEN
	  state. Pagers and similar applications should use _NET_WM_STATE_HIDDEN instead of WM_STATE to decide whether to display a window in miniature
	  representations of the windows on a desktop.

	  Implementation note: if an Application asks to toggle _NET_WM_STATE_HIDDEN the Window Manager should probably just ignore the request, since
	  _NET_WM_STATE_HIDDEN is a function of some other aspect of the window such as minimization, rather than an independent state.

	  _NET_WM_STATE_FULLSCREEN indicates that the window should fill the entire screen and have no window decorations. Additionally the Window Manager
	  is responsible for restoring the original geometry after a switch from fullscreen back to normal window. For example, a presentation program would
	  use this hint.

	  _NET_WM_STATE_ABOVE indicates that the window should be on top of most windows (see the section called “Stacking order” for details).

	  _NET_WM_STATE_BELOW indicates that the window should be below most windows (see the section called “Stacking order” for details).

	  _NET_WM_STATE_ABOVE and _NET_WM_STATE_BELOW are mainly meant for user preferences and should not be used by applications e.g. for drawing
	  attention to their dialogs (the Urgency hint should be used in that case, see the section called “Urgency”).'

	  _NET_WM_STATE_DEMANDS_ATTENTION indicates that some action in or with the window happened. For example, it may be set by the Window Manager if the
	  window requested activation but the Window Manager refused it, or the application may set it if it finished some work. This state may be set by
	  both the Client and the Window Manager. It should be unset by the Window Manager when it decides the window got the required attention (usually,
	  that it got activated).

	  _NET_WM_STATE_FOCUSED indicates whether the window's decorations are drawn in an active state. Clients MUST regard it as a read-only hint. It
	  cannot be set at map time or changed via a _NET_WM_STATE client message. The window given by _NET_ACTIVE_WINDOW will usually have this hint, but
	  at times other windows may as well, if they have a strong association with the active window and will be considered as a unit with it by the user.
	  Clients that modify the appearance of internal elements when a toplevel has keyboard focus SHOULD check for the availability of this state in
	  _NET_SUPPORTED and, if it is available, use it in preference to tracking focus via FocusIn events. By doing so they will match the window
	  decorations and accurately reflect the intentions of the Window Manager.

	  To change the state of a mapped window, a Client MUST send a _NET_WM_STATE client message to the root window:

	   window  = the respective client window
	   message_type = _NET_WM_STATE
	   format = 32
	   data.l[0] = the action, as listed below
	   data.l[1] = first property to alter
	   data.l[2] = second property to alter
	   data.l[3] = source indication
	   other data.l[] elements = 0

	  This message allows two properties to be changed simultaneously, specifically to allow both horizontal and vertical maximization to be altered
	  together. l[2] MUST be set to zero if only one property is to be changed. See the section called “Source indication in requests” for details on
	  the source indication. l[0], the action, MUST be one of:

	  _NET_WM_STATE_REMOVE        0     remove/unset property
	  _NET_WM_STATE_ADD           1     add/set property
	  _NET_WM_STATE_TOGGLE        2     toggle property

	  See also the implementation notes on urgency and fixed size windows.
	*/

	Atom* prop = NULL;
	ulong items_size;

	if (get_net_atom_list(window, display, net_wm_state, (uchar**)&prop, &items_size) != 0) return;

	((sl_window_mutable*)window)->state = 0;

	for (size_t i = 0; i < items_size; ++i) {
		if (prop[i] == display->atoms[net_wm_state_modal]) ((sl_window_mutable*)window)->state |= window_state_modal_bit;
		if (prop[i] == display->atoms[net_wm_state_sticky]) ((sl_window_mutable*)window)->state |= window_state_sticky_bit;
		if (prop[i] == display->atoms[net_wm_state_maximized_vert]) ((sl_window_mutable*)window)->state |= window_state_maximized_vert_bit;
		if (prop[i] == display->atoms[net_wm_state_maximized_horz]) ((sl_window_mutable*)window)->state |= window_state_maximized_horz_bit;
		if (prop[i] == display->atoms[net_wm_state_shaded]) ((sl_window_mutable*)window)->state |= window_state_shaded_bit;
		if (prop[i] == display->atoms[net_wm_state_skip_taskbar]) ((sl_window_mutable*)window)->state |= window_state_skip_taskbar_bit;
		if (prop[i] == display->atoms[net_wm_state_skip_pager]) ((sl_window_mutable*)window)->state |= window_state_skip_pager_bit;
		if (prop[i] == display->atoms[net_wm_state_hidden]) ((sl_window_mutable*)window)->state |= window_state_hidden_bit;
		if (prop[i] == display->atoms[net_wm_state_fullscreen]) ((sl_window_mutable*)window)->state |= window_state_fullscreen_bit;
		if (prop[i] == display->atoms[net_wm_state_above]) ((sl_window_mutable*)window)->state |= window_state_above_bit;
		if (prop[i] == display->atoms[net_wm_state_below]) ((sl_window_mutable*)window)->state |= window_state_below_bit;
		if (prop[i] == display->atoms[net_wm_state_demands_attention]) ((sl_window_mutable*)window)->state |= window_state_demands_attention_bit;
		if (prop[i] == display->atoms[net_wm_state_focused]) ((sl_window_mutable*)window)->state |= window_state_focused_bit;
	}

	XFree(prop);

	char buffer[256] = "";

	if (window->state & window_state_modal_bit) strcat(buffer, "modal ");
	if (window->state & window_state_sticky_bit) strcat(buffer, "sticky ");
	if (window->state & window_state_maximized_vert_bit) strcat(buffer, "maximized_vert ");
	if (window->state & window_state_maximized_horz_bit) strcat(buffer, "maximized_horz ");
	if (window->state & window_state_shaded_bit) strcat(buffer, "shaded ");
	if (window->state & window_state_skip_taskbar_bit) strcat(buffer, "skip_taskbar ");
	if (window->state & window_state_skip_pager_bit) strcat(buffer, "skip_pager ");
	if (window->state & window_state_hidden_bit) strcat(buffer, "hidden ");
	if (window->state & window_state_fullscreen_bit) strcat(buffer, "fullscreen ");
	if (window->state & window_state_above_bit) strcat(buffer, "above ");
	if (window->state & window_state_below_bit) strcat(buffer, "below ");
	if (window->state & window_state_demands_attention_bit) strcat(buffer, "demands_attention ");
	if (window->state & window_state_focused_bit) strcat(buffer, "focused ");

	warn_log_va("[%lu] window state: %s", window->x_window, buffer);
}

void sl_window_set_net_wm_allowed_actions (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_ALLOWED_ACTIONS, ATOM[]

	  A list of atoms indicating user operations that the Window Manager supports for this window. Atoms present in the list indicate allowed actions,
	  atoms not present in the list indicate actions that are not supported for this window. The Window Manager MUST keep this property updated to
	  reflect the actions which are currently "active" or "sensitive" for a window. Taskbars, Pagers, and other tools use _NET_WM_ALLOWED_ACTIONS to
	  decide which actions should be made available to the user.

	  Possible atoms are:

	  _NET_WM_ACTION_MOVE, ATOM
	  _NET_WM_ACTION_RESIZE, ATOM
	  _NET_WM_ACTION_MINIMIZE, ATOM
	  _NET_WM_ACTION_SHADE, ATOM
	  _NET_WM_ACTION_STICK, ATOM
	  _NET_WM_ACTION_MAXIMIZE_HORZ, ATOM
	  _NET_WM_ACTION_MAXIMIZE_VERT, ATOM
	  _NET_WM_ACTION_FULLSCREEN, ATOM
	  _NET_WM_ACTION_CHANGE_DESKTOP, ATOM
	  _NET_WM_ACTION_CLOSE, ATOM
	  _NET_WM_ACTION_ABOVE, ATOM
	  _NET_WM_ACTION_BELOW, ATOM

	  An implementation MAY add new atoms to this list. Implementations without extensions MUST ignore any unknown atoms, effectively removing them from
	  the list. These extension atoms MUST NOT start with the prefix _NET.

	  Note that the actions listed here are those that the Window Manager will honor for this window. The operations must still be requested through the
	  normal mechanisms outlined in this specification. For example, _NET_WM_ACTION_CLOSE does not mean that clients can send a WM_DELETE_WINDOW message
	  to this window; it means that clients can use a _NET_CLOSE_WINDOW message to ask the Window Manager to do so.

	  Window Managers SHOULD ignore the value of _NET_WM_ALLOWED_ACTIONS when they initially manage a window. This value may be left over from a
	  previous Window Manager with different policies.

	  _NET_WM_ACTION_MOVE indicates that the window may be moved around the screen.

	  _NET_WM_ACTION_RESIZE indicates that the window may be resized. (Implementation note: Window Managers can identify a non-resizable window because
	  its minimum and maximum size in WM_NORMAL_HINTS will be the same.)

	  _NET_WM_ACTION_MINIMIZE indicates that the window may be iconified.

	  _NET_WM_ACTION_SHADE indicates that the window may be shaded.

	  _NET_WM_ACTION_STICK indicates that the window may have its sticky state toggled (as for _NET_WM_STATE_STICKY). Note that this state has to do
	  with viewports, not desktops.

	  _NET_WM_ACTION_MAXIMIZE_HORZ indicates that the window may be maximized horizontally.

	  _NET_WM_ACTION_MAXIMIZE_VERT indicates that the window may be maximized vertically.

	  _NET_WM_ACTION_FULLSCREEN indicates that the window may be brought to fullscreen state.

	  _NET_WM_ACTION_CHANGE_DESKTOP indicates that the window may be moved between desktops.

	  _NET_WM_ACTION_CLOSE indicates that the window may be closed (i.e. a _NET_CLOSE_WINDOW message may be sent).

	  _NET_WM_ACTION_ABOVE indicates that the window may placed in the "above" layer of windows (i.e. will respond to _NET_WM_STATE_ABOVE changes; see
	  also the section called “Stacking order” for details).

	  _NET_WM_ACTION_BELOW indicates that the window may placed in the "below" layer of windows (i.e. will respond to _NET_WM_STATE_BELOW changes; see
	  also the section called “Stacking order” for details)).
	*/

	Atom* prop = NULL;
	ulong items_size;

	if (get_net_atom_list(window, display, net_wm_allowed_actions, (uchar**)&prop, &items_size) != 0) return;

	((sl_window_mutable*)window)->allowed_actions = 0;

	for (size_t i = 0; i < items_size; ++i) {
		if (prop[i] == display->atoms[net_wm_action_move]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_move_bit;
		if (prop[i] == display->atoms[net_wm_action_resize]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_resize_bit;
		if (prop[i] == display->atoms[net_wm_action_minimize]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_minimize_bit;
		if (prop[i] == display->atoms[net_wm_action_shade]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_shade_bit;
		if (prop[i] == display->atoms[net_wm_action_stick]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_stick_bit;
		if (prop[i] == display->atoms[net_wm_action_maximize_horz]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_maximize_horz_bit;
		if (prop[i] == display->atoms[net_wm_action_maximize_vert]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_maximize_vert_bit;
		if (prop[i] == display->atoms[net_wm_action_fullscreen]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_fullscreen_bit;
		if (prop[i] == display->atoms[net_wm_action_change_desktop]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_change_desktop_bit;
		if (prop[i] == display->atoms[net_wm_action_close]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_close_bit;
		if (prop[i] == display->atoms[net_wm_action_above]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_above_bit;
		if (prop[i] == display->atoms[net_wm_action_below]) ((sl_window_mutable*)window)->allowed_actions |= allowed_action_below_bit;
	}

	XFree(prop);

	char buffer[256] = "";

	if (window->allowed_actions & allowed_action_move_bit) strcat(buffer, "move ");
	if (window->allowed_actions & allowed_action_resize_bit) strcat(buffer, "resize ");
	if (window->allowed_actions & allowed_action_minimize_bit) strcat(buffer, "minimize ");
	if (window->allowed_actions & allowed_action_shade_bit) strcat(buffer, "shade ");
	if (window->allowed_actions & allowed_action_stick_bit) strcat(buffer, "stick ");
	if (window->allowed_actions & allowed_action_maximize_horz_bit) strcat(buffer, "maximize_horz ");
	if (window->allowed_actions & allowed_action_maximize_vert_bit) strcat(buffer, "maximize_vert ");
	if (window->allowed_actions & allowed_action_fullscreen_bit) strcat(buffer, "fullscreen ");
	if (window->allowed_actions & allowed_action_change_desktop_bit) strcat(buffer, "change_desktop ");
	if (window->allowed_actions & allowed_action_close_bit) strcat(buffer, "close ");
	if (window->allowed_actions & allowed_action_above_bit) strcat(buffer, "above ");
	if (window->allowed_actions & allowed_action_below_bit) strcat(buffer, "below ");

	warn_log_va("[%lu] allowed actions: %s", window->x_window, buffer);
}

void sl_window_set_net_wm_strut (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_STRUT, left, right, top, bottom, CARDINAL[4]/32

	  This property is equivalent to a _NET_WM_STRUT_PARTIAL property where all start values are 0 and all end values are the height or width of the
	  logical screen. _NET_WM_STRUT_PARTIAL was introduced later than _NET_WM_STRUT, however, so clients MAY set this property in addition to
	  _NET_WM_STRUT_PARTIAL to ensure backward compatibility with Window Managers supporting older versions of the Specification.
	*/

	warn_log("todo: _net_wm_strut");
}

void sl_window_set_net_wm_strut_partial (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_STRUT_PARTIAL, left, right, top, bottom, left_start_y, left_end_y,
	  right_start_y, right_end_y, top_start_x, top_end_x, bottom_start_x,
	  bottom_end_x,CARDINAL[12]/32

	  This property MUST be set by the Client if the window is to reserve space at the edge of the screen. The property contains 4 cardinals specifying
	  the width of the reserved area at each border of the screen, and an additional 8 cardinals specifying the beginning and end corresponding to each
	  of the four struts. The order of the values is left, right, top, bottom, left_start_y, left_end_y, right_start_y, right_end_y, top_start_x,
	  top_end_x, bottom_start_x, bottom_end_x. All coordinates are root window coordinates. The client MAY change this property at any time, therefore
	  the Window Manager MUST watch for property notify events if the Window Manager uses this property to assign special semantics to the window.

	  If both this property and the _NET_WM_STRUT property are set, the Window Manager MUST ignore the _NET_WM_STRUT property values and use instead the
	  values for _NET_WM_STRUT_PARTIAL. This will ensure that Clients can safely set both properties without giving up the improved semantics of the new
	  property.

	  The purpose of struts is to reserve space at the borders of the desktop. This is very useful for a docking area, a taskbar or a panel, for
	  instance. The Window Manager should take this reserved area into account when constraining window positions - maximized windows, for example,
	  should not cover that area.

	  The start and end values associated with each strut allow areas to be reserved which do not span the entire width or height of the screen. Struts
	  MUST be specified in root window coordinates, that is, they are not relative to the edges of any view port or Xinerama monitor.

	  For example, for a panel-style Client appearing at the bottom of the screen, 50 pixels tall, and occupying the space from 200-600 pixels from the
	  left of the screen edge would set a bottom strut of 50, and set bottom_start_x to 200 and bottom_end_x to 600. Another example is a panel on a
	  screen using the Xinerama extension. Assume that the set up uses two monitors, one running at 1280x1024 and the other to the right running at
	  1024x768, with the top edge of the two physical displays aligned. If the panel wants to fill the entire bottom edge of the smaller display with a
	  panel 50 pixels tall, it should set a bottom strut of 306, with bottom_start_x of 1280, and bottom_end_x of 2303. Note that the strut is relative
	  to the screen edge, and not the edge of the xinerama monitor.

	  Rationale: A simple "do not cover" hint is not enough for dealing with e.g. auto-hide panels.

	  Notes: An auto-hide panel SHOULD set the strut to be its minimum, hidden size. A "corner" panel that does not extend for the full length of a
	  screen border SHOULD only set one strut.
	*/

	warn_log("todo: _net_wm_strut_partial");
}

void sl_window_set_net_wm_icon_geometry (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_ICON_GEOMETRY, x, y, width, height, CARDINAL[4]/32

	  This optional property MAY be set by stand alone tools like a taskbar or an iconbox. It specifies the geometry of a possible icon in case the
	  window is iconified.

	  Rationale: This makes it possible for a Window Manager to display a nice animation like morphing the window into its icon.
	*/

	warn_log("todo: _net_wm_icon_geometry");
}

void sl_window_set_net_wm_icon (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_ICON CARDINAL[][2+n]/32

	  This is an array of possible icons for the client. This specification does not stipulate what size these icons should be, but individual desktop
	  environments or toolkits may do so. The Window Manager MAY scale any of these icons to an appropriate size.

	  This is an array of 32bit packed CARDINAL ARGB with high byte being A, low byte being B. The first two cardinals are width, height. Data is in
	  rows, left to right and top to bottom.
	*/

	warn_log("todo: _net_wm_icon");
}

void sl_window_set_net_wm_pid (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_PID CARDINAL/32

	  If set, this property MUST contain the process ID of the client owning this window. This MAY be used by the Window Manager to kill windows which
	  do not respond to the _NET_WM_PING protocol.

	  If _NET_WM_PID is set, the ICCCM-specified property WM_CLIENT_MACHINE MUST also be set. While the ICCCM only requests that WM_CLIENT_MACHINE is
	  set “ to a string that forms the name of the machine running the client as seen from the machine running the server” conformance to this
	  specification requires that WM_CLIENT_MACHINE be set to the fully-qualified domain name of the client's host.

	  See also the implementation notes on killing hung processes.
	*/

	warn_log("todo: _net_wm_pid");
}

void sl_window_set_net_wm_handled_icons (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_HANDLED_ICONS

	  This property can be set by a Pager on one of its own toplevel windows to indicate that the Window Manager need not provide icons for iconified
	  windows, for example if it is a taskbar and provides buttons for iconified windows.
	*/

	warn_log("todo: _net_wm_handled_icons");
}

void sl_window_set_net_wm_user_time (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_USER_TIME CARDINAL/32

	  This property contains the XServer time at which last user activity in this window took place.

	  Clients should set this property on every new toplevel window (or on the window pointed out by the _NET_WM_USER_TIME_WINDOW property), before
	  mapping the window, to the timestamp of the user interaction that caused the window to appear. A client that only deals with core events, might,
	  for example, use the timestamp of the last KeyPress or ButtonPress event. ButtonRelease and KeyRelease events should not generally be considered
	  to be user interaction, because an application may receive KeyRelease events from global keybindings, and generally release events may have later
	  timestamp than actions that were triggered by the matching press events. Clients can obtain the timestamp that caused its first window to appear
	  from the DESKTOP_STARTUP_ID environment variable, if the app was launched with startup notification. If the client does not know the timestamp of
	  the user interaction that caused the first window to appear (e.g. because it was not launched with startup notification), then it should not set
	  the property for that window. The special value of zero on a newly mapped window can be used to request that the window not be initially focused
	  when it is mapped.

	  If the client has the active window, it should also update this property on the window whenever there's user activity.

	  Rationale: This property allows a Window Manager to alter the focus, stacking, and/or placement behavior of windows when they are mapped depending
	  on whether the new window was created by a user action or is a "pop-up" window activated by a timer or some other event.
	*/

	warn_log("todo: _net_wm_user_time");
}

void sl_window_set_net_wm_user_time_window (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_USER_TIME_WINDOW WINDOW/32

	  This property contains the XID of a window on which the client sets the _NET_WM_USER_TIME property. Clients should check whether the window
	  manager supports _NET_WM_USER_TIME_WINDOW and fall back to setting the _NET_WM_USER_TIME property on the toplevel window if it doesn't.

	  Rationale: Storing the frequently changing _NET_WM_USER_TIME property on the toplevel window itself causes every application that is interested in
	  any of the properties of that window to be woken up on every keypress, which is particularly bad for laptops running on battery power.
	*/

	warn_log("todo: _net_wm_user_time_window");
}

void sl_window_set_net_frame_extents (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_FRAME_EXTENTS, left, right, top, bottom, CARDINAL[4]/32

	  The Window Manager MUST set _NET_FRAME_EXTENTS to the extents of the window's frame. left, right, top and bottom are widths of the respective
	  borders added by the Window Manager.
	*/

	warn_log("todo: _net_frame_extents");
}

void sl_window_set_net_wm_opaque_region (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_OPAQUE_REGION, x, y, width, height, CARDINAL[][4]/32

	  The Client MAY set this property to a list of 4-tuples [x, y, width, height], each representing a rectangle in window coordinates where the pixels
	  of the window's contents have a fully opaque alpha value. If the window is drawn by the compositor without adding any transparency, then such a
	  rectangle will occlude whatever is drawn behind it. When the window has an RGB visual rather than an ARGB visual, this property is not typically
	  useful, since the effective opaque region of a window is exactly the bounding region of the window as set via the shape extension. For windows
	  with an ARGB visual and also a bounding region set via the shape extension, the effective opaque region is given by the intersection of the region
	  set by this property and the bounding region set via the shape extension. The compositing manager MAY ignore this hint.

	  Rationale: This gives the compositing manager more room for optimizations. For example, it can avoid drawing occluded portions behind the window.
	*/

	warn_log("todo: _net_wm_opaque_region");
}

void sl_window_set_net_wm_bypass_compositor (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  _NET_WM_BYPASS_COMPOSITOR, CARDINAL/32

	  The Client MAY set this property to hint the compositor that the window would benefit from running uncomposited (i.e not redirected offscreen) or
	  that the window might be hurt from being uncomposited. A value of 0 indicates no preference. A value of 1 hints the compositor to disabling
	  compositing of this window. A value of 2 hints the compositor to not disabling compositing of this window. All other values are reserved and
	  should be treated the same as a value of 0. The compositing manager MAY bypass compositing for both fullscreen and non-fullscreen windows if
	  bypassing is requested, but MUST NOT bypass if it would cause differences from the composited appearance.

	  Rationale: Some applications like fullscreen games might want run without the overhead of being redirected offscreen (to avoid extra copies) and
	  thus perform better. An application which creates pop-up windows might always want to run composited to avoid exposes.
	*/

	warn_log("todo: _net_wm_bypass_compositor");
}

void sl_window_set_all_properties (sl_window* window, sl_display* display) {
	sl_set_window_name(window, display);
	sl_set_window_icon_name(window, display);
	sl_set_window_normal_hints(window, display);
	sl_set_window_hints(window, display);
	sl_set_window_class(window, display);
	sl_set_window_transient_for(window, display);
	sl_set_window_protocols(window, display);
	sl_set_window_colormap_windows(window, display);
	sl_set_window_client_machine(window, display);

	sl_window_set_net_wm_name(window, display);
	sl_window_set_net_wm_visible_name(window, display);
	sl_window_set_net_wm_icon_name(window, display);
	sl_window_set_net_wm_visible_icon_name(window, display);
	sl_window_set_net_wm_desktop(window, display);
	sl_window_set_net_wm_window_type(window, display);
	sl_window_set_net_wm_state(window, display);
	sl_window_set_net_wm_allowed_actions(window, display);
	sl_window_set_net_wm_strut(window, display);
	sl_window_set_net_wm_strut_partial(window, display);
	sl_window_set_net_wm_icon_geometry(window, display);
	sl_window_set_net_wm_icon(window, display);
	sl_window_set_net_wm_pid(window, display);
	sl_window_set_net_wm_handled_icons(window, display);
	sl_window_set_net_wm_user_time(window, display);
	sl_window_set_net_wm_user_time_window(window, display);
	sl_window_set_net_frame_extents(window, display);
	sl_window_set_net_wm_opaque_region(window, display);
	sl_window_set_net_wm_bypass_compositor(window, display);
}

static void window_state_change (sl_window* window, sl_display* display) {
	size_t i = 0;

	if (window->state & window_state_modal_bit) ++i;
	if (window->state & window_state_sticky_bit) ++i;
	if (window->state & window_state_maximized_vert_bit) ++i;
	if (window->state & window_state_maximized_horz_bit) ++i;
	if (window->state & window_state_shaded_bit) ++i;
	if (window->state & window_state_skip_taskbar_bit) ++i;
	if (window->state & window_state_skip_pager_bit) ++i;
	if (window->state & window_state_hidden_bit) ++i;
	if (window->state & window_state_fullscreen_bit) ++i;
	if (window->state & window_state_above_bit) ++i;
	if (window->state & window_state_below_bit) ++i;
	if (window->state & window_state_demands_attention_bit) ++i;
	if (window->state & window_state_focused_bit) ++i;

	Atom data[i];
	i = 0;

	if (window->state & window_state_modal_bit) {
		data[i] = display->atoms[net_wm_state_modal];
		++i;
	}
	if (window->state & window_state_sticky_bit) {
		data[i] = display->atoms[net_wm_state_sticky];
		++i;
	}
	if (window->state & window_state_maximized_vert_bit) {
		data[i] = display->atoms[net_wm_state_maximized_vert];
		++i;
	}
	if (window->state & window_state_maximized_horz_bit) {
		data[i] = display->atoms[net_wm_state_maximized_horz];
		++i;
	}
	if (window->state & window_state_shaded_bit) {
		data[i] = display->atoms[net_wm_state_shaded];
		++i;
	}
	if (window->state & window_state_skip_taskbar_bit) {
		data[i] = display->atoms[net_wm_state_skip_taskbar];
		++i;
	}
	if (window->state & window_state_skip_pager_bit) {
		data[i] = display->atoms[net_wm_state_skip_pager];
		++i;
	}
	if (window->state & window_state_hidden_bit) {
		data[i] = display->atoms[net_wm_state_hidden];
		++i;
	}
	if (window->state & window_state_fullscreen_bit) {
		data[i] = display->atoms[net_wm_state_fullscreen];
		++i;
	}
	if (window->state & window_state_above_bit) {
		data[i] = display->atoms[net_wm_state_above];
		++i;
	}
	if (window->state & window_state_below_bit) {
		data[i] = display->atoms[net_wm_state_below];
		++i;
	}
	if (window->state & window_state_demands_attention_bit) {
		data[i] = display->atoms[net_wm_state_demands_attention];
		++i;
	}
	if (window->state & window_state_focused_bit) {
		data[i] = display->atoms[net_wm_state_focused];
		++i;
	}

	XChangeProperty(display->x_display, window->x_window, display->atoms[net_wm_state], XA_ATOM, 32, PropModeReplace, (uchar*)data, i);
}

void sl_window_set_fullscreen (sl_window* window, sl_display* display, bool fullscreen) {
	if (fullscreen)
		((sl_window_mutable*)window)->state |= window_state_fullscreen_bit;
	else
		((sl_window_mutable*)window)->state &= all_window_states - window_state_fullscreen_bit;

	window_state_change(window, display);
}

void sl_window_toggle_fullscreen (sl_window* window, sl_display* display) {
	return sl_window_set_fullscreen(window, display, !(window->state & window_state_fullscreen_bit));
}
