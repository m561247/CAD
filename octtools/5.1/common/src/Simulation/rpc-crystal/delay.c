/* delay.c
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)delay.c	2.17 (Berkeley) 10/21/84"
 *
 * This file contains routines that figure out delays through the
 * circuit network from when one node changes until everything in
 * the system is stable.  Actually, the routines in this file just
 * compute stages (chains of transistors) that are affected, then
 * call routines in the model package to compute delays.  The
 * general process is as follows:
 *
 * When it is determined that a gate will rise or fall, and
 * that this is the worst rise or fall time seen so far, then
 * delay information has to be propagated.  First, find all
 * paths from the transistor to a source of GND or Vdd (busses
 * and inputs are also sources).  Then for each source, find
 * all paths from the other side of the transistor to gates or
 * busses.  Compute the delay for each combination of source
 * and target.  If this results in a new worst-case settling
 * time for the target, repeat the process recursively starting
 * at the target.  The search is depth-first, and has to be in
 * order to handle circularities cleanly.
 */

#include "crystal.h"
#include "cOct.h"
#include "hash.h"

/* Imports from other Crystal modules: */

extern Node *VddNode, *GroundNode;
extern char *RunStats(), *PrintNode(), *PrintFET(), *NextField();
extern char *ExpandNext();
extern HashTable NodeTable;
extern float DPMemThreshold, DPAnyThreshold, DPWatchThreshold;

/* Imports from the Oct module						*/
extern unsigned cOctStatus;

/* Limit of how many nodes to search before giving up: */

int DelayLimit = 200000;
static int count = 0;

/* Limit of how many stage overflow error messages to print before stopping
 * printing them.
 */

int StageOverflowLimit = 10;
static int stageOverflowCount = 0;

/* Flags telling what information to print out. */

int DelayPrint = FALSE;
int DelayPrintAll = FALSE;

/* Threshold capacitance (in pf) at which a node is considered to be a bus. */

float DelayBusThreshold = 2.0;

/* The following counters are used to gather statistics. */

int DelayChaseVGCalls = 0;
int DelayChaseGatesCalls = 0;
int DelayPropagateCalls = 0;
int DelayChaseLoadsCalls = 0;
int DelayFeedbacks = 0;


int
chaseVG(stage)
register Stage *stage;		/* Describes the stage seen so far.  On
				 * entry the stage contains >= 1 entries
				 * in piece1, plus piece2Node[0] must be
				 * initialized with the correct value for
				 * the first call to chaseGates.
				 */

/*---------------------------------------------------------
 *	This procedure finds paths from a given node to a
 *	source of Vdd or GND, then calls chaseGates for
 *	each source it finds.
 *
 *	Results:
 *	The return value is TRUE if the procedure terminated
 *	normally, FALSE if it gave up because too many nodes
 *	had been searched.
 *
 *	Side Effects:
 *	Information is written into stage to describe paths
 *	to Vdd or GND.
 *---------------------------------------------------------
 */

