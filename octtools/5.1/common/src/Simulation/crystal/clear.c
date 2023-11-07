/* clear.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)clear.c	2.5 (Berkeley) 5/1/83"
 *
 * This file contains a single routine that clears flags
 * for Crystal, a VLSI timing analyzer.
 */

#include "crystal.h"
#include "hash.h"

extern char *PrintNode(), *NextField();
extern Node *VddNode, *GroundNode, *FirstNode;
extern HashTable NodeTable, FlowTable;

ClearCmd()

/*---------------------------------------------------------
 *	ClearCmd reinitializes timing and flag information
 *	in all of the nodes, transistors, and flow locks.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	This routine clears all the information set by flow
 *	analysis, simulation, and delay analysis.  It doesn't
 *	clear information such as dynamic memory nodes or inputs.
 *	Along the way it checks to make sure that certain flags
 *	that aren't supposed to be left set, like NODEINPATH,
 *	haven't been left set.
 *---------------------------------------------------------
 */

{
    HashSearch hs;
    HashEntry *h;
    register Node *n;
    register Pointer *p;
    register FET *f;
    Flow *flow;

    for (n = FirstNode; n != NULL; n = n->n_next)
    {
	if (n->n_flags & NODEINPATH)
	    printf("Crystal bug: NODEINPATH left set at %s.\n", PrintNode(n));
	n->n_flags &= NODEISDYNAMIC|NODEISINPUT|NODEISOUTPUT|NODEISBUS;
	n->n_hiTime = n->n_loTime = -1.0;
	n->n_count = 0;
	for (p = n->n_pointer;  p != NULL;  p = p->p_next)
	{
	    f = p->p_fet;
	    if (f->f_gate != n) continue;
	    f->f_flags &= ~(FET_FLOWFROMSOURCE|FET_FLOWFROMDRAIN
		|FET_FORCEDON|FET_FORCEDOFF);
	}
    }

    HashStartSearch(&hs);
    while ((h = HashNext(&FlowTable, &hs)) != NULL)
    {
	flow = (Flow *) HashGetValue(h);
	if (flow == 0) continue;
	if (flow->fl_entered != (Node *) NULL)
	    printf("Crystal bug: flow %s still enter-locked at %s.\n",
	    flow->fl_name, PrintNode(flow->fl_entered));
	flow->fl_entered = (Node *) NULL;
	if (flow->fl_left != (Node *) NULL)
	    printf("Crystal bug: flow %s still leave-locked at %s.\n",
	    flow->fl_name, PrintNode(flow->fl_left));
	flow->fl_left = (Node *) NULL;
	flow->fl_flags = 0;
    }

    /* It is crucial that Vdd and GND be left marked as inputs and
     * as fixed in value at all times.  Otherwise the routines in
     * mark.c will misbehave terribly.
     */

    GroundNode->n_flags |= NODEISINPUT|NODE0ALWAYS;
    VddNode->n_flags |= NODEISINPUT|NODE1ALWAYS;

    /* Clear the recorded delay information. */

    DPClear();
}
