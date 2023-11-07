/* flow.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)flow.c	2.1 (Berkeley) 4/28/83"
 *
 * This file contains routines that watch pass transistor flow
 * for Crystal.  They handle the bidirectional flow attributes
 * (those other than "In" or "Out") to keep Crystal from getting
 * lost in complex pass transistor structures.
 */

#include "crystal.h"
#include "hash.h"

/* Library imports: */

/* Imports from other Crystal modules: */

extern char *NextField(), *ExpandNext(), *PrintFET();
extern HashTable FlowTable;


int
FlowLock(fet, input)
register FET *fet;		/* Fet being passed though. */
Node *input;			/* Node from which info flows INTO fet.*/

/*---------------------------------------------------------
 *	AttFlowLock sees if it is OK to pass through a transistor
 *	in a given direction, considering other pass transistors
 *	that are already pending.
 *
 *	Results:
 *	The return value is TRUE if it is OK to pass through,
 *	FALSE otherwise.
 *
 *	Side Effects:
 *	If TRUE is returned, then info may have been changed
 *	in the attribute.  FlowUnlock must be called to
 *	reset the bits.  If FALSE is returned, then there is
 *	no need to call FlowUnlock.
 *
 *	More Info:
 *	This procedure and FlowUnlock are the ones that use
 *	direction information in transistors.  This procedure
 *	is called when starting through a transistor, and
 *	FlowUnlock is called when all paths through the transistor
 *	have been finished with.  Source and drain attributes
 *	indicate directions of information flow.  Once information
 *	has flowed into a transistor with a source or drain attribute
 *	it cannot flow into any other transistor in the opposite
 *	direction until FlowUnlock has been called.  This prevents
 *	us from chasing illegitimate paths through muxes and shift
 *	arrays.
 *---------------------------------------------------------
 */

{
    int enteredBit;		/* Selects whether info enters at source
				 * or drain. */
    int leftBit;		/* Selects where info leaves transistor. */
    register FPointer *fp;
    register Flow *fl;

    /* Figure out which side we're coming from, then skim through the
     * attribute list to make sure that there are no direction violations.
     */

    if (input == fet->f_source)
    {
	enteredBit = FP_SOURCE;
	leftBit = FP_DRAIN;
    }
    else if (input == fet->f_drain)
    {
	enteredBit = FP_DRAIN;
	leftBit = FP_SOURCE;
    }
    else return TRUE;

    /* First make sure that there's no conflict with existing flow locks. */

    for (fp = fet->f_fp; fp != NULL; fp = fp->fp_next)
    {
	fl = fp->fp_flow;
	if (fl->fl_flags & FL_IGNORE) continue;
	if (fl->fl_flags & FL_OFF) return FALSE;
	if (fp->fp_flags & enteredBit)
	    if ((fl->fl_flags & FL_OUTONLY) || (fl->fl_left != NULL))
		return FALSE;
	if (fp->fp_flags & leftBit)
	    if ((fl->fl_flags & FL_INONLY) || (fl->fl_entered != NULL))
		return FALSE;
    }

    /* Now go through again and set flow locks so we can't go the
     * wrong way later.
     */

    for (fp = fet->f_fp; fp != NULL; fp = fp->fp_next)
    {
	fl = fp->fp_flow;
	if ((fp->fp_flags & enteredBit) && (fl->fl_entered == NULL))
	    fl->fl_entered = input;
	if ((fp->fp_flags & leftBit) && (fl->fl_left == NULL))
	    fl->fl_left = input;
    }
    return TRUE;
}


FlowUnlock(fet, input)
FET *fet;
Node *input;			/* Node from which info flowed into fet. */

/*---------------------------------------------------------
 *	This procedure just clears the markings left by
 *	FlowLock.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	All the markings left by the corresponding call to
 *	FlowLock are cleared.
 *---------------------------------------------------------
 */

{
    register FPointer *fp;
    register Flow *fl;

    for (fp = fet->f_fp; fp != NULL; fp = fp->fp_next)
    {
	fl = fp->fp_flow;
	if (fl->fl_entered == input)
	    fl->fl_entered = (Node *) NULL;
	if (fl->fl_left == input)
	    fl->fl_left = (Node *) NULL;
    }
}


FlowCmd(string)
char *string;			/* Describes flow information. */

/*---------------------------------------------------------
 *	This routine marks attributes to restrict flow or
 *	permit arbitrary flow.  The string contains one of
 *	the keywords "in", "out", "off", or "ignore", followed
 *	by one or more attribute names.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Each of the named attributes is marked according to
 *	the keyword:  "in" means that signals may only flow
 *	into the transistor from the side containing the
 *	attribute;  "out" means that signals may only flow
 *	out at the side of the attribute (i.e. the transistor
 *	can only drive gates on the attribute side);  "off" means
 *	do not let anything flow through the transistor at all;
 *	"ignore" means completely ignore this attribute for flow
 *	control (anything can flow anywhere); and "normal" means
 *	use the standard flow interpretation.
 *---------------------------------------------------------
 */

{
    static char *(keywords[]) = {"ignore", "in", "normal", "off", "out", NULL};
    Flow *fl;
    char *p;
    int index, flags;

    /* Scan off the keyword and look it up. */

    p = NextField(&string);
    if (p == NULL)
    {
	printf("No keyword given:  try ignore, off, in, out, or normal.\n");
	return;
    }
    index = Lookup(p, keywords);
    if (index < 0)
    {
	printf("Bad keyword:  try ignore, off, in, out, or normal.\n");
	return;
    }
    switch (index)
    {
	case 0: flags = FL_IGNORE; break;
	case 1: flags = FL_INONLY; break;
	case 2: flags = 0; break;
	case 3: flags = FL_OFF; break;
	case 4: flags = FL_OUTONLY; break;
    }

    /* Go through the attributes and mark them. */

    while ((p = NextField(&string)) != NULL)
    {
	ExpandInit(p);
	while ((fl= (Flow *) ExpandNext(&FlowTable))
	    != (Flow *) NULL)
	{
	    fl->fl_flags &= ~(FL_INONLY | FL_OUTONLY | FL_IGNORE | FL_OFF);
	    fl->fl_flags |= flags;
	}
    }
}