{
    Pointer *p;
    register FET *f;
    register Node *node;
    Node *other;
    int size, result;

    DelayChaseVGCalls += 1;

    /* If we've already seen this node, then return immediately. */

    size = stage->st_piece1Size;
    node = stage->st_piece1Node[size-1];
    if (node->n_flags & NODEINPATH) return TRUE;
    node->n_flags |= NODEINPATH;

    if (count >= DelayLimit)
    {
	cvgGiveUp: printf("ChaseVG giving up at %s.\n", PrintNode(node));
	node->n_flags &= ~NODEINPATH;
	return FALSE;
    }

    /* See if this node is highly capacitive (input or bus).
     * If it is, then chaseGates immediately.
     */

    if ((node->n_flags &
        (NODEISINPUT|NODEISBUS|NODEISPRECHARGED|NODEISPREDISCHARGED))
	|| (node->n_C >= DelayBusThreshold))
    {
	if (!(node->n_flags & (NODE0ALWAYS)))
	{
	    stage->st_piece2Size = 1;
	    stage->st_rise = TRUE;
	    if (!chaseGates(stage)) goto cvgGiveUp;
	}
	if (!(node->n_flags & (NODE1ALWAYS)))
	{
	    stage->st_piece2Size = 1;
	    stage->st_rise = FALSE;
	    if (!chaseGates(stage)) goto cvgGiveUp;
	}
	goto cvgDone;
    }

    /* Scan all of the FETs whose sources or drains connect to
     * the node, and either call chaseGates if they connect to
     * Vdd or GND, or call this procedure recursively.
     */
    
    for (p = node->n_pointer; p != NULL; p = p->p_next)
    {
	f = p->p_fet;

	/* Make sure that information can flow through this
	 * transistor in the desired direction.
	 */

	if (f->f_source == node)
	{
	    other = f->f_drain;
	    if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	}
	else if (f->f_drain == node)
	{
	    other = f->f_source;
	    if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	}
	else continue;
	if (f->f_flags & FET_FORCEDOFF) continue;
	if (f->f_fp != NULL) if (!FlowLock(f, other)) continue;

	/* Make sure that there's room in the delay stage for more info.
	 * If not, it's an error (too many transistors in series).
	 */
	
	if (size >= PIECELIMIT)
	{
	    if (++stageOverflowCount <= StageOverflowLimit)
	    {
		printf("More than %d transistors in series, see %s.\n",
		    PIECELIMIT, PrintNode(node));
		if (stageOverflowCount == StageOverflowLimit)
		{
		    printf("No more messages of this kind");
		    printf(" will be printed.....\n");
		}
	    }
	    if (f->f_fp != NULL) FlowUnlock(f, other);
	    break;
	}

	/* Add more information to the delay stage. */

	stage->st_piece1FET[size] = f;
	stage->st_piece1Node[size] = other;
	stage->st_piece1Size = size + 1;

	if (other == VddNode)
	{
	    stage->st_piece2Size = 1;
	    stage->st_rise = TRUE;
	    result = chaseGates(stage);
	}
	else if (other == GroundNode)
	{
	    stage->st_piece2Size = 1;
	    stage->st_rise = FALSE;
	    result = chaseGates(stage);
	}
	else result = chaseVG(stage);
	if (f->f_fp != NULL) FlowUnlock(f, other);
	if (!result) goto cvgGiveUp;
    }

    /* Mark this node as no longer in the current path. */

    cvgDone: node->n_flags &= ~NODEINPATH;
    return TRUE;
}


int
chaseGates(stage)
register Stage *stage;		/* Describes stage up to and including
				 * node from which to chase gates.
				 */

/*---------------------------------------------------------
 *	This routine chases out stages to gates, recalculates
 *	delays, and calls DelayPropagate recursively if necessary.
 *
 *	Results:
 *	TRUE is returned if the routine completed normally, FALSE
 *	is returned if it gave up because too many nodes had
 *	been searched.
 *
 *	Side Effects:
 *	Delay times in nodes get updated.  The information in
 *	the stage is augmented.  At the time of this call, the
 *	last entry in piece2 of the stage contains a node but
 *	no FET.  Piece1 of the stage is complete (meaning 0 or
 *	more entries for FETs and Nodes).
 *---------------------------------------------------------
 */

