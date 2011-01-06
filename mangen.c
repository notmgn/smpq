/*
    mangen.c - StormLib MPQ archiving utility
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

#define main _main
#include "main.c"
#undef main

int append(const char * archive, const char * const files[], int flags, int locale, int hashTableSize, const char * compression) { (void)archive; (void)files; (void)flags; (void)locale; (void)hashTableSize; (void)compression; return 0; }
int extract(const char * archive, const char * const files[], int flags, const char * listfile, int locale, const char * const parchives[]) { (void)archive; (void)files; (void)flags; (void)listfile; (void)locale; (void)parchives; return 0; }
int info(const char * archive) { (void)archive; return 0; }
int remove(const char * archive, const char * const files[], int flags, const char * listfile, int locale) { (void)archive; (void)files; (void)flags; (void)listfile; (void)locale; return 0; }
int rename(const char * archive, const char * oldName, const char * newName, int flags, const char * listfile, int locale) { (void)archive; (void)oldName; (void)newName; (void)newName; (void)flags; (void)listfile; (void)locale; return 0; }

#include <stdio.h>
#include <string.h>

#define prints(s, e) do { char * _c; for ( _c = s; _c < e; ++_c ) putchar(*_c); } while (0)

int main() {

	printf(".TH SMPQ 1 \"Dec 2010\" \"smpq - StormLib MPQ archiving utility, version " VERSION "\"\n");

	char * start = (char *)HELP;
	char * end = start;
	char * len = start + strlen(HELP) + 1;

	while ( start < len && ( end = strchr(start, '\n') ) != NULL ) {

		if ( strncmp(start, "Usage:", 6) == 0 ) {

			char * next = strstr(start, "%s");
			if ( next == NULL || next > end )
				next = start;
			else
				next += 2;

			printf("\n.SH SYNOPSIS\n.B smpq\n.I ");
			prints(next, end);
			putchar('\n');

		} else if ( strncmp(start, "smpq", 4) == 0 ) {

			printf("\n.SH NAME\n");
			prints(start, end);
			putchar('\n');

		} else if ( *(end - 1) == ':' ) {

			printf("\n.SH ");
			prints(start, end - 1);
			putchar('\n');

		} else if ( strncmp(start, "       ", 7) == 0 ) {

			char * next = start;
			while ( *(++next) == ' ' && next < end );

			putchar('\n');
			prints(next, end);
			putchar('\n');

		} else if ( *start == ' ' ) {

			char * next = start;
			while ( *(++next) == ' ' && next < end );

			char * next2 = strstr(next, "  ");
			if ( next2 == NULL || next2 > end )
				next2 = next;

			char * next3 = next2;
			while ( *(++next3) == ' ' && next3 < end );

			printf(".TP\n.B ");
			prints(next, next2);
			putchar('\n');
			prints(next3, end);
			putchar('\n');

		} else {

			prints(start, end);
			putchar('\n');

		}

		start = end + 2;

	}

	return 0;

}
