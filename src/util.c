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

#include "util.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include "compiler-differences.h"
#include "message.h"

void signal_handler (int signal_number) {
	if (signal_number == SIGCHLD) {
		int status = 0;
		pid_t ret = waitpid(-1, &status, WNOHANG);
		if (ret < 0) {
			perror("waitpid");
			assert_not_reached();
		}
		if (WIFEXITED(status)) {
			int ret = WEXITSTATUS(status);
			if (ret != 0) warn_log_va("Warning: child process returned %i\n", ret);
		}
	}
}

int xerror_handler (Display* display, XErrorEvent* p_error_event) {
	XErrorEvent* const error_event = p_error_event;
	char error_text[4096];
	XGetErrorText(display, error_event->error_code, error_text, 4096);
	warn_log_va(
	"XError (%i): %s\n"
	"serial: %li\n"
	"opcode: %i.%i\n"
	"resource id: %lu\n",
	error_event->error_code, error_text, error_event->serial, error_event->request_code, error_event->minor_code, error_event->resourceid
	);
	assert_not_reached();
}

int xio_error_handler (M_maybe_unused Display* display) { assert_not_reached(); }

void exec_program (M_maybe_unused Display* display, char* const* args) {
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		assert_not_reached();
	}
	if (pid == 0) {
		// fclose(stdin);
		// fclose(stderr);
		// fclose(stdout);
		if (setsid() == -1) {
			perror("setsid");
			assert_not_reached();
		}
		if (execvp(args[0], args)) {
			perror("execvp");
			assert_not_reached();
		}
		exit(0);
	}
}