{
    register Pointer *p;
    register FET *f;
    register Node *node;
    Node *other;
    int size, result;

    DelayChaseGatesCalls += 1;

    /* If this node is fixed in value, then return immediately:
     * the delay to here must be zero.  If the node is already
     * in the path we're checking, then also return to avoid
     * circular scans.  In this case, we've just discovered a
     * static memory element (feedback).  Before returning, if
     * this is a static memory element then record the delay.
     * All memory nodes found here are of the "OR" type.
     */
    
    size = stage->st_piece2Size;
    stage->st_piece2FET[size-1] = NULL;	/* (needed by the modelling routine) */
    node = stage->st_piece2Node[size-1];
    if (node->n_flags & (NODEINPATH|NODE1ALWAYS|NODE0ALWAYS))
    {
	if (node->n_flags & (NODE1ALWAYS|NODE0ALWAYS)) return TRUE;
	DelayFeedbacks += 1;
	if (stage->st_prev->st_time >= DPMemThreshold)
	    DPRecord(stage->st_prev, 1);
	return TRUE;
    }
    
    /* If this node is precharged, there's no need to consider
     * rising transitions.  Same for predischarged.
     */
    
    if ((node->n_flags & NODEISPRECHARGED) && stage->st_rise) return TRUE;
    if ((node->n_flags & NODEISPREDISCHARGED) && (!stage->st_rise))
	return TRUE;
    node->n_flags |= NODEINPATH;

    if (count >= DelayLimit)
    {
	cgGiveUp: printf("ChaseGates giving up at %s.\n", PrintNode(node));
	node->n_flags &= ~NODEINPATH;
	return FALSE;
    }

    /* If the node is an input, then we can usually return
     * right away since its delay is fixed by the outside
     * world.  There are two exceptions.  If the node is also
     * an output, then we still have to calculate delays to
     * it.  Also, if this node is absolutely the only entry in
     * its stage so far, then this stage starts from the node;
     * in that case we are computing delays FROM the node, so
     * we skip the delay calculation and go on to searching through
     * transistors.
     */

    /* Added: if NODE is blocked, don't propogate critical path. */

    if ((stage->st_piece1Size == 0) && (size == 1)) goto searchFets;
    if (node->n_flags & NODEISINPUT)
    {
	if (!(node->n_flags & NODEISOUTPUT)) goto cgDone;
    }
    ModelDelay(stage);
    if (stage->st_rise)
    {
	if (stage->st_time > node->n_hiTime)
	{
	    node->n_hiTime = stage->st_time;
	    DelayPropagate(stage);
	}
    }
    else if (stage->st_time > node->n_loTime)
    {
	node->n_loTime = stage->st_time;
	DelayPropagate(stage);
    }

    if (count >= DelayLimit) goto cgGiveUp;

    /* If this is a bus, then DelayPropagate already propagated
     * the information on from the bus, so we can just return.
     */
    
    /* Added: if NODEISBLOCKED, that is, we want to stop at this node
     * since it is the end of a sub-piece of a larger design we should
     * just return.  This terminates one path of a critical path search.
     * The user must assure that delay paths between ANY blocked nodes
     * cannot occur.
     */
    if ((node->n_flags &
	(NODEISBUS|NODEISPRECHARGED|NODEISPREDISCHARGED|NODEISBLOCKED))
	|| (node->n_C >= DelayBusThreshold))
    {
	node->n_flags &= ~NODEINPATH;
	return TRUE;
    }

    /* Now go through the fets that connect to this node.
     * For each non-load transistor, keep chasing gates on the
     * other side, unless the other side is an input or we're
     * going the wrong way through a transistor.
     */

    searchFets: for (p = node->n_pointer; p != NULL; p = p->p_next)
    {
	f = p->p_fet;
	if (f->f_flags & FET_FORCEDOFF) continue;
	if (f->f_source == node)
	{
	    other = f->f_drain;
	    if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	}
	else if (f->f_drain == node)
	{
	    other = f->f_source;
	    if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	}
	else continue;
	if (other->n_flags & (NODE1ALWAYS|NODE0ALWAYS)) continue;
	if (f->f_fp != NULL) if (!FlowLock(f, node)) continue;

	/* Make sure there is enough space in the stage for more
	 * info.  If not, it is an error.
	 */

	if (size >= PIECELIMIT)
	{
	    if (++stageOverflowCount <= StageOverflowLimit)
	    {
		printf("More than %d transistors in series, see %s.\n",
		    PIECELIMIT, PrintNode(node));
		if (stageOverflowCount == StageOverflowLimit)
		{
		    printf("No more messages of this kind will");
		    printf(" be printed.....\n");
		}
	    }
	    if (f->f_fp != NULL) FlowUnlock(f, node);
	    break;
	}

	stage->st_piece2FET[size-1] = f;
	stage->st_piece2Node[size] = other;
	stage->st_piece2Size = size + 1;

	result = chaseGates(stage);
	if (f->f_fp != NULL) FlowUnlock(f, node);
	if (!result) goto cgGiveUp;
    }
	
    /* Remove the stage marking from this node. */

    cgDone: node->n_flags &= ~NODEINPATH;
    return TRUE;
}


chaseLoads(node, stage)
Node *node;			/* Node at which to search for loads. */
register Stage *stage;		/* Stage to use when load is found. */

