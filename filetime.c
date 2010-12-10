/*
    filetime.c - StormLib MPQ archiving utility
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


/*
    This source code is from:
    http://wiki.devklog.net/index.php?title=The_MoPaQ_Archive_Format#Conversion_of_FILETIME_And_time_t
*/

#include "common.h"

/* Number of 100 ns units between 01/01/1601 and 01/01/1970 */
#define EPOCH_OFFSET 116444736000000000ULL

int GetTimeFromFileTime(const unsigned long long int fileTime, time_t * time)
{
	/* The FILETIME represents a 64-bit integer: the number of 100 ns units since January 1, 1601 */
	unsigned long long nTime = fileTime;

	if (nTime < EPOCH_OFFSET)
		return 0;

	/* Convert the time base from 01/01/1601 to 01/01/1970 */
	nTime -= EPOCH_OFFSET;

	/* Convert 100 ns to sec */
	nTime /= 10000000ULL;

	time_t timeT = (time_t)nTime;

	/* Test for overflow (FILETIME is 64 bits, time_t is 32 bits) */
	if ((nTime - (unsigned long long)timeT) > 0)
		return 0;

	*time = timeT;

	return 1;
}

void GetFileTimeFromTime(const time_t time, unsigned long long int * fileTime)
{
	unsigned long long nTime = (unsigned long long)time;

	nTime *= 10000000ULL;
	nTime += EPOCH_OFFSET;

	*fileTime = nTime;
}
