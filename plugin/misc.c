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

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "misc.h"

/*!
 * \brief A method to make sure a particular directory exists.
 *
 *
 * \param path The path that needs to exist
 *
 * \return true if directory exists, false if directory does not exist
 */
bool make_sure_dir_exists(char *path) {

	bool ret = false;

	DIR *dir;
	dir = opendir(path);
	if (dir) {
		ret = true;
		closedir(dir);
	} else if (mkdir(path, 0777)==0) {
		ret = true;
	}

	return ret;

}

/*!
 * \brief Create a timestamp in the form 'YYYY-MM-DD_hhmmss'.
 *
 * \param timestamp A string to hold the timestamp.
 */
void get_timestamp_string(char *timestamp) {

	struct tm *now = NULL;
	int hour = 0;
	time_t time_value = 0;

	time_value = time(NULL);
	now = localtime(&time_value);

	sprintf(timestamp, "%d-%.2d-%.2d_%.2d%.2d%.2d", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

}

/*!
 * \brief Remove a directory and all the files in it.
 *
 *
 * \param path The path to the directory to be removed.
 *
 * \return 0 on success, -1 on error
 */
int remove_dir(char *path) {
	struct dirent *d;
	DIR *dir = opendir(path);
	char file[PATH_MAX];
	while(d = readdir(dir)) {
		sprintf(file, "%s/%s", path, d->d_name);
		remove(file);
	}
	closedir(dir);
	return remove(path);
}

/*!
 * \brief Remove 0-length files in a directory.
 *
 *
 * \param path The path of the directory to be cleaned of 0-length files.
 * \param removed The number of files removed.
 *
 * \return 0 for success, -1 if an error removing a file occurred.
 *
 */
int clean_dir(char *path, int removed) {
	int ret = 0;
	removed = 0;
	struct dirent *d;
	DIR *dir = opendir(path);
	char file[PATH_MAX];
	while(d = readdir(dir)) {
		sprintf(file, "%s/%s", path, d->d_name);
		struct stat st;
		stat(file, &st);
		if (S_ISREG(st.st_mode) && st.st_size==0) {
			if (remove(file)==0)
				removed++;
			else ret = -1;
		}
	}
	closedir(dir);
	return ret;
}
