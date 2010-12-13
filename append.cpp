/*
    append.cpp - StormLib MPQ archiving utility
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

#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <errno.h>

#include "common.h"

int append(const char * archive, const char * const files[], int flags, const char * listfile) {

	int i;
	int needCompact = 0;
	HANDLE SArchive = NULL;

	if ( flags & CREATE ) {

		struct stat st;

		if ( ( flags & OVERWRITE ) && stat(archive, &st) == 0 ) {

			printVerbose(archive, "Remove old archive", archive);

			if ( unlink(archive) != 0 ) {

				printError(archive, "Cannot remove existing archive", archive, errno);
				return -1;

			}

		}

		if ( flags & VERBOSE )
			printVerbose(archive, "Create new archive", archive);

		// TODO: Configure hash table size
		if ( ! SFileCreateArchive(archive, MPQ_CREATE_ARCHIVE_V2 | MPQ_CREATE_ATTRIBUTES, 1 << 4 /*HASH_TABLE_SIZE_MAX*/ , &SArchive) ) {

			printError(archive, "Cannot create archive", archive, GetLastError());
			return -1;

		}

	} else {

		// TODO: Add flags
		if ( ! SFileOpenArchive(archive, 0, 0 /*MPQ_OPEN_CHECK_SECTOR_CRC*/, &SArchive) ) {

			printError(archive, "Cannot open archive", archive, GetLastError());
			return -1;

		}

	}

	if ( ! ( flags & NO_SYSTEM ) )
		systemListfiles(SArchive, archive, flags);

	for ( i = 0; files[i]; ++i ) {

		struct stat st;
		FILE * file = NULL;
		const char * fileName = files[i];
		size_t fileSize = 0;

		HANDLE SFile = NULL;
		char SFileName[strlen(fileName) + 1];
		unsigned long long int SFileTime = 0;

		char buffer[0x10000];
		size_t bytes = 0;

		toArchivePath(SFileName, fileName);

		if ( strlen(SFileName) == 16 && strncasecmp(SFileName, "File", 4) == 0 && SFileName[12] == '.' ) {

			printError(archive, "File with mask `File????????.???\' is not allowed. Cannot create new file", SFileName, EPERM);
			continue;

		}

		if ( strcasecmp(SFileName, "(listfile)") == 0 || strcasecmp(SFileName, "(signature)") == 0 || strcasecmp(SFileName, "(attributes)") == 0 ) {

			printError(archive, "Files `(listfile)' `(signature)' `(attributes)' are for internal usage. Cannot create new file", SFileName, EPERM);
			continue;

		}

		file = fopen(fileName, "rb");

		if ( ! file ) {

			printError(archive, "Cannot open file", fileName, errno);
			continue;

		}

		fseek(file, 0, SEEK_END);
		fileSize = ftell(file);
		rewind(file);

		if ( ( flags & OVERWRITE ) && SFileOpenFileEx(SArchive, SFileName, SFILE_OPEN_FROM_MPQ, &SFile) ) {

			SFileCloseFile(SFile);

			if ( flags & VERBOSE )
				printVerbose(archive, "Remove old file", SFileName);

			if ( ! SFileRemoveFile(SArchive, SFileName, SFILE_OPEN_FROM_MPQ) ) {

				printError(archive, "Cannot remove existing file", SFileName, GetLastError());
				fclose(file);
				continue;

			}

			needCompact = 1;

		}

		if ( flags & VERBOSE )
			printVerbose(archive, "Append file", SFileName);

		if ( stat(fileName, &st) == -1 ) {

			printError(archive, "Cannot stat file", fileName, errno);
			fclose(file);
			continue;

		}

		toFileTime(&SFileTime, st.st_mtime);

		// TODO: Add flags
		if ( ! SFileCreateFile(SArchive, SFileName, SFileTime, fileSize, 0 /*locale*/, MPQ_FILE_COMPRESS, &SFile) ) {

			printError(archive, "Cannot create new file", SFileName, GetLastError());
			fclose(file);
			continue;

		}

		while ( 1 ) {

			bytes = fread(buffer, 1, sizeof(buffer), file);

			if ( ferror(file) ) {

				printError(archive, "Cannot read file", fileName, errno);
				break;

			}

			if ( ! SFileWriteFile(SFile, buffer, bytes, MPQ_COMPRESSION_LZMA) ) {

				printError(archive, "Cannot write file new", SFileName, GetLastError());
				break;

			}

			if ( feof(file) )
				break;

		}

		fclose(file);
		SFileFinishFile(SFile);
		SFileFlushArchive(SArchive);

	}

	if ( needCompact ) {

		if ( flags & VERBOSE )
			printVerbose(archive, "Compact archive", archive);

		if ( ! SFileCompactArchive(SArchive, listfile, 0) )
			printError(archive, "Cannot compact archive", archive, GetLastError());

	}

	SFileCloseArchive(SArchive);

	return 0;

}

///
}
///
