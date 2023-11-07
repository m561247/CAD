/* check.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)check.c	2.11 (Berkeley) 9/11/83"
 *
 * This file contains procedures to perform simple electrical
 * checks on a circuit.  It's part of the Crystal timing
 * analyzer.
 */

#include "crystal.h"
#include "hash.h"

/* Library imports: */

/* Imports from other Crystal modules: */

extern char *PrintFET(), *PrintNode(), *NextField();
extern Node *VddNode, *GroundNode, *FirstNode;

/* Limit of how many messages of any one type to print. */

int CheckMsgLimit = 100;

/* Limits for valid transistor ratios. */

float NormalLow = 3.8;
float NormalHigh = 4.2;
float PassLow = 7.8;
float PassHigh = 8.2;

/* Info used to suppress printing of many identical ratio errors. */

int RatioLimit = 20;
HashTable ratioTable;
int ratioDups;
int totalRatioErrs;
int RatioTotalLimit = 1000;


Check()

/*---------------------------------------------------------
 *	This procedure performs some simple static electrical
 *	checks.  It shouldn't be invoked until after pass
 *	transistor flow has been determined.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The circuit isn't changed, but messages are output for
 *	the following errors:
 *	1. Nodes not attached to any transistors.
 *	2. Nodes that can't be driven.
 *	3. Nodes that don't drive anything.
 *	4. Transistors that can't pass information.
 *	5. Bidirectional pass transistors with no flow attributes.
 *	6. Transistors between Vdd and Ground.
 *---------------------------------------------------------
 */

{
    register Node *n;
    register Pointer *p;
    register FET *f;
    int drives, driven, msg1, msg2, msg3, msg4, msg5, msg6, skipped;

    msg1 = msg2 = msg3 = msg4 = msg5 = msg6 = skipped = 0;

    /* Search through all the nodes. */

    for (n = FirstNode; n != NULL; n = n->n_next)
    {
	/* If node has no transistors attached, error. */

	if (n->n_pointer == NULL)
	{
	    msg1 += 1;
	    if (msg1 <= CheckMsgLimit)
		printf("Node %s doesn't connect to any transistors.\n",
		    PrintNode(n));
	    else skipped += 1;
	    if (msg1 == CheckMsgLimit)
		printf("    No more of these messages will be printed.\n");
	    continue;
	}

	/* Go through all the transistors attached to the node.
	 * See if the node is driven and if it drives anything,
	 * and also check out the transistors on the way (only
	 * check a transistor when its gate is seen, to avoid
	 * duplication).
	 */
	
	drives = FALSE;
	driven = FALSE;
	for (p = n->n_pointer;  p != NULL;  p = p->p_next)
	{
	    f = p->p_fet;
	    if (f->f_source == n)
	    {
		if (f->f_flags & FET_FLOWFROMDRAIN) driven = TRUE;
		if (f->f_flags & FET_FLOWFROMSOURCE) drives = TRUE;
	    }
	    if (f->f_drain == n)
	    {
		if (f->f_flags & FET_FLOWFROMSOURCE) driven = TRUE;
		if (f->f_flags & FET_FLOWFROMDRAIN) drives = TRUE;
	    }
	    if (f->f_gate == n)
	    {
		drives = TRUE;

		/* If transistor can't flow at all, error. */

		if (!(f->f_flags & (FET_FLOWFROMSOURCE|FET_FLOWFROMDRAIN)))
		{
		    msg4 += 1;
		    if (msg4 <= CheckMsgLimit)
		    {
			printf("No flow through FET: %s\n", PrintFET(f));
			if (msg4 == 1)
			{
			    printf("    Maybe you haven't marked all");
			    printf(" the inputs and outputs?\n");
			}
		    }
		    else skipped += 1;
		    if (msg4 == CheckMsgLimit)
		    {
			printf("    No more of these messages");
			printf(" will be printed.\n");
		    }
		}

		/* If transistor is bidirectional but has no flow
		 * indicator, issue a warning.
		 */
		
		if ((f->f_flags & FET_FLOWFROMSOURCE)
		    && (f->f_flags & FET_FLOWFROMDRAIN)
		    && (f->f_fp == NULL))
		{
		    msg5 += 1;
		    if (msg5 <= CheckMsgLimit)
		    {
			printf("Bidirectional FET: %s\n", PrintFET(f));
			if (msg5 == 1)
			{
			    printf("    Maybe you should label flow?\n");
			    msg5 = TRUE;
			}
		    }
		    else skipped += 1;
		    if (msg5 == CheckMsgLimit)
		    {
			printf("    No more of these messages");
			printf(" will be printed.\n");
		    }
		}

		/* If transistor connects Vdd and Ground, error. */

		if (((f->f_drain == VddNode) && (f->f_source == GroundNode))
		    || ((f->f_source == VddNode) && (f->f_drain == GroundNode)))
		{
		    msg6 += 1;
		    if (msg6 <= CheckMsgLimit)
			printf("FET between Vdd and GND: %s\n", PrintFET(f));
		    else skipped += 1;
		    if (msg6 == CheckMsgLimit)
		    {
			printf("    No more of these messages");
			printf(" will be printed.\n");
		    }
		}
	    }
	}

	/* Make sure the node can drive and be driven. */

	if (!((n->n_flags & NODEISOUTPUT) || drives))
	{
	    msg3 += 1;
	    if (msg3 <= CheckMsgLimit)
	    {
		printf("Node doesn't drive anything: %s\n", PrintNode(n));
		if (msg3 == 1)
		{
		    printf("    Maybe you haven't marked all the");
		    printf(" inputs and outputs?\n");
		}
	    }
	    else skipped += 1;
	    if (msg3 == CheckMsgLimit)
		printf("    No more of these messages will be printed.\n");
	}
	if (!((n->n_flags & NODEISINPUT) || driven))
	{
	    msg2 += 1;
	    if (msg2 <= CheckMsgLimit)
	    {
		printf("Node isn't driven: %s\n", PrintNode(n));
		if (msg2 == 1)
		{
		    printf("    Maybe you haven't marked all the");
		    printf(" inputs and outputs?\n");
		}
	    }
	    else skipped += 1;
	    if (msg2 == CheckMsgLimit)
		printf("    No more of these messages will be printed.\n");
	}
    }

    if (skipped > 0)
	printf("%d duplicate errors not printed.\n", skipped);
}


