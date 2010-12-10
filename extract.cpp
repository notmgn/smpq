/*
    extract.cpp - StormLib MPQ archiving utility
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>

#include "common.h"

int extract(const char * archive, const char * const files[], int flags, const char * listfile) {

	int i, j;
	HANDLE SArchive = NULL;

	if ( ! SFileOpenArchive(archive, 0, MPQ_OPEN_READ_ONLY, &SArchive) ) {

		printError(archive, "Cannot open archive", archive, GetLastError());
		return -1;

	}

	if ( ! ( flags & NO_SYSTEM ) )
		systemListfiles(SArchive, archive, flags);

	for ( i = 0; files[i]; ++i ) {

		char mask[strlen(files[i])+1];
		convertPathToArchive(mask, files[i]);

		SFILE_FIND_DATA SFileFindData;
		HANDLE SFileFind = SFileFindFirstFile(SArchive, mask, &SFileFindData, listfile);

		if ( ! SFileFind ) {

			SFileFind = (HANDLE)0xFFFFFFFF;
			SFileFindData.dwFileTimeLo = 0;
			SFileFindData.dwFileTimeHi = 0;

			strcpy(SFileFindData.cFileName, mask);

			HANDLE SFile;

			if ( SFileOpenFileEx(SArchive, mask, 0, &SFile) ) {

				unsigned int high = 0;
				unsigned int low = SFileGetFileSize(SFile, &high);

				SFileFindData.dwFileSize = low | ( (unsigned long long int)high << 32 );

				SFileCloseFile(SFile);

			}

		}

		while ( SFileFind ) {

			struct stat st;
			FILE * file = NULL;
			char fileName[strlen(SFileFindData.cFileName)+1];
			char fileDir[strlen(SFileFindData.cFileName)+1];
			size_t fileSize = SFileFindData.dwFileSize;
			time_t fileTime = 0;

			HANDLE SFile = NULL;
			const char * SFileName = SFileFindData.cFileName;
			unsigned long long int SFileTime = SFileFindData.dwFileTimeLo | ( ((unsigned long long int)SFileFindData.dwFileTimeHi) << 32 );

			int last = 0;
			char buffer[0x10000];
			size_t bytes = 1;

			if ( strcmp(SFileName, "(listfile)") == 0 || strcmp(SFileName, "(signature)") == 0 || strcmp(SFileName, "(attributes)") == 0 )
				goto next;

			convertPathFromArchive(fileName, SFileName);

			if ( ! GetTimeFromFileTime(SFileTime, &fileTime) )
				fileTime = 0;

			if ( ! SFileOpenFileEx(SArchive, SFileName, 0, &SFile) ) {

				printError(archive, "Cannot open file", SFileName, GetLastError());
				goto next;

			}

			j = -1;

			while ( SFileName[++j] )
				if ( SFileName[j] == '\\' )
					last = j;

			if ( ( flags & VERBOSE ) && ! ( flags & LIST ) )
				printVerbose(archive, "Extract", fileName);

			if ( ( flags & LIST ) ) {

				char strtime[80];
				strftime(strtime, 80, "%Y-%m-%d %H:%M", localtime(&fileTime));
				printMessage("%12u %s %s", fileSize, strtime, fileName);

			}

			if ( flags & LIST )
				goto next;

			memcpy(fileDir, fileName, last);
			fileDir[last] = 0;

			if ( last != 0 ) {

				if ( mkpath(fileDir, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) != 0 ) {

					printError(archive, "Cannot create directory", fileDir, errno);
					printError(archive, "Cannot extract file", fileName, ENOENT);
					goto next;

				}

			}

			if ( stat(fileName, &st) != -1 ) {

				if ( ! ( flags & OVERWRITE ) ) {

					printError(archive, "Cannot extract file", fileName, EEXIST);
					goto next;

				}

				if ( S_ISDIR(st.st_mode) ) {

					printError(archive, "Cannot extract file", fileName, EISDIR);
					goto next;

				}

				unlink(fileName);

			}

			file  = fopen(fileName, "wb");

			if ( ! file ) {

				printError(archive, "Cannot open file", fileName, errno);
				goto next;

			}

			while ( 1 ) {

				int eof = 0;

				if ( ! SFileReadFile(SFile, buffer, sizeof(buffer), &bytes, NULL) ) {

					eof = GetLastError() == ERROR_HANDLE_EOF;
				       
					if ( ! eof ) {

						printError(archive, "Cannot read file", SFileName, GetLastError());
						break;

					}

				}

				if ( fwrite(buffer, 1, bytes, file) != bytes ) {

					printError(archive, "Cannot write file", fileName, errno);
					break;

				}

				if ( eof )
					break;

			}

			fclose(file);

			{
				struct utimbuf fileTimeBuf = { fileTime, fileTime };
				utime(fileName, &fileTimeBuf);
			}

next:
			if ( SFile )
				SFileCloseFile(SFile);

			if ( SFileFind == (HANDLE)0xFFFFFFFF )
				break;

			if ( ! SFileFindNextFile(SFileFind, &SFileFindData) )
				break;

		}

		if ( SFileFind != (HANDLE)0xFFFFFFFF )
			SFileFindClose(SFileFind);

	}

	SFileCloseArchive(SArchive);

	return 0;

}

///
}
///
