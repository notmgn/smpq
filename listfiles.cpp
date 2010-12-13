/*
    listfiles.cpp - StormLib MPQ archiving utility
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

#include <StormLib.h>

///
extern "C" {
///

#if defined(WIN32) || defined(_MSC_VER)
void systemListfiles(HANDLE Archive) { }
#else

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

#include "common.h"

#define LISTPATH "/usr/share/stormlib"

void systemListfiles(HANDLE Archive, const char * archive, int flags) {

	DIR * dir = opendir(LISTPATH);

	if ( ! dir )
		return;

	struct dirent * ent;

	while ( ( ent = readdir(dir) ) ) {

		if ( strcasecmp(ent->d_name+strlen(ent->d_name)-4, ".txt") != 0 )
			continue;

		char listfile[strlen(LISTPATH)+strlen(ent->d_name)+2];

		strcpy(listfile, LISTPATH);
		strcpy(listfile+strlen(LISTPATH)+1, ent->d_name);
		listfile[strlen(LISTPATH)] = '/';

		struct stat st;

		if ( stat(listfile, &st) == -1 )
			continue;

		if ( S_ISDIR(st.st_mode) )
			continue;

		if ( flags & VERBOSE )
			printVerbose(archive, "Loading system listfile", listfile);

		SFileAddListFile(Archive, listfile);

	}

}

#endif

///
}
///
