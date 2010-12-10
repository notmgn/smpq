/*
    print.c - StormLib MPQ archiving utility
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

#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(WIN32) || defined(_MSC_VER)
#include <windows.h>
#endif

#include "common.h"

void printError(const char * archive, const char * message, const char * file, int errnum) {

	char * error = NULL;

#if defined(WIN32) || defined(_MSC_VER)

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errnum, 0, error, 0, NULL);

#elif defined(__APPLE__)

	/* TODO: Convert errnum to message */
	error = malloc(20 * sizeof(char));
	sprintf(error, "Error code %d", errnum);

#else

	if ( errnum < 105 )
		error = strerror(errnum);
	else if ( errnum == 105 )
		error = (char *)"Bad format";
	else if ( errnum == 106 )
		error = (char *)"No more files";
	else if ( errnum == 107 )
		error = (char *)"Handle EOF";
	else if ( errnum == 108 )
		error = (char *)"Cannot compile";
	else if ( errnum == 109 )
		error = (char *)"File corrupted";

#endif

	fprintf(stderr, "%s: %s: Error: %s `%s': %s\n", app, archive, message, file, error);
	fflush(stderr);

#if defined(WIN32) || defined(_MSC_VER)

	LocalFree(error);

#elif defined(__APPLE__)

	free(error);

#endif

}

void printVerbose(const char * archive, const char * message, const char * file) {

	printf("%s: %s: %s `%s' ...\n", app, archive, message, file);
	fflush(stdout);

}
