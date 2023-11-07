/* unitmodel.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)unitmodel.c	2.1 (Berkeley) 4/9/83"
 *
 * This file contains routines that provide simple unit delay
 * models for Crystal.  Under the simple delay models, each
 * time a signal passes through a transistor gate, a delay
 * of one is added to it.  Resistances and capacitances
 * are ignored.
 */

#include "crystal.h"

extern char *PrintFET();

/* Tables to define model-dependent constants (we have none): */

char *(UnitNames[]) = NULL;
float *(UnitParms[]) = NULL;


UnitDelay(path, node, pup, pdown, pload)
DelayPath *path;		/* Describes the path through which the
				 * signal must pass.
				 */
Node *node;			/* The node being driven (it isn't part of
				 * path).
				 */
float *pup, *pdown, *pload;	/* These point to locations that will be
				 * filled in with rise and fall times.
				 */

/*---------------------------------------------------------
 *	This routine computes delays.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The words at pup, pdown, and pload are filled in with
 *	rise and fall times.  Pup will contain the latest time
 *	at which the driver in path can pull the last node to
 *	logic one (this is via a pass transistor).  Pdown will
 *	contain the latest time at which the driver can pull
 *	the last node to zero.  Pload is the latest time (after
 *	the driving node turns off) that the last node rises
 *	to one.
 *---------------------------------------------------------
 */

{
    /* Make sure that there is stuff in the path, then just
     * return one for the times. */

    if ((path->pa_piece1Size == 0) && (path->pa_piece2Size == 0))
    {
	printf("Crystal bug: nothing in delay path!\n");
	*pup = *pdown = *pload = -1.0;
	return;
    }

    *pup = *pdown = 1.0;
    if (path->pa_loadFET != NULL) *pload = 1.0;
    else *pload = -1.0;
}
