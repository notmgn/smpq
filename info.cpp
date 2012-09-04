/*
    info.cpp - StormLib MPQ archiving utility
    Copyright (C) 2010 - 2011  Pali Rohár <pali.rohar@gmail.com>

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

static inline unsigned int GetInfo(HANDLE SArchive, unsigned int info) {

	unsigned int ret;

	if ( SFileGetFileInfo(SArchive, info, &ret, sizeof(ret), NULL) )
		return ret;
	else
		return 0;

}

int info(const char * archive, unsigned int flags) {

	HANDLE SArchive = NULL;

	unsigned int streamFlags;
	unsigned int verify;

	unsigned int SFlags = STREAM_FLAG_READ_ONLY;

	if ( flags & MPQ_VERSION_1 )
		SFlags |= MPQ_OPEN_FORCE_MPQ_V1;

	if ( flags & MPQ_ENCRYPTED )
		SFlags |= STREAM_PROVIDER_ENCRYPTED;

	if ( ! SFileOpenArchive(archive, 0, SFlags, &SArchive) ) {

		printError(archive, "Cannot open archive", archive, GetLastError());
		return -1;

	}

	printMessage("Archive name: %s", archive);
	printMessage("Archive size: %u", GetInfo(SArchive, SFILE_INFO_ARCHIVE_SIZE));
	printMessage("Number of files in archive: %u", GetInfo(SArchive, SFILE_INFO_NUM_FILES));
	printMessage("Maximum file count of archive: %u", GetInfo(SArchive, SFILE_INFO_MAX_FILE_COUNT));
	printMessage("Hash table size: %u", GetInfo(SArchive, SFILE_INFO_HASH_TABLE_SIZE));
	printMessage("Block table size: %u", GetInfo(SArchive, SFILE_INFO_BLOCK_TABLE_SIZE));
	printMessage("Sector size: %u", GetInfo(SArchive, SFILE_INFO_SECTOR_SIZE));

	streamFlags = GetInfo(SArchive, SFILE_INFO_STREAM_FLAGS);

	if ( streamFlags & STREAM_PROVIDER_PARTIAL )
		printMessage("Archive partial: Yes");
	else
		printMessage("Archive partial: No");

	if ( streamFlags & STREAM_PROVIDER_ENCRYPTED )
		printMessage("Archive encryped: Yes");
	else
		printMessage("Archive encryped: No");

	verify = SFileVerifyArchive(SArchive);

	if ( verify == ERROR_NO_SIGNATURE )
		printMessage("Archive signature: No signature");
	else if ( verify == ERROR_VERIFY_FAILED )
		printMessage("Archive signature: Verification failed");
	else if ( verify == ERROR_WEAK_SIGNATURE_OK )
		printMessage("Archive signature: Weak digital signature - Valid");
	else if ( verify == ERROR_WEAK_SIGNATURE_ERROR )
		printMessage("Archive signature: Weak digital signature - Invalid");
	else if ( verify == ERROR_STRONG_SIGNATURE_OK )
		printMessage("Archive signature: Strong digital signature - Valid");
	else if ( verify == ERROR_STRONG_SIGNATURE_ERROR )
		printMessage("Archive signature: Strong digital signature - Invalid or No public key");

	SFileCloseArchive(SArchive);

	return 0;

}

///
}
///