/*---------------------------------------------------------
 *	This procedure is used to chase out loads that can
 *	drive a stage when a transistor turns off.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	This procedure searches out recursively all the nodes
 *	that can legally be reached from node, looking for
 *	loads.  For each load found, the stage is modified
 *	and chaseGates is called.
 *
 *	Design Note:
 *	This procedure uses a restrictive notion of what is
 *	a load.  A load will be found when a given FET turns
 *	off ONLY if signals could conceivably flow from the
 *	FET to the load.  This will handle the normal case
 *	of loads part of logic gates, but will not handle
 *	several other conceivable situations.  Another alternative,
 *	which wasn't implemented, is to find ALL loads that can
 *	be reached through FET channels from transistor that
 *	turned off.  This looks like it's extremely expensive,
 *	since it might involve searching large pass transistor
 *	arrays.
 *---------------------------------------------------------
 */

{
    register Pointer *p;
    register FET *f;
    Node *other;

    DelayChaseLoadsCalls += 1;

    /* Make sure that we're not looping circularly.  A circularity
     * here corresponds to an "AND" type of memory node;  when this
     * is found, call the delay recorder.
     */

    if (node->n_flags & (NODEINPATH|NODE1ALWAYS|NODE0ALWAYS))
    {
	if (node->n_flags & (NODE1ALWAYS|NODE0ALWAYS)) return;
	DelayFeedbacks += 1;
	if (stage->st_prev->st_time >= DPMemThreshold)
	    DPRecord(stage->st_prev, 1);
	return;
    }

    /* See if this node has any loads attached to it. If so,
     * then chase gates from the load and return.  A load is defined
     * as a transistor that is always on and connects to a node
     * that is forced to a particular value.
     */
    
    for (p = node->n_pointer;  p != NULL;  p = p->p_next)
    {
	f = p->p_fet;
	if (!(f->f_flags & (FET_FORCEDON|FET_ONALWAYS))) continue;
	if (f->f_source == node)
	{
	    other = f->f_drain;
	    if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	}
	else if (f->f_drain == node)
	{
	    other = f->f_source;
	    if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	}
	else continue;
	if (other->n_flags & NODE1ALWAYS) stage->st_rise = TRUE;
	else if (other->n_flags & NODE0ALWAYS) stage->st_rise = FALSE;
	else continue;
	if (f->f_fp != NULL) if (!FlowLock(f, other)) continue;
	stage->st_piece1Size = 0;
	stage->st_piece2Size = 2;
	stage->st_piece2Node[0] = other;
	stage->st_piece2FET[0] = f;
	stage->st_piece2Node[1] = node;
	chaseGates(stage);
	if (f->f_fp != NULL) FlowUnlock(f, other);
	return;
    }

    /* This node doesn't have any loads.  In this case, continue
     * working outwards, looking recursively for loads.  Note: the
     * NODEINPATH flag doesn't get set until here, because it
     * must be UNSET when we call chaseGates above (chaseGates will
     * set it itself).
     */

    node->n_flags |= NODEINPATH;
    for (p = node->n_pointer;  p != NULL;  p = p->p_next)
    {
	f = p->p_fet;
	if (f->f_flags & FET_FORCEDOFF) continue;
	if (f->f_source == node)
	{
	    other = f->f_drain;
	    if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	}
	else if (f->f_drain == node)
	{
	    other = f->f_source;
	    if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	}
	else continue;
	if (f->f_fp != NULL)
	{
	    if (FlowLock(f, node))
	    {
		chaseLoads(other, stage);
		FlowUnlock(f, node);
	    }
	}
	else chaseLoads(other, stage);
    }

    node->n_flags &= ~NODEINPATH;
    return;
}


DelayPropagate(prevStage)
register Stage *prevStage;	/* Describes stage leading to node whose
				 * worst-case delay has changed.
				 */

/*---------------------------------------------------------
 *	This procedure considers the consequences of a given
 *	node changing at a given time.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	PrevStage gives information about which node has changed
 *	and when it changed.  This procedure propagates changes
 *	throughout the circuit by looking for transistors that
 *	will be activated or deactivated by the change.  For each
 *	of these, a new stage is created and analyzed recursively.
 *---------------------------------------------------------
 */

