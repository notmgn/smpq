/*
    mkpath.c - StormLib MPQ archiving utility
    Copyright (C) 2010  Pali Rohár <pali.rohar@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"

int mkpath(const char * s, mode_t mode) {

	char * q = NULL;
	char * r = NULL;
	char * path = NULL;
	char * up = NULL;
	int rv = -1;

	if ( strcmp(s, ".") == 0 || strcmp(s, "/") == 0 )
		return 0;

	if ( ( path = strdup(s) ) == NULL )
		return -1;

	if ( ( q = strdup(s) ) == NULL )
		return -1;

	if ( ( r = (char *)dirname(q) ) == NULL )
		goto out;

	if ( ( up = strdup(r) ) == NULL )
		return -1;

	if ( ( mkpath(up, mode) == -1 ) && ( errno != EEXIST ) )
		goto out;

	if ( ( mkdir(path, mode) != -1 ) || ( errno == EEXIST ) )
		rv = 0;

out:
	if ( up != NULL )
		free(up);

	free(q);
	free(path);

        return rv;

}
