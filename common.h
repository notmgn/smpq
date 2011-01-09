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

/*********
 * Flags *
 *********/

#define CREATE		1 << 0
#define LIST		1 << 0

/* Options */
#define LISTFILE	1 << 1
#define LOCALE		1 << 2
#define NO_SYSTEM_LF	1 << 3
#define NO_LISTFILE	1 << 4
#define NO_ATTRIBUTES	1 << 5
#define MPQ_VERSION	1 << 6
#define MPQ_VERSION_1	1 << 7
#define MPQ_VERSION_2	1 << 8
#define MPQ_VERSION_3	1 << 9
#define MPQ_VERSION_4	1 << 10
#define SECTOR_CRC	1 << 11
#define QUIET		1 << 12
#define OVERWRITE	1 << 13
#define VERBOSE		1 << 14
#define INDEX		1 << 15
#define PATCHED		1 << 16

/* Append file */
#define HASH_SIZE	1 << 20
#define ENCRYPT		1 << 21
#define FIX_KEY		1 << 22
#define DELETE_MARKER	1 << 23
#define SINGLE_UNIT	1 << 24
#define COMPRESSION	1 << 25
#define ENCRYPTED	1 << 26

#define LOCALE_ARG	1
#define MPQ_VERSION_ARG	2
#define LISTFILE_ARG	3
#define HASH_SIZE_ARG	4
#define COMPRESSION_ARG	5

/*************
 * Variables *
 *************/

/* StormLib copyright stored in StormLib library */
extern char StormLibCopyright[];

/* Application name */
extern char * app;

/***********************************************
 * Functions for manipulating with MPQ archive *
 ***********************************************/

/**
 * Create new archive and/or append file(s) to archive
 *
 * Internaly this function calls SFileCreateArchive or SFileOpenArchive for getting access to archive.
 * For each file check if it has correct name and calls SFileCreateFile. Next it uses SFileWriteFile for writing file data to archive.
 */
int append(const char * archive, const char * const files[], int flags, int locale, int hashTableSize, const char * compression);

/**
 * Extract or print list files from archive
 *
 * Internaly this function open archive throw SFileOpenArchive and append all patched archives to memmory by calling SFileOpenPatchArchive
 * For each file mask it calls SFileFindFirstFile. Is return first (and then continue searching) valid file with mask and then it try
 * extract using functions SFileOpenFileEx and SFileReadFile. MPQ archives does not have stored real filenames (only hashes) so original
 * file names must be stored in other list text file (MPQ archives does not support directory structures, so for this is used standard
 * windows separator = char backslash '\'). So SFileFindFirstFile only tries check if file witch given name from list is correct
 * for stored hashes. When we use more patched archives it is normal that file with same name is in more patched archives (so search
 * function return one file name more times). To prevent extracting one file more times, smpq remember extracted files. For this is used
 * dictionary struct trie, which spend linear time (of path) for remeber file and linear time too for check if file is in this structre
 * (if file was extracted). When is needed to extract file with long path and subdirs does not exist, smpq will use function mkpath,
 * which recursive create needed directories (find separator '/').
 */
int extract(const char * archive, const char * const files[], int flags, const char * listfile, int locale, const char * const parchives[]);

/**
 * Show info about archive
 *
 * Internaly this function only calls StormLib GetInfo function.
 */
int info(const char * archive);

/**
 * Remove file(s) from archive
 *
 * Internaly this function only calls for each specified file SFileRemoveFile if exist.
 */
int remove(const char * archive, const char * const files[], int flags, const char * listfile, int locale);

/**
 * Rename file in archive
 *
 * Internaly this function only calls for each specified file SFileRenameFile if exist.
 */
int rename(const char * archive, const char * oldName, const char * newName, int flags, const char * listfile, int locale);

/**
 * Load system listfiles for archive to memory
 *
 * Internaly this function looks for all *.txt files in system StormLib directory and tries load all text list files to memory
 * by calling SFileAddListFile. On Windows is this directory same with directory where are smpq executable.
 */
void systemListfiles(HANDLE SArchive, const char * archive, int flags);

/************************
 * Functions for output *
 ************************/

/* Print formatted error message */
void printError(const char * archive, const char * file, const char * message, int errnum);

/* Print verbose message */
void printVerbose(const char * archive, const char * message, const char * file);

/* Print normal message */
#define printMessage(message, ...) do { printf(message "\n", ##__VA_ARGS__); fflush(stdout); } while (0)

/*************************************
 * Functions for FILETIME conversion *
 *************************************/

/**
 * MPQ archives stores modification time of files in FILETIME format.
 * These function convert FILETIME to/from Unix timestamp which is default format
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

/**********************************************
 * Functions for path conversation in archive *
 **********************************************/

/**
 * MPQ archives does not support directory structures. Instead it store full path to file in file name
 * As default MPQ archives are designed for Windows, so dir separator is backslash character.
 * These functions convert Windows file path to/from Unix
 */

/* Replace all chars '/' in path to '\\' */
static inline void toArchivePath(char * to, const char * from) {

	int i = -1;

	while ( from[++i] ) {

		if ( from[i] == '/' )
			to[i] = '\\';
		else
			to[i] = from[i];

	}

	to[i] = 0;

}

/* Replace all chars '\\' in path to '/' on other OS than Windows */
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