RatioCmd(string)
char *string;			/* Contains args to modify limits. */

/*---------------------------------------------------------
 *	This command checks transistor ratios for nMOS.
 *	The command line may contain arguments of the form
 *	"limit value" to change the acceptable range for
 *	ratios.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	None, except that error messages are printed if ratio
 *	errors are found.
 *---------------------------------------------------------
 */

{
    register Node *n;
    register Pointer *p;
    char *arg;
    int index;
    static char *(limitTable[]) =
    {
	"normalhigh",
	"normallow",
	"passhigh",
	"passlow",
	NULL
    };

    HashInit(&ratioTable, 16);
    ratioDups = 0;
    NormalLow = 3.8;
    NormalHigh = 4.2;
    PassLow = 7.8;
    PassHigh = 8.2;

    /* Read in parameters, if there are any. */

    while ((arg = NextField(&string)) != NULL)
    {
	index = Lookup(arg, limitTable);
	arg = NextField(&string);
	if (index < 0)
	{
	    printf("Bad limit:  try normalhigh, normallow, ");
	    printf("passhigh, or passlow.\n");
	    continue;
	}
	if (arg == NULL)
	{
	    printf("No value given for %s.\n", limitTable[index]);
	    break;
	}
	switch(index)
	{
	    case 0: NormalHigh = atof(arg); break;
	    case 1: NormalLow = atof(arg); break;
	    case 2: PassHigh = atof(arg); break;
	    case 3: PassLow = atof(arg); break;
	}
    }

    /* Go through every node in the hash table.  If the node
     * has a load attached to it, then check each path to ground
     * from the node and make sure that the ratio of the ground
     * path's size to the load's size is within limits.
     */
    
    totalRatioErrs = 0;
    for (n = FirstNode;  n != NULL;  n = n->n_next)
    {
	if ((n == VddNode) || (n == GroundNode)) continue;
	for (p = n->n_pointer;  p != NULL;  p = p->p_next)
	{
	    if (p->p_fet->f_typeIndex == FET_NLOAD) break;
	}
	if (p == NULL) continue;
	checkRatio(n, n, p->p_fet->f_aspect, 0.0, FALSE);
    }
    HashKill(&ratioTable);
    if (totalRatioErrs > RatioTotalLimit)
	printf("Ratio checking aborted after %d errors.\n", totalRatioErrs);
    if (ratioDups > 0)
	printf("%d duplicate ratios not printed.\n", ratioDups);
}


