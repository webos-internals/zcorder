/*=============================================================================
 Copyright (C) 2009 Ryan Hope <rmh3093@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 =============================================================================*/

#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <syslog.h>

#include "zcorder.h"
#include "misc.h"

char *tmpDir;

static struct option long_options[] = {
		{ "help",		no_argument,		0, 'h' },
		{ "version",	no_argument,		0, 'V' },
		{ 0, 0, 0, 0 }
};

void print_version() {
	printf("zcorder - zcorder service daemon (%s)\n", VERSION);
}

void print_help(char *argv[]) {

	printf("Usage: %s [OPTION]...\n\n"
			"Miscellaneous:\n"
			"  -h, --help\t\tprint help information and exit\n"
			"  -V, --version\t\tprint version information and exit\n", argv[0]);

}

int getopts(int argc, char *argv[]) {

	int c, retVal = 0;

	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "Vh", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'V':
			print_version();
			retVal = 1;
			break;
		case 'h':
			print_help(argv);
			retVal = 1;
			break;
		default:
			abort();
		}
	}
	return retVal;

}

void cleanup() {

	chdir("/tmp");
	remove_dir(tmpDir);
	free(tmpDir);

	int removed = 0;
	clean_dir(DEFAULT_FILE_LOCATION, removed);
	if (removed)
		printf("Removed %d 0-length files from %s", removed, DEFAULT_FILE_LOCATION);

}

void sighandler(int sig) {
	cleanup();
	exit(0);
}

int main(int argc, char *argv[]) {

	syslog(LOG_INFO, "Is it starting at all? zsoc");

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGQUIT, sighandler);

	if (!make_sure_dir_exists(DEFAULT_FILE_LOCATION)) {
		printf("Directory error: %s\n", DEFAULT_FILE_LOCATION);
		return 1;
	}

	char template[] = "/tmp/zcorder.XXXXXX";
	tmpDir = strdup(mkdtemp(template));
	chdir(tmpDir);

	if (getopts(argc, argv) == 1)
		return 1;

	struct sched_param sp;
	sp.sched_priority = sched_get_priority_max(SCHED_RR);
	sched_setscheduler(getpid(), SCHED_RR, &sp);

	start_service();

	cleanup();
	PDL_Quit();

	return 0;

}
