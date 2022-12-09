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

#include <X11/Xutil.h>

#include "compiler-differences.h"
#include "display.h"

typedef struct sl_window_mutable {
	Window x_window;
	bool started, mapped, fullscreen, maximized;

	sl_window_dimensions saved_dimensions;

	workspace_type workspace;

	struct sl_window_have_protocols have_protocols;
} sl_window_mutable;

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

	warn_log("todo: wm_name");

	XTextProperty text_proterty;

	XGetWMName(display->x_display, window->x_window, &text_proterty);

	for (size_t i = 0; i < 64 && i < text_proterty.nitems; ++i) {
		warn_log("hardcoding window->name size");
		window->name[i] = text_proterty.value[i];
	}
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

	warn_log("todo: wm_icon_name");

	XTextProperty text_proterty;

	XGetWMIconName(display->x_display, window->x_window, &text_proterty);

	for (size_t i = 0; i < 64 && i < text_proterty.nitems; ++i) {
		warn_log("hardcoding window->icon_name size");
		window->icon_name[i] = text_proterty.value[i];
	}
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

	warn_log("todo: wm_normal_hints");
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

	warn_log("todo: wm_window_hints");
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

	for (size_t i = 0; i <= (size_t)n; ++i) {
		if (protocols[i] == display->atoms[wm_take_focus]) {
			((sl_window_mutable*)window)->have_protocols.take_focus = true;
			continue;
		}

		if (protocols[i] == display->atoms[wm_take_focus]) {
			((sl_window_mutable*)window)->have_protocols.delete_window = true;
			continue;
		}
	}

	XFree(protocols);
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

	warn_log("todo: wm_window_colormaps");
}

void sl_set_window_client_machine (M_maybe_unused sl_window* window, M_maybe_unused sl_display* display) {
	/*
	  The client should set the WM_CLIENT_MACHINE property (of one of the TEXT
	  types) to a string that forms the name of the machine running the client as seen
	  from the machine running the server.
	*/

	warn_log("todo: wm_client_machine");
}
