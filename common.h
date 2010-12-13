/*
    common.h - StormLib MPQ archiving utility
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

#ifdef __STRICT_ANSI__
#define inline __inline__
#endif

#if defined(WIN32) || defined(_MSC_VER)
#include <windows.h>
#else
typedef void * HANDLE;
#endif

#define append _smpq_append
#define extract _smpq_extract
#define info _smpq_info
#define remove _smpq_remove
#define rename _smpq_rename

/**
 * Flags
 */

#define CREATE		1 << 0
#define LIST		1 << 0

/* Options */
#define LISTFILE	1 << 1
#define LOCALE		1 << 2
#define NO_SYSTEM_LF	1 << 3
#define NO_LISTFILE	1 << 4
#define NO_ATTRIBUTES	1 << 5
#define MPQ_VERSION_1	1 << 6
#define SECTOR_CRC	1 << 7
#define QUIET		1 << 8
#define OVERWRITE	1 << 9
#define VERBOSE		1 << 10
#define INDEX		1 << 11
#define PATCHED		1 << 12

/* Append file */
#define HASH_SIZE	1 << 19
#define ENCRYPT		1 << 20
#define DELETION_MARKER	1 << 21
#define SINGLE_UNIT	1 << 23
#define COMPRESSION	1 << 24

#define LOCALE_ARG	1
#define LISTFILE_ARG	2
#define HASH_SIZE_ARG	3
#define COMPRESSION_ARG	4

/**
 * Variables
 */

/* Application name */
extern char * app;

/**
 * Functions for manipulating with MPQ archive
 */

/* Create new archive and/or append files to archive */
int append(const char * archive, const char * const files[], int flags, const char * listfile, int locale, int hashTableSize, const char * compression);

/* Extract or print list files from archive */
int extract(const char * archive, const char * const files[], int flags, const char * listfile, int locale, const char * const parchives[]);

/* Show info about archive */
int info(const char * archive);

/* Remove file(s) from archive */
int remove(const char * archive, const char * const files[], int flags, const char * listfile, int locale);

/* Rename file in archive */
int rename(const char * archive, const char * oldName, const char * newName, int flags, const char * listfile, int locale);

/* Load system listfiles for archive to memory */
void systemListfiles(HANDLE SArchive, const char * archive, int flags);

/**
 * Functions for output
 */

/* Print formatted error message */
void printError(const char * archive, const char * file, const char * message, int errnum);

/* Print verbose message */
void printVerbose(const char * archive, const char * message, const char * file);

/* Print normal message */
#define printMessage(message, ...) do { printf(message "\n", ##__VA_ARGS__); fflush(stdout); } while (0)

/**
 * Functions for FILETIME conversion
 */

#define OFFSET 116444736000000000ULL /* Number of 100 ns units between 01/01/1601 and 01/01/1970 */
#define NSEC 10000000ULL /* Convert 100 ns to sec */

/* Convert time_t to FILETIME */
static inline void toFileTime(unsigned long long int * to, time_t from) {

        if ( from == 0 ) 
                *to = 0;
        else
                *to = from * NSEC + OFFSET;

}

/* Convert FILETIME to time_t */
static inline int fromFileTime(time_t * to, unsigned long long int from) {

        if ( from < OFFSET )
                return 0;

        if ( ( from - OFFSET ) / NSEC > ( 1ULL << sizeof(time_t) * 8 ) ) 
                return 1;

        *to = ( from - OFFSET ) / NSEC;

        return 1;

}

#undef OFFSET
#undef NSEC

/**
 * Functions for path conversation in archive
 */

/* Replace all chars '/' in path to '\\' */
static inline void toArchivePath(char * to, const char * from) {

#if defined(WIN32) || defined(_MSC_VER)

	strcpy(to, from);
	return;

#endif

	int i = -1;

	while ( from[++i] ) {

		if ( from[i] == '/' )
			to[i] = '\\';
		else
			to[i] = from[i];

	}

	to[i] = 0;

}

/* Replace all chars '\\' in path to '/' */
static inline void fromArchivePath(char * to, const char * from) {

#if defined(WIN32) || defined(_MSC_VER)

	strcpy(to, from);
	return;

#endif

	int i = -1;

	while ( from[++i] ) {

		if ( from[i] == '\\' )
			to[i] = '/';
		else
			to[i] = from[i];

	}

	to[i] = 0;

}
