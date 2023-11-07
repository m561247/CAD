/* runstats.c -
 *
 * Copyright (C) 1982 by John K. Ousterhout
 * sccsid "@(#)runstats.c	2.2 (Berkeley) 9/8/83"
 *
 * This file provides procedures that return strings describing
 * the amount of user and system time used so far, along with the
 * size of the data area.
 */

#include "port.h"
#include <sys/times.h>

/* Library imports: */

extern end;

/* The following variables keep track of the time used as of
 * the last call to one of the procedures in this module.
 */

int lastSysTime = 0;
int lastUserTime = 0;


char *
RunStats()

/*---------------------------------------------------------
 *	This procedure collects information about the
 *	process.
 *
 *	Results:
 *	The return value is a string of the form
 *	"[mins:secs mins:secs size]" where the first time is
 *	the amount of user-space CPU time this process has
 *	used so far, the second time is the amount of system
 *	time used so far, and size is the size of the heap area.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    struct tms buffer;
    static char string[100];
    int umins, size, smins;
    float usecs, ssecs;

    times(&buffer);
    lastUserTime = buffer.tms_utime;
    lastSysTime = buffer.tms_stime;
    umins = buffer.tms_utime;
    usecs = umins % 3600;
    usecs /= 60;
    umins /= 3600;
    smins = buffer.tms_stime;
    ssecs = smins % 3600;
    ssecs /= 60;
    smins /= 3600;
    size = (((int) sbrk(0) - (int) &end) + 512)/1024;
    (void) sprintf(string, "[%d:%04.1fu %d:%04.1fs %dk]",
	umins, usecs, smins, ssecs, size);
    return string;
}


char *
RunStatsSince()

/*---------------------------------------------------------
 *	This procedure returns information about what's
 *	happened since the last call.
 *
 *	Results:
 *	Just the same as for RunStats, except that the times
 *	refer to the time used since the last call to this
 *	procedure or RunStats.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    struct tms buffer;
    static char string[100];
    int umins, size, smins;
    float usecs, ssecs;

    times(&buffer);
    umins = buffer.tms_utime - lastUserTime;
    smins = buffer.tms_stime - lastSysTime;
    lastUserTime = buffer.tms_utime;
    lastSysTime = buffer.tms_stime;
    usecs = umins % 3600;
    usecs /= 60;
    umins /= 3600;
    ssecs = smins % 3600;
    ssecs /= 60;
    smins /= 3600;
    size = (((int) sbrk(0) - (int) &end) + 512)/1024;
    (void) sprintf(string, "[%d:%04.1fu %d:%04.1fs %dk]",
	umins, usecs, smins, ssecs, size);
    return string;
}
