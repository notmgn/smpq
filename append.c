/*
    append.cpp - StormLib MPQ archiving utility
    Copyright (C) 2010 - 2016  Pali Rohár <pali.rohar@gmail.com>

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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#if !defined(WIN32) && !defined(_MSC_VER)
#include <dirent.h>
#endif

#if defined(WIN32) || defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#include "common.h"

#if !defined(WIN32) && !defined(_MSC_VER)

static int smpq_add_file_to_list(char *** list, unsigned int * count, unsigned int * capacity, const char * path) {

	char ** newList;
	char * copy;

	if ( *count >= *capacity ) {

		unsigned int newCapacity = ( *capacity == 0 ) ? 256 : ( *capacity * 2 );

		newList = (char **)realloc(*list, newCapacity * sizeof(char *));

		if ( newList == NULL )
			return -1;

		*list = newList;
		*capacity = newCapacity;

	}

	copy = strdup(path);

	if ( copy == NULL )
		return -1;

	(*list)[*count] = copy;
	++(*count);

	return 0;

}

static int smpq_collect_files(const char * path, char *** list, unsigned int * count, unsigned int * capacity) {

	struct stat st;

	if ( lstat(path, &st) != 0 )
		return -1;

	if ( ! S_ISDIR(st.st_mode) )
		return smpq_add_file_to_list(list, count, capacity, path);

	{

		DIR * dir;
		struct dirent * entry;

		dir = opendir(path);

		if ( dir == NULL )
			return -1;

		while ( ( entry = readdir(dir) ) != NULL ) {

			char child[1024];
			int n;

			if ( strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 )
				continue;

			if ( strcmp(path, ".") == 0 || strcmp(path, "./") == 0 )
				n = snprintf(child, sizeof(child), "./%s", entry->d_name);
			else
				n = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

			if ( n < 0 || (size_t)n >= sizeof(child) ) {

				closedir(dir);
				return -1;

			}

			if ( smpq_collect_files(child, list, count, capacity) != 0 ) {

				closedir(dir);
				return -1;

			}

		}

		closedir(dir);

	}

	return 0;

}

#endif

#if !defined(WIN32) && !defined(_MSC_VER)
#define SMPQ_FREE_EXPANDED_FILES(expandedFiles) \
	do { \
		unsigned int freeIndex; \
		for ( freeIndex = 0; (expandedFiles) && (expandedFiles)[freeIndex]; ++freeIndex ) \
			free((expandedFiles)[freeIndex]); \
		free(expandedFiles); \
	} while ( 0 )
#else
#define SMPQ_FREE_EXPANDED_FILES(expandedFiles) ((void)0)
#endif

int smpq_append(const char * archive, const char * const files[], unsigned int flags, unsigned int locale, unsigned int maxFileCount, const char * compression) {

	unsigned int SFlags = 0;
	unsigned int SCompFlags = 0;

	int i;
	HANDLE SArchive = NULL;

#if !defined(WIN32) && !defined(_MSC_VER)

	char ** expandedFiles = NULL;
	unsigned int expandedCount = 0;
	unsigned int expandedCapacity = 0;
	const char * const * inputFiles = files;

	for ( i = 0; files[i]; ++i ) {

		struct stat st;

		if ( lstat(files[i], &st) != 0 ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "Cannot stat file", files[i], errno);

			continue;

		}

		if ( S_ISDIR(st.st_mode) ) {

			if ( smpq_collect_files(files[i], &expandedFiles, &expandedCount, &expandedCapacity) != 0 ) {

				if ( ! ( flags & QUIET ) )
					printError(archive, "Cannot recursively enumerate directory", files[i], errno);

				SMPQ_FREE_EXPANDED_FILES(expandedFiles);
				return -1;

			}

		} else {

			if ( smpq_add_file_to_list(&expandedFiles, &expandedCount, &expandedCapacity, files[i]) != 0 ) {

				SMPQ_FREE_EXPANDED_FILES(expandedFiles);
				return -1;

			}

		}

	}

	if ( expandedCount >= expandedCapacity ) {

		char ** newList;
		unsigned int newCapacity = ( expandedCapacity == 0 ) ? 1 : expandedCapacity + 1;

		newList = (char **)realloc(expandedFiles, newCapacity * sizeof(char *));

		if ( newList == NULL ) {

			SMPQ_FREE_EXPANDED_FILES(expandedFiles);
			return -1;

		}

		expandedFiles = newList;
		expandedCapacity = newCapacity;

	}

	expandedFiles[expandedCount] = NULL;
	inputFiles = (const char * const *)expandedFiles;

	files = inputFiles;

#endif

	if ( flags & ENCRYPT )
		SFlags |= MPQ_FILE_ENCRYPTED;

	if ( flags & FIX_KEY )
		SFlags |= MPQ_FILE_FIX_KEY;

	if ( flags & DELETE_MARKER )
		SFlags |= MPQ_FILE_DELETE_MARKER;

	if ( flags & SECTOR_CRC )
		SFlags |= MPQ_FILE_SECTOR_CRC;

	if ( flags & SINGLE_UNIT )
		SFlags |= MPQ_FILE_SINGLE_UNIT;

	if ( flags & OVERWRITE )
		SFlags |= MPQ_FILE_REPLACEEXISTING;

	if ( compression != NULL ) {

		if ( strcmp(compression, "none") == 0 ) {

		} else if ( strcasecmp(compression, "IMPLODE") == 0 ) {

			SFlags |= MPQ_FILE_IMPLODE;

		} else {

			SFlags |= MPQ_FILE_COMPRESS;

			if ( strcasecmp(compression, "HUFFMANN") == 0 )
				SCompFlags |= MPQ_COMPRESSION_HUFFMANN;
			else if ( strcasecmp(compression, "ADPCM_MONO") == 0 )
				SCompFlags |= MPQ_COMPRESSION_ADPCM_MONO;
			else if ( strcasecmp(compression, "ADPCM_STEREO") == 0 )
				SCompFlags |= MPQ_COMPRESSION_ADPCM_STEREO;
			else if ( strcasecmp(compression, "ZLIB") == 0 )
				SCompFlags |= MPQ_COMPRESSION_ZLIB;
			else if ( strcasecmp(compression, "PKWARE") == 0 )
				SCompFlags |= MPQ_COMPRESSION_PKWARE;
			else if ( strcasecmp(compression, "BZIP2") == 0 )
				SCompFlags |= MPQ_COMPRESSION_BZIP2;
			else if ( strcasecmp(compression, "SPARSE") == 0 )
				SCompFlags |= MPQ_COMPRESSION_SPARSE;
			else if ( strcasecmp(compression, "LZMA") == 0 )
				SCompFlags |= MPQ_COMPRESSION_LZMA;
			else if ( strcasecmp(compression, "HUFFMANN+ADPCM_MONO") == 0 )
				SCompFlags |= MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO;
			else if ( strcasecmp(compression, "HUFFMANN+ADPCM_STEREO") == 0 )
				SCompFlags |= MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_STEREO;
			else if ( strcasecmp(compression, "ZLIB+PKWARE") == 0 )
				SCompFlags |= MPQ_COMPRESSION_ZLIB | MPQ_COMPRESSION_PKWARE;
			else if ( strcasecmp(compression, "BZIP2+PKWARE") == 0 )
				SCompFlags |= MPQ_COMPRESSION_BZIP2 | MPQ_COMPRESSION_PKWARE;
			else if ( strcasecmp(compression, "SPARSE+ZLIB") == 0 )
				SCompFlags |= MPQ_COMPRESSION_SPARSE | MPQ_COMPRESSION_ZLIB;
			else if ( strcasecmp(compression, "SPARSE+PKWARE") == 0 )
				SCompFlags |= MPQ_COMPRESSION_SPARSE | MPQ_COMPRESSION_PKWARE;
			else if ( strcasecmp(compression, "SPARSE+BZIP2") == 0 )
				SCompFlags |= MPQ_COMPRESSION_SPARSE | MPQ_COMPRESSION_BZIP2;
			else if ( strcasecmp(compression, "SPARSE+ZLIB+PKWARE") == 0 )
				SCompFlags |= MPQ_COMPRESSION_SPARSE | MPQ_COMPRESSION_ZLIB | MPQ_COMPRESSION_PKWARE;
			else if ( strcasecmp(compression, "SPARSE+BZIP2+PKWARE") == 0 )
				SCompFlags |= MPQ_COMPRESSION_SPARSE | MPQ_COMPRESSION_BZIP2 | MPQ_COMPRESSION_PKWARE;
			else if ( strcasecmp(compression, "choose") == 0 ) {

				if ( ! ( flags & QUIET ) )
					printError(archive, "Choose the best compression is not implemented yet", compression, EINVAL);

				SMPQ_FREE_EXPANDED_FILES(expandedFiles);
				return -1;

			} else {

				if ( ! ( flags & QUIET ) )
					printError(archive, "Specified unknown compression method", compression, EINVAL);

				SMPQ_FREE_EXPANDED_FILES(expandedFiles);
				return -1;

			}

		}

	}

	if ( flags & CREATE ) {

		struct stat st;
		unsigned int SOpenFlags = 0;

		if ( ( flags & OVERWRITE ) && stat(archive, &st) == 0 ) {

			if ( flags & VERBOSE )
				printVerbose(archive, "Remove old archive", archive);

			if ( unlink(archive) != 0 ) {

				if ( ! ( flags & QUIET ) )
					printError(archive, "Cannot remove existing archive", archive, errno);

				SMPQ_FREE_EXPANDED_FILES(expandedFiles);
				return -1;

			}

		}

		if ( flags & VERBOSE )
			printVerbose(archive, "Create new archive", archive);

		if ( flags & MPQ_VERSION_1 )
			SOpenFlags |= MPQ_CREATE_ARCHIVE_V1;
		else if ( flags & MPQ_VERSION_2 )
			SOpenFlags |= MPQ_CREATE_ARCHIVE_V2;
		else if ( flags & MPQ_VERSION_3 )
			SOpenFlags |= MPQ_CREATE_ARCHIVE_V3;
		else
			SOpenFlags |= MPQ_CREATE_ARCHIVE_V4;

		if ( ! ( flags & NO_ATTRIBUTES ) )
			SOpenFlags |= MPQ_CREATE_ATTRIBUTES;

		if ( maxFileCount == 0 )
			for ( i = 0; files[i]; ++i )
				++maxFileCount;

		if ( maxFileCount < 4 )
			maxFileCount = 4;

		if ( ! SFileCreateArchive(archive, SOpenFlags, maxFileCount, &SArchive) ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "Cannot create archive", archive, GetLastError());

			SMPQ_FREE_EXPANDED_FILES(expandedFiles);
			return -1;

		}

	} else {

		unsigned int SOpenFlags = 0;

		if ( flags & NO_LISTFILE )
			SOpenFlags |= MPQ_OPEN_NO_LISTFILE;

		if ( flags & NO_ATTRIBUTES )
			SOpenFlags |= MPQ_OPEN_NO_ATTRIBUTES;

		if ( flags & MPQ_VERSION_1 )
			SOpenFlags |= MPQ_OPEN_FORCE_MPQ_V1;

		if ( ! SFileOpenArchive(archive, 0, SOpenFlags, &SArchive) ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "Cannot open archive", archive, GetLastError());

			SMPQ_FREE_EXPANDED_FILES(expandedFiles);
			return -1;

		}

		if ( maxFileCount == 0 ) {

			unsigned int fileCount;

			if ( ! SFileGetFileInfo(SArchive, SFileMpqNumberOfFiles, &fileCount, sizeof(fileCount), 0) )
				fileCount = 0;

			for ( i = 0; files[i]; ++i )
				++fileCount;

			if ( ! SFileGetFileInfo(SArchive, SFileMpqMaxFileCount, &maxFileCount, sizeof(maxFileCount), 0) )
				maxFileCount = 0;

			if ( maxFileCount < fileCount )
				maxFileCount = fileCount;
			else
				maxFileCount = 0;

		}

		if ( maxFileCount != 0 ) {

			if ( flags & VERBOSE )
				printVerbose(archive, "Change maximum file count", archive);

			if ( ! SFileSetMaxFileCount(SArchive, maxFileCount) ) {

				if ( ! ( flags & QUIET ) )
					printError(archive, "Cannot change maximum file count", archive, GetLastError());

				SFileCloseArchive(SArchive);
				SMPQ_FREE_EXPANDED_FILES(expandedFiles);
				return -1;

			}

		}

	}

	SFileSetLocale(locale);

	for ( i = 0; files[i]; ++i ) {

		struct stat st;
		FILE * file = NULL;
		const char * fileName = files[i];

#if !defined(WIN32) && !defined(_MSC_VER)
		if ( strncmp(fileName, "./", 2) == 0 )
			fileName += 2;
#endif

		size_t fileSize = 0;

		HANDLE SFile = NULL;
		char SFileName[1024];
		unsigned long long int SFileTime = 0;

		char buffer[0x10000];
		size_t bytes = 0;

		if ( strlen(fileName) + 1 > 1024 ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "File `%s' has too long path. Cannot create new file", SFileName, EPERM);

			continue;

		}

		toArchivePath(SFileName, fileName);

		if ( strlen(SFileName) == 16 && strncasecmp(SFileName, "File", 4) == 0 && SFileName[12] == '.' ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "File with mask `File????????.???\' is not allowed. Cannot create new file", SFileName, EPERM);

			continue;

		}

		if ( strcmp(SFileName, "(listfile)") == 0 || strcmp(SFileName, "(signature)") == 0 || strcmp(SFileName, "(attributes)") == 0 || strstr(SFileName, "(patch_metadata)") != NULL ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "Files `(listfile)' `(signature)' `(attributes)' `(patch_metadata)' are for internal usage. Cannot create new file", SFileName, EPERM);

			continue;

		}

		file = fopen(fileName, "rb");

		if ( ! file ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "Cannot open file", fileName, errno);

			continue;

		}

		fseek(file, 0, SEEK_END);
		fileSize = ftell(file);
		rewind(file);

		if ( flags & VERBOSE )
			printVerbose(archive, "Append file", SFileName);

		if ( stat(fileName, &st) == -1 ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "Cannot stat file", fileName, errno);

			fclose(file);
			continue;

		}

		toFileTime(&SFileTime, st.st_mtime);

		if ( ! SFileCreateFile(SArchive, SFileName, SFileTime, fileSize, locale, SFlags, &SFile) ) {

			if ( ! ( flags & QUIET ) )
				printError(archive, "Cannot create new file", SFileName, GetLastError());

			fclose(file);
			continue;

		}

		while ( 1 ) {

			bytes = fread(buffer, 1, sizeof(buffer), file);

			if ( ferror(file) ) {

				if ( ! ( flags & QUIET ) )
					printError(archive, "Cannot read file", fileName, errno);

				break;

			}

			if ( ! SFileWriteFile(SFile, buffer, bytes, SCompFlags) ) {

				if ( ! ( flags & QUIET ) )
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

	if ( flags & OVERWRITE )
		SFileCompactArchive(SArchive, NULL, 0);

	SFileCloseArchive(SArchive);

	SMPQ_FREE_EXPANDED_FILES(expandedFiles);

	return 0;

}
