/*
    main.c - StormLib MPQ archiving utility
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common.h"

#define VERSION "1.0"

#define HELP \
	"Usage: %s [action] [options] [archive] [files]\n" \
	"\n" \
	"smpq - StormLib MPQ archiving utility, version " VERSION "\n" \
	"\n" \
	"Action:\n" \
	"     -c, --create                  Create new archive with file(s)\n" \
	"     -a, --append, --add           Append file(s) to archive\n" \
	"     -d, -r, --delete, --remove    Remove file(s) from archive\n" \
	"     -R, --rename                  Rename file in archive\n" \
	"     -l, --list                    List file(s) of archive\n" \
	"     -e, -x, --extract             Extract file(s) from archive\n" \
	"     -i, --info                    Show info about archive\n" \
	"\n" \
	"     -h, -u, --help, --usage       Show this help/usage information\n" \
	"     -V, --license, --version      Show version license information\n" \
	"\n" \
	"Options:\n" \
	"     -L, --listfile <file>         Additional external listfile (not used when appending file)\n" \
	"     -n, --no-system-listfiles     Do not load system listfile(s) (not used when appending file)\n" \
	"     -N, --no-archive-listfile     Do not use/create archive listfile (no file names will be read/stored)\n" \
	"     -A, --no-attributes           Do not allow using file attributes (time, checksum, hash)\n" \
	"     -M, --mpq-version-1           Use MPQ archive version 1 (default: use new version 2)\n" \
	"     -S, --sector-crc              Store/Check CRC for each sector, ignored if file has none compression or is single unit\n" \
	"     -q, --quiet                   Be quiet, do not show any output\n" \
	"     -f, -o, --force, --overwrite  Enable overwrite file(s)\n" \
	"     -v, --verbose                 Enable verbose output\n" \
	"     -O, --locale <id>             Set locale id (default: neutral=0)\n" \
	"          For all locale id see: http://msdn.microsoft.com/en-us/library/0h88fahh(v=VS.85).aspx\n" \
	"\n" \
	"Options for appending file(s) to archive:\n" \
	"     -H, --hash-table-size <size>  Set/Change hash table size in archive: between 4 and 524288; 0 - do not change when not set -c (default: 0 or 16)\n" \
	"     -E, --encrypt                 Store as encrypted\n" \
	"     -F, --fix-key                 Encryption key will be adjusted according to file size in the archive (need -E)\n" \
	"     -D, --deletion-marker         Set deletion marker\n" \
	"     -U, --single-unit             Add file as single unit, cannot be encrypted\n" \
	"     -C, --compression <method>    Compression method: (default LZMA)\n" \
	"          none                  None compression\n" \
	"          IMPLODE               Pkware Data Compression IMPLODE method - OBSOLATE (It was used only in Diablo I)\n" \
	"          HUFFMANN              Huffmann compression\n" \
	"          ADPCM_MONO            IMA ADPCM compression for 1-channel (mono) WAVE files - Lossy compression, only for WAVE files (Now it is not used)\n" \
	"          ADPCM_STEREO          IMA ADPCM compression for 2-channel (stereo) WAVE files - Lossy compression, only for WAVE files (Now it is not used)\n" \
	"          ZLIB                  ZLIB compression\n" \
	"          PKWARE                Pkware Data compression\n" \
	"          BZIP2                 BZIP2 compression\n" \
	"          SPARSE                SPARSE compression\n" \
	"          LZMA                  LZMA compression\n" \
	"          HUFFMANN+ADPCM_MONO   Together Huffmann and IMA ADPCM compression for 1-channel (mono) WAVE files\n" \
	"          HUFFMANN+ADPCM_STEREO Together Huffmann and IMA ADPCM compression for 2-channel (stereo) WAVE files\n" \
	"          ZLIB+PKWARE           Together ZLIB and Pkware Data compression\n" \
	"          BZIP2+PKWARE          Together BZIP2 and Pkware Data compression\n" \
	"          SPARSE+ZLIB           Together SPARSE and ZLIB compression\n" \
	"          SPARSE+PKWARE         Together SPARSE and Pkware Data compression\n" \
	"          SPARSE+BZIP2          Together SPARSE and BZIP2 compression\n" \
	"          SPARSE+ZLIB+PKWARE    Together SPARSE, ZLIB and Pkware Data compression\n" \
	"          SPARSE+BZIP2+PKWARE   Together SPARSE. BZIP2 and Pkware Data compression\n" \
	"          choose                Try all compression (expect ADPCM_MONO and ADPCM_STEREO) and choose the best for each file - This will spend a lot of time\n" \
	"\n" \
	"Options for deleting file(s) from archive:\n" \
	"     -I, --index                   Specify file(s) by index(es) (not by name)\n" \
	"\n" \
	"Options for extracting file(s) from archive:\n" \
	"     -I, --index                   Specify file(s) by index(es) (not by name)\n" \
	"     -p                            Open more (patched) archives, when file is in more archives, will be extracted from last\n" \
	"          Usage with more (patched) archives: %s -l|-x [options] [archive] -p [(patched)archives] -- [files]\n" \
	""

#define LICENSE \
	"smpq - StormLib MPQ archiving utility, version " VERSION "\n" \
	"Copyright (C) 2010  Pali Rohár <pali.rohar@gmail.com>\n" \
	"\n" \
	"This program is free software: you can redistribute it and/or modify\n" \
	"it under the terms of the GNU General Public License as published by\n" \
	"the Free Software Foundation, either version 3 of the License, or\n" \
	"(at your option) any later version.\n" \
	"\n" \
	"This program is distributed in the hope that it will be useful,\n" \
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
	"GNU General Public License for more details.\n" \
	"\n" \
	"You should have received a copy of the GNU General Public License\n" \
	"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"

char * app;
int skip = 0;
int action = 0;
int flags = 0;

void parse(char c) {

	switch ( c ) {

		case 'L':
			flags |= LISTFILE;
			skip = LISTFILE_ARG;
			break;

		case 'n':
			flags |= NO_SYSTEM_LF;
			break;

		case 'N':
			flags |= NO_LISTFILE;
			break;

		case 'A':
			flags |= NO_ATTRIBUTES;
			break;

		case 'M':
			flags |= MPQ_VERSION_1;
			break;

		case 'S':
			flags |= SECTOR_CRC;
			break;

		case 'q':
			flags |= QUIET;
			break;

		case 'f':
		case 'o':

			flags |= OVERWRITE;
			break;

		case 'v':

			flags |= VERBOSE;
			break;

		case 'O':
			flags |= LOCALE;
			skip = LOCALE_ARG;
			break;

		case 'H':
			flags |= HASH_SIZE;
			skip = HASH_SIZE_ARG;
			break;

		case 'E':
			flags |= ENCRYPT;
			break;

		case 'F':
			flags |= FIX_KEY;
			break;

		case 'D':
			flags |= DELETE_MARKER;
			break;

		case 'U':
			flags |= SINGLE_UNIT;
			break;

		case 'C':
			flags |= COMPRESSION;
			skip = COMPRESSION_ARG;

		case 'I':
			flags |= INDEX;
			break;

		case 'p':
			flags |= PATCHED;
			break;

		case 'c':
		case 'a':
		case 'd':
		case 'r':
		case 'R':
		case 'l':
		case 'e':
		case 'x':
		case 'i':

			if ( action != 0 ) {

				fprintf(stderr, "%s: Error: More then one action specified\n", app);
				exit(-1);

			}

			action = c;

			if ( action == 'l' ) {

				flags |= LIST;
				action = 'x';

			}

			if ( action == 'c' ) {

				flags |= CREATE;
				action = 'a';

			}

			if ( action == 'd' )
				action = 'r';

			if ( action == 'e' )
				action = 'x';

			break;

		case 'h':
		case 'u':

			printf(HELP, app, app);
			exit(0);

		case 'V':

			printf(LICENSE);
			exit(0);

		default:

			fprintf(stderr, "%s: Error: unknown option/action -%c specified\n", app, c);
			exit(-1);

	}


}

int main(int argc, char * argv[]) {

	app = argv[0];

	int i, j;
	int skipArg[10] = { 0 };

	for ( i = 1; i < argc; ++i ) {

		if ( skip ) {

			skipArg[skip] = i;
			skip = 0;
			continue;

		}

		if ( argv[i][0] != '-' )
			break;

		if ( argv[i][1] == '-' ) {

			if ( strcmp(argv[i], "--create") == 0 )
				parse('c');
			else if ( strcmp(argv[i], "--append") == 0 || strcmp(argv[i], "--add") == 0 )
				parse('a');
			else if ( strcmp(argv[i], "--delete") == 0 || strcmp(argv[i], "--remove") == 0 )
				parse('r');
			else if ( strcmp(argv[i], "--rename") == 0 )
				parse('R');
			else if ( strcmp(argv[i], "--list") == 0 )
				parse('l');
			else if ( strcmp(argv[i], "--extract") == 0 )
				parse('x');
			else if ( strcmp(argv[i], "--info") == 0 )
				parse('i');
			else if ( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "--usage") == 0 )
				parse('h');
			else if ( strcmp(argv[i], "--license") == 0 || strcmp(argv[i], "--version") == 0 )
				parse('V');
			else if ( strcmp(argv[i], "--listfile") == 0 )
				parse('L');
			else if ( strcmp(argv[i], "--no-system-listfiles") == 0 )
				parse('n');
			else if ( strcmp(argv[i], "--no-archive-listfile") == 0 )
				parse('N');
			else if ( strcmp(argv[i], "--no-attributes") == 0 )
				parse('A');
			else if ( strcmp(argv[i], "--mpq-version-1") == 0 )
				parse('M');
			else if ( strcmp(argv[i], "--sector-crc") == 0 )
				parse('S');
			else if ( strcmp(argv[i], "--quiet") == 0 )
				parse('q');
			else if ( strcmp(argv[i], "--force") == 0 || strcmp(argv[i], "--overwrite") == 0 )
				parse('f');
			else if ( strcmp(argv[i], "--verbose") == 0 )
				parse('v');
			else if ( strcmp(argv[i], "--locale") == 0 )
				parse('O');
			else if ( strcmp(argv[i], "--hash-table-size") == 0 )
				parse('H');
			else if ( strcmp(argv[i], "--encrypt") == 0 )
				parse('E');
			else if ( strcmp(argv[i], "--fix-key") == 0 )
				parse('F');
			else if ( strcmp(argv[i], "--delete-marker") == 0 )
				parse('D');
			else if ( strcmp(argv[i], "--single-unit") == 0 )
				parse('U');
			else if ( strcmp(argv[i], "--compression") == 0 )
				parse('C');
			else if ( strcmp(argv[i], "--index") == 0 )
				parse('I');
			else {
				fprintf(stderr, "%s Error: unknown option/action %s specified\n", app, argv[i]);
				return -1;
			}

			continue;

		}

		for ( j = 1; j < (int)strlen(argv[i]); ++j )
			parse(argv[i][j]);

	}

	if ( action == 0 ) {

		fprintf(stderr, "%s Error: No action specified\n", app);
		return -1;

	}

	if ( argc - i <= 0 ) {

		fprintf(stderr, "%s Error: No archive specified\n", app);
		return -1;

	}

	char * listfile = NULL;

	if ( flags & LISTFILE ) {

		if ( skipArg[LISTFILE_ARG] > argc-1 || skipArg[LISTFILE_ARG] == 0 ) {

			fprintf(stderr, "%s Error: No listfile specified\n", app);
			return -1;

		}

		listfile = argv[skipArg[LISTFILE_ARG]];

		struct stat st;

		if ( stat(listfile, &st) == -1 ) {

			fprintf(stderr, "%s Error: Cannot stat listfile `%s': %s\n", app, listfile, strerror(errno));
			return -1;

		}

		if ( S_ISDIR(st.st_mode) ) {

			fprintf(stderr, "%s Error: Cannot open listfile `%s': It is directory\n", app, listfile);
			return -1;

		}

	}

	int locale = 0;

	if ( flags & LOCALE ) {

		if ( skipArg[LOCALE_ARG] > argc-1 || skipArg[LOCALE_ARG] == 0 ) {

			fprintf(stderr, "%s Error: No locale specified\n", app);
			return -1;

		}

		locale = atoi(argv[skipArg[LOCALE_ARG]]);

	}

	int hashTableSize = 16;
	
	if ( flags & HASH_SIZE ) {

		if ( skipArg[HASH_SIZE_ARG] > argc-1 || skipArg[HASH_SIZE_ARG] == 0 ) {

			fprintf(stderr, "%s Error: No hash table size specified\n", app);
			return -1;

		}

		hashTableSize = atoi(argv[skipArg[HASH_SIZE_ARG]]);

		if ( hashTableSize < 4 || hashTableSize > 524288 ) {

			fprintf(stderr, "%s Error: Unsupported hash table size %d\n", app, hashTableSize);
			return -1;

		}

	}

	char * compression = (char *)"none";

	if ( flags & COMPRESSION ) {

		if ( skipArg[COMPRESSION_ARG] > argc-1 || skipArg[COMPRESSION_ARG] == 0 ) {

			fprintf(stderr, "%s Error: No MPQ compression method specified\n", app);
			return -1;

		}

		compression = argv[skipArg[COMPRESSION_ARG]];

	}

	char * archive = argv[i++];

	if ( action == 'i' ) {

		if ( i < argc-1 ) {

			fprintf(stderr, "%s Error: Info need only one archive\n", app);
			return -1;

		}

		return info(archive);

	}

	if ( action == 'R' ) {

		if ( i+1 < argc-1 ) {

			fprintf(stderr, "%s Error: Cannot rename more then one file\n", app);
			return -1;

		}

		return rename(archive, argv[i], argv[i+1], flags, listfile, locale);

	}

	int parchivesc = argc - i;
	char * parchives[parchivesc + 2];

	if ( action == 'x' && i < argc && strcmp(argv[i], "-p") ) {

		while ( i < argc ) {

			if ( strcmp(argv[i], "--") )
				break;

			parchives[parchivesc - argc + i] = argv[i];
			++i;

		}

	}

	parchives[parchivesc - argc + i] = NULL;

	int filesc = argc - i;
	char * files[filesc + 2];

	for ( ; i < argc; ++i )
		files[filesc - argc + i] = argv[i];

	if ( action == 'x' ) {

		if ( filesc == 0 ) {

			filesc = 1;
			files[0] = (char *)"*";

		}

	} else {

		if ( filesc == 0 ) {

			fprintf(stderr, "%s Error: No file(s) specified\n", app);
			return -1;

		}

	}

	files[filesc] = NULL;

	switch ( action ) {

		case 'a':
			return append(archive, (const char * const *)files, flags, locale, hashTableSize, compression);

		case 'x':
			return extract(archive, (const char * const *)files, flags, listfile, locale, (const char * const *)parchives);

		case 'r':
			return remove(archive, (const char * const *)files, flags, listfile, locale);

	}

	return 0;

}