{
    register Pointer *p;
    register FET *f;
    register Node *node;
    Stage stage;
    int result, oldFlags;

    DelayPropagateCalls += 1;

    /* Quit if too many delays have been calculated:  this probably
     * means that nodes aren't marked properly (user error), or else
     * Crystal is messing up.
     */

    if (++count >= DelayLimit)
    {
	if (count == DelayLimit)
	{
	    printf("Aborting: no solution after examining %d stages.\n",
	        DelayLimit);
	    printf("Perhaps you forgot to mark some of the flow in");
	    printf(" pass transistors?\n");
	    printf("The information below may point out the trouble spot:\n");
	}
	return;
    }

    node = prevStage->st_piece2Node[prevStage->st_piece2Size - 1];
    if (DelayPrintAll || (DelayPrint && isalpha(node->n_name[0])))
	printf("Delay at %s set to %6.1fns up, %6.1fns down.\n",
	PrintNode(node), node->n_hiTime, node->n_loTime);
    
    /* Record delay information for posterity. */

    if (prevStage->st_time >= DPAnyThreshold) DPRecord(prevStage, 0);
    if ((node->n_flags & NODEISDYNAMIC)
	&& (prevStage->st_time >= DPMemThreshold)) DPRecord(prevStage, 1);
    if ((node->n_flags & NODEISWATCHED)
	&& (prevStage->st_time >= DPWatchThreshold)) DPRecord(prevStage, 2);
    
    /* We've recorded the times up to this node, but let's not continue */
    /* Don't propagate critical path beyond this blocked node.*/
    if (node->n_flags & NODEISBLOCKED) return;

    stage.st_prev = prevStage;

    /* If this node is an input or bus, then call chaseGates immediately.
     * A NULL piece1 is used to indicate the input status to the
     * device modellers.
     */

    if ((node->n_flags &
	(NODEISINPUT|NODEISBUS|NODEISPRECHARGED|NODEISPREDISCHARGED)) ||
	(node->n_C >= DelayBusThreshold))
    {
	stage.st_piece1Size = 0;
	stage.st_piece2Size = 1;
	stage.st_piece2Node[0] = node;
	stage.st_rise = prevStage->st_rise;
	
	/* We have to turn off the NODEINPATH flag, because if this
	 * procedure is invoked from chasGates with a bus then the bus
	 * is both the destination of one delay stage and the trigger
	 * of the next.  On the other hand, we have to be sure to turn
	 * the flag on again after chaseGates returns.
	 */

	oldFlags = node->n_flags & NODEINPATH;
	node->n_flags &= ~NODEINPATH;
	result = chaseGates(&stage);
	node->n_flags |= oldFlags;
	if (!result) return;
    }

    /* Scan through all the FETs connecting to this node. */

    for (p = node->n_pointer; p != NULL; p = p->p_next)
    {
	f = p->p_fet;
	if (f->f_gate != node) continue;
	if (f->f_flags & (FET_ONALWAYS|FET_FORCEDON|FET_FORCEDOFF)) continue;

	/* If the signal change could potentially turn the transistor
	 * on, then look first at the source side, then at the drain
	 * side.  For each side, if info could flow from that side
	 * then call chaseVG to find the source.  If the side connects
	 * directly to a supply rail, then bypass the call to chaseVg
	 * and go straight to chaseGates (this is a performance
	 * optimization and isn't strictly necessary).
	 */

	if ((prevStage->st_rise && !(f->f_flags & FET_ON0))
	    || ((!prevStage->st_rise) && !(f->f_flags & FET_ON1)))
	{
	    if ((f->f_flags & FET_FLOWFROMSOURCE)
		&& ((f->f_fp == NULL) || FlowLock(f, f->f_source)))
	    {
		stage.st_piece1Size = 1;
		stage.st_piece1FET[0] = f;
		stage.st_piece1Node[0] = f->f_source;
		stage.st_piece2Size = 1;
		stage.st_piece2Node[0] = f->f_drain;
		if (f->f_source == GroundNode)
		{
		    stage.st_rise = FALSE;
		    result = chaseGates(&stage);
		}
		else if (f->f_source == VddNode)
		{
		    stage.st_rise = TRUE;
		    result = chaseGates(&stage);
		}
		else result = chaseVG(&stage);
		if (f->f_fp != NULL) FlowUnlock(f, f->f_source);
		if (!result) return;
	    }
	    if ((f->f_flags & FET_FLOWFROMDRAIN)
		&& ((f->f_fp == NULL) || FlowLock(f, f->f_drain)))
	    {
		stage.st_piece1Size = 1;
		stage.st_piece1FET[0] = f;
		stage.st_piece1Node[0] = f->f_drain;
		stage.st_piece2Size = 1;
		stage.st_piece2Node[0] = f->f_source;
		if (f->f_drain == GroundNode)
		{
		    stage.st_rise = FALSE;
		    result = chaseGates(&stage);
		}
		else if (f->f_drain == VddNode)
		{
		    stage.st_rise = TRUE;
		    result = chaseGates(&stage);
		}
		else result = chaseVG(&stage);
		if (f->f_fp != NULL) FlowUnlock(f, f->f_drain);
		if (!result) return;
	    }
	}

	/* This transition could only have turned the transistor off.
	 * In this case, we scan out on each side of the transistor
	 * looking for a load to take over now that the transistor
	 * is turned off.  A couple of notes here:  since this fet
	 * is turning OFF, there's no need to do flow locking.  However,
	 * when chasing loads on one side of the transistor, we must
	 * set the node on the other side to be in the path.  Otherwise,
	 * chaseLoads will try to use the transistor we just turned off
	 * on the path to a load.  Be careful:  it's possible that
	 * somebody else has already turned on the NODEINPATH flag.
	 */

	else
	{
	    if (f->f_flags & FET_FLOWFROMSOURCE)
	    {
		oldFlags = f->f_source->n_flags & NODEINPATH;
		f->f_source->n_flags |= NODEINPATH;
		chaseLoads(f->f_drain, &stage);
		if (!oldFlags) f->f_source->n_flags &= ~NODEINPATH;
	    }
	    if (f->f_flags & FET_FLOWFROMDRAIN)
	    {
		oldFlags = f->f_drain->n_flags & NODEINPATH;
		f->f_drain->n_flags |= NODEINPATH;
		chaseLoads(f->f_source, &stage);
		if (!oldFlags) f->f_drain->n_flags &= ~NODEINPATH;
	    }
	}
    }
}


