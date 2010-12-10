/*
    info.cpp - StormLib MPQ archiving utility
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

#include "common.h"

int info(const char * archive) {

	HANDLE SArchive = NULL;

	// TODO: Add flags
	if ( ! SFileOpenArchive(archive, 0, MPQ_OPEN_READ_ONLY /*MPQ_OPEN_CHECK_SECTOR_CRC*/, &SArchive) ) {

		printError(archive, "Cannot open archive", archive, GetLastError());
		return -1;

	}

	// TODO: Show more info
	printMessage("Archive Name: %s", archive);

	SFileCloseArchive(SArchive);

	return 0;

}

///
}
///
