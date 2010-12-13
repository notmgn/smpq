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
	"     -l, --list                    List file(s) of archive\n" \
	"     -e, -x, --extract             Extract file(s) from archive\n" \
	"     -i, -I, --info                Show info about archive\n" \
	"\n" \
	"     -h, -u, --help, --usage       Show this help/usage information\n" \
	"     -V, --license                 Show version license information\n" \
	"\n" \
	"Options:\n" \
	"     -L, --listfile                Specify additional external listfile\n" \
	"     -n, --no-system-listfiles     Do not load system listfiles\n" \
	"     -f, -o, --force, --overwrite  Enable overwrite file(s)\n" \
	"     -v, --verbose                 Enable verbose output\n"

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
			flags |= NO_SYSTEM;
			break;

		case 'f':
		case 'o':

			flags |= OVERWRITE;
			break;

		case 'v':

			flags |= VERBOSE;
			break;

		case 'a':
		case 'c':
		case 'd':
		case 'e':
		case 'i':
		case 'I':
		case 'l':
		case 'r':
		case 'x':

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

			if ( action == 'I' )
				action = 'i';

			break;

		case 'h':
		case 'u':

			printf(HELP, app);
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
	int skipArg[10];

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
			else if ( strcmp(argv[i], "--delete") == 0 || strcmp(argv[i], "--remove") == 0 )
				parse('r');
			else if ( strcmp(argv[i], "--append") == 0 || strcmp(argv[i], "--add") == 0 )
				parse('a');
			else if ( strcmp(argv[i], "--list") == 0 )
				parse('l');
			else if ( strcmp(argv[i], "--extract") == 0 )
				parse('x');
			else if ( strcmp(argv[i], "--info") == 0 )
				parse('i');
			else if ( strcmp(argv[i], "--force") == 0 || strcmp(argv[i], "--overwrite") == 0 )
				parse('f');
			else if ( strcmp(argv[i], "--verbose") == 0 )
				parse('v');
			else if ( strcmp(argv[i], "--listfile") == 0 )
				parse('L');
			else if ( strcmp(argv[i], "--no-system-listfiles") == 0 )
				parse('n');
			else if ( strcmp(argv[i], "--license") == 0 || strcmp(argv[i], "--version") == 0 )
				parse('V');
			else if ( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "--usage") == 0 )
				parse('h');
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

	char * listfile = NULL;

	if ( flags & LISTFILE ) {

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

	if ( argc - i <= 0 ) {

		fprintf(stderr, "%s Error: No archive specified\n", app);
		return -1;

	}

	char * archive = argv[i++];

	if ( action == 'i' )
		return info(archive);

	int filesc = argc - i;
	char * files[filesc + 2];

	for ( ; i < argc; ++i ) {

		files[filesc - argc + i] = argv[i];

		if ( filesc == argc - i )
			continue;

	}

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
			return append(archive, (const char * const *)files, flags, listfile);

		case 'x':
			return extract(archive, (const char * const *)files, flags, listfile);

		case 'r':
			return remove(archive, (const char * const *)files, flags, listfile);

	}

	return 0;

}
