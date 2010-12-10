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

#include <sys/types.h>

typedef void * HANDLE;

#endif

#include <time.h>

#define append _smpq_append
#define extract _smpq_extract
#define info _smpq_info
#define remove _smpq_remove

/**
 * Flags
 */

#define CREATE		1
#define LIST		1

#define OVERWRITE	1 << 1
#define VERBOSE		1 << 2
#define LISTFILE	1 << 3
#define NO_SYSTEM	1 << 4

/**
 * Variables
 */

/* Application name */
extern char * app;

/**
 * Functions for manipulating with MPQ archive
 */

/* Create new archive and/or append files to archive */
int append(const char * archive, const char * const files[], int flags, const char * listfile);

/* Extract or print list files from archive */
int extract(const char * archive, const char * const files[], int flags, const char * listfile);

/* Show info about archive */
int info(const char * archive);

/* Remove file(s) fro archive */
int remove(const char * archive, const char * const files[], int flags, const char * listfile);

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
 * Function for disk operations
 */

/* Recursive create directory */
int mkpath(const char * s, mode_t mode);

/* Load system listfiles to memory */
void systemListfiles(HANDLE SArchive, const char * archive, int flags);

/**
 * Functions for FILETIME conversion
 */

/* Convert FILETIME to time_t */
int GetTimeFromFileTime(const unsigned long long int fileTime, time_t * time);

/* Convert time_t to FILETIME */
void GetFileTimeFromTime(const time_t time, unsigned long long int * fileTime);

/**
 * Path conversation in archive
 */

/* Replace all chars '/' in path to '\\' */
static inline void convertPathToArchive(char * out, const char * in) {

#if defined(WIN32) || defined(_MSC_VER)

	strcpy(out, in);
	return;

#endif

	int i = -1;

	while ( in[++i] ) {

		if ( in[i] == '/' )
			out[i] = '\\';
		else
			out[i] = in[i];

	}

	out[i] = 0;

}

/* Replace all chars '\\' in path to '/' */
static inline void convertPathFromArchive(char * out, const char * in) {

#if defined(WIN32) || defined(_MSC_VER)

	strcpy(out, in);
	return;

#endif

	int i = -1;

	while ( in[++i] ) {

		if ( in[i] == '\\' )
			out[i] = '/';
		else
			out[i] = in[i];

	}

	out[i] = 0;

}