DelaySetFromString(string)
char *string;			/* String containing expandable node name,
				 * rise time, fall time.
				 */

/*---------------------------------------------------------
 *	This routine parses a string and calls DelayPropagate.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Delay information gets propagated throughout the network.
 *---------------------------------------------------------
 */

{
    Node *n;
    char name[100];
    float hiTime, loTime;
    Stage stage;

    if (sscanf(string, "%99s %f %f", name,&hiTime, &loTime) != 3)
    {
	printf("Rise/fall times not specified correctly.\n");
	return;
    }

    ExpandInit(name);
    while ((n = (Node *) ExpandNext(&NodeTable)) != NULL)
    {
	stage.st_prev = NULL;
	stage.st_piece1Size = 0;
	stage.st_piece2Size = 1;
	stage.st_piece2Node[0] = n;
	stage.st_piece2FET[0] = NULL;
	if (hiTime >= 0.0)
	{
	    stage.st_time = hiTime;
	    stage.st_rise = TRUE;
	    stage.st_edgeSpeed = 0;
	    if (hiTime > n->n_hiTime) n->n_hiTime = hiTime;
	    DelayPropagate(&stage);
	}
	if (loTime >= 0.0)
	{
	    stage.st_time = loTime;
	    stage.st_rise = FALSE;
	    stage.st_edgeSpeed = 0;
	    if (loTime > n->n_loTime) n->n_loTime = loTime;
	    DelayPropagate(&stage);
	}
    }

    printf("(%d stages examined.)\n", count);
    count = 0;

    if( cOctStatus & COCT_ON){
	cOctStatus |= COCT_PENDING_CRIT_UPDATE;
    }
}


DelayStats()

/*---------------------------------------------------------
 *	This procedure prints out statistics.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Statistics gathered while the module has executed are
 *	printed on standard output.
 *---------------------------------------------------------
 */

{
    printf("Number of calls to chase Vdd or GND: %d.\n", DelayChaseVGCalls);
    printf("Number of calls to chase gates: %d.\n", DelayChaseGatesCalls);
    printf("Number of calls to chase loads: %d.\n", DelayChaseLoadsCalls);
    printf("Number of calls to propagate delays: %d.\n", DelayPropagateCalls);
    printf("Number of delay feedback paths: %d.\n", DelayFeedbacks);
}