checkRatio(origin, current, loadSize, curSize, gotPass)
Node *origin;			/* Node from which search started (used
				 * only for printing error messages).
				 */
Node *current;			/* Current node in search. */
float loadSize;			/* Size of load. */
float curSize;			/* Net size of path from load's output
				 * up to current.
				 */
int gotPass;			/* True means one of the transistors
				 * along the way is driven through a
				 * pass transistor.
				 */

/*--------------------------------------------------------
 *	This routine is the workhorse of the ratio-checker.
 *	It scans recursively through pulldowns and checks
 *	ratios whenever it reaches ground.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Error messages are printed if ratio errors are found.
 *--------------------------------------------------------
 */

{
    register Pointer *p, *p2;
    register FET *f;
    char ratioString[100];
    float newSize, x, y, ratio;
    int newPass, i;
    register Node *next;
    HashEntry *h;

    /* Set a flag to avoid infinite recursion through circular structures. */

    current->n_flags |= NODEINPATH;

    /* Skim through all transistors attached to the node.  If the
     * transistor connects to ground, check the ratio.  Otherwise,
     * call this routine recursively to check on the other side.
     */
    
    for (p = current->n_pointer;  p != NULL;  p = p->p_next)
    {
	if (totalRatioErrs > RatioTotalLimit)
	{
	    current->n_flags &= ~NODEINPATH;
	    return;
	}
	f = p->p_fet;

	/* Make sure that the transistor's source or drain connects
	 * to this node, and that flow in this direction is OK.
	 */
	
	if (f->f_source == current)
	{
	    next = f->f_drain;
	    if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	}
	else if (f->f_drain == current)
	{
	    next = f->f_source;
	    if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	}
	else continue;

	/* Ignore circular paths and paths to Vdd. */

	if (next->n_flags & NODEINPATH) continue;
	if (next == VddNode) continue;
	if (f->f_fp != NULL) if (!FlowLock(f, next)) continue;

	/* See if the gate of this transistor is load-driven or
	 * pass transistor driven.  An input counts just like a
	 * load-driven node.  Ignore depletion transistors.
	 */
	
	newPass = gotPass;
	if (!(f->f_flags & FET_ONALWAYS))
	{
	    for (p2 = f->f_gate->n_pointer;  p2 != NULL;  p2 = p2->p_next)
	    {
	        if (p2->p_fet->f_typeIndex == FET_NLOAD) break;
		if (p2->p_fet->f_typeIndex == FET_NSUPER) break;
	    }
	    if ((p2 == NULL) && !(f->f_gate->n_flags & NODEISINPUT))
		newPass = TRUE;
	}

	/* Now check the ratio. */

	newSize = curSize + f->f_aspect;
	if (next == GroundNode)
	{
	    ratio = loadSize/newSize;
	    if (newPass)
	    {
		if ((ratio >= PassLow) && (ratio <= PassHigh))
		    goto endRatioLoop;
	    }
	    else
		if ((ratio >= NormalLow) && (ratio <= NormalHigh))
		    goto endRatioLoop;
	    
	    /* The next piece of code is to eliminate extra messages:
	     * more than one message about a particular node, more than
	     * a certain number of messages for a given ratio, and more
	     * than a certain total number of messages.
	     */

	    if (origin->n_flags & NODERATIOERROR)
	    {
		ratioDups += 1;
		goto endRatioLoop;
	    }
	    origin->n_flags |= NODERATIOERROR;
	    totalRatioErrs += 1;
	    PrintFETXY(f, &x, &y);
	    (void) sprintf(ratioString, "pullup = %.1f, pulldown = %.1f",
		loadSize, newSize);
	    h = HashFind(&ratioTable, ratioString);
	    i = (int) HashGetValue(h) + 1;
	    HashSetValue(h, i);
	    if (i > RatioLimit) ratioDups += 1;
	    else
	    {
	        printf("Ratio error at node %s: %s\n",
		    PrintNode(origin), ratioString);
	        printf("    Last pulldown is at (%.0f, %.0f)\n", x, y);
	    }
	}
	else checkRatio(origin, next, loadSize, newSize, newPass);
	endRatioLoop: if (f->f_fp != NULL) FlowUnlock(f, next);
    }
    current->n_flags &= ~NODEINPATH;
}
