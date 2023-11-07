/* mark.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)mark.c	2.16 (Berkeley) 10/21/84"
 *
 * The routines in this file constitute the marking phase of
 * Crystal.  They set information about transistor flow and
 * about node levels that are fixed.  This information is
 * used during delay analysis to avoid looking at paths that
 * can't occur.
 */

#include "crystal.h"
#include "hash.h"

extern char *PrintFET(), *PrintNode(), *NextField();
extern char *ExpandNext();
extern Node *VddNode, *GroundNode, *FirstNode;
extern Type TypeTable[];
extern HashTable NodeTable;
extern float DelayBusThreshold;

/* The following two flags determine whether we print out node names
 * as we set them to particular values.
 */

int MarkPrint = FALSE;		/* Print names beginning with alphas. */
int MarkPrintAll = FALSE;	/* Print all names. */
int MarkPrintDynamic = FALSE;	/* Print dynamic node names when found. */

/* The following two flags determine whether we print out node names
 * as we mark the flow direction.
 */

int FlowPrint = FALSE;
int FlowPrintAll = FALSE;

/* The following variables record information for the "statistics" command. */

int Mark0Nodes = 0;
int Mark1Nodes = 0;
int MarkDynamicCount = 0;
int MarkSimSearch = 0;
int MarkFlowSearch = 0;

/* The following stuff is used to cause a graceful abort in findStrengh
 * when there aren't enough flow indicators.
 */

int MarkFSLimit = 1000;
int MarkFSCount = 0;


MarkNodeLevel(node, level, propAnyway)
register Node *node;		/* Node to mark. */
int level;			/* 0 or 1 forced level of node. */
int propAnyway;			/* Propagate setting even if node was
				 * already set.
				 */

/*---------------------------------------------------------
 *	This routine is called when it is known for sure that
 *	a particular node will always have a particular value.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The node is marked to have the given level.  If this
 *	setting changes the status of neighboring transistors
 *	and nodes, then marking may propagate throughout the
 *	network.  If the node was already set to a value, then
 *	there's no need to propagate unless propAnyway is TRUE.
 *---------------------------------------------------------
 */

{
    register Pointer *p;

    /* Make sure that there is no conflict in setting the node's
     * level, then set the flags on the node.  If the level was
     * already forced, then we can return immediately (this check
     * prevents circular loops).
     */

    if (level == 0)
    {
	if (node->n_flags & NODE1ALWAYS)
	    printf("Node forced to 1, then to 0 (0 wins): %s.\n",
		PrintNode(node));
	node->n_flags &= ~NODE1ALWAYS;
	if ((node->n_flags & NODE0ALWAYS) && !propAnyway) return;
	node->n_flags |= NODE0ALWAYS;
	Mark0Nodes += 1;
    }
    else
    {
	if (node->n_flags & NODE0ALWAYS)
	    printf("Node forced to 0, then to 1 (1 wins): %s.\n",
		PrintNode(node));
	node->n_flags &= ~NODE0ALWAYS;
	if ((node->n_flags & NODE1ALWAYS) && !propAnyway) return;
	node->n_flags |= NODE1ALWAYS;
	Mark1Nodes += 1;
    }
    if (MarkPrintAll || (MarkPrint && isalpha(node->n_name[0])))
	printf("Node forced to %d: %s.\n", level, PrintNode(node));

    /* Find all of the FETs connecting to this node, and take
     * action (if necessary) to propagate the level setting.
     */

    for (p = node->n_pointer; p != NULL; p = p->p_next)
    {
	register FET *f;

	f = p->p_fet;

	/* Where this node connects to sources or drains,
	 * check to see if level information will propagate
	 * to the next node over.
	 */
	
	if ((f->f_source == node)
	    && (f->f_flags & FET_FLOWFROMSOURCE)
	    && (f->f_flags & (FET_FORCEDON|FET_ONALWAYS)))
	    checkNode(f->f_drain);
	if ((f->f_drain == node)
	    && (f->f_flags & FET_FLOWFROMDRAIN)
	    && (f->f_flags & (FET_FORCEDON|FET_ONALWAYS)))
	    checkNode(f->f_source);
	
	/* If the node connects to the transistor's gate and turns
	 * the transistor on, this may cause one of the nodes on
	 * either side to become fixed in value.  If the transistor
	 * is now turned off, then the source of drain may become
	 * fixed because of the reduced "competition" for that node.
	 */
	
	if (f->f_gate == node)
	{
	    if (((level == 1) && (f->f_flags & FET_ON1))
		|| ((level == 0) && (f->f_flags & FET_ON0)))
		f->f_flags |= FET_FORCEDON;
	    else if (f->f_flags & (FET_ON1|FET_ON0))
		f->f_flags |= FET_FORCEDOFF;
	    else continue;
	    if (f->f_flags & FET_FLOWFROMDRAIN)
		checkNode(f->f_source);
	    if (f->f_flags & FET_FLOWFROMSOURCE)
		checkNode(f->f_drain);
	}
    }
}


checkNode(node)
Node *node;			/* Node that may be forced to a value. */

/*---------------------------------------------------------
 *	This local procedure checks to see if a node is
 *	now forced to a value.  If so, the node is set
 *	to that value.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The node may be set to a 0 or 1 value.  This procedure
 *	proceeds in two stages.  First, it finds the strongest
 *	certain source of 0 or 1 (i.e. path to 0 or 1 through
 *	transistors that are definitely turned on).  Then it
 *	finds the strongest potential source of the opposite
 *	value (i.e. path to that value through transistors that
 *	aren't definitely turned off).  If the certain source
 *	is stronger than any conflicting potential source, then
 *	the node value is set.  If in the process of all this
 *	checking we discover that there are no transistors that
 *	can source information to the node, then the procedure
 *	is called recursively on all nodes that this node can
 *	source (this handles the case where the bottom transistor
 *	of a NAND gate turns off).
 *---------------------------------------------------------
 */

{
    register Pointer *p;
    register FET *f;
    register Node *n2;
    int strength, value;

    MarkSimSearch += 1;

    /* Find the strongest certain source of a 0 or 1. */

    strength = 0;
    for (p = node->n_pointer;  p != NULL;  p = p->p_next)
    {
	f = p->p_fet;
	if (!(f->f_flags & (FET_FORCEDON|FET_ONALWAYS))) continue;
	if (f->f_source == node)
	{
	    n2 = f->f_drain;
	    if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	}
	else if (f->f_drain == node)
	{
	    n2 = f->f_source;
	    if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	}
	else continue;
	if (n2->n_flags & NODE0ALWAYS)
	{
	    if (TypeTable[f->f_typeIndex].t_strengthLo > strength)
	    {
		strength = TypeTable[f->f_typeIndex].t_strengthLo;
		value = 0;
	    }
	}
	else
	{
	    if (TypeTable[f->f_typeIndex].t_strengthHi > strength)
	    {
		strength = TypeTable[f->f_typeIndex].t_strengthHi;
		value = 1;
	    }
	}
    }

    /* If there is a certain signal source, then see if there is a
     * potential source of the opposite signal that is at least as
     * strong as the certain source.  If not, then it's OK to set
     * the node's value.
     */
    
    MarkFSCount = 0;
    if (strength > 0)
    {
        if (strength > findStrength(node, !value, strength-1))
	    MarkNodeLevel(node, value, FALSE);
    }

    /* There's no certain signal source.  If there isn't
     * even a possible source for the node, then mark
     * all the transistors that flow away from this node
     * as disconnected and call ourselves recursively to
     * handle the nodes on the other side.
     */
    
    else if ((findStrength(node, 0, 0) == 0)
	&& (findStrength(node, 1, 0) == 0))
    {
	for (p = node->n_pointer;  p != NULL;  p = p->p_next)
	{
	    f = p->p_fet;
	    if (f->f_source == node)
	    {
		if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
		f->f_flags &= ~FET_FLOWFROMSOURCE;
		checkNode(f->f_drain);
	    }
	    else if (f->f_drain == node)
	    {
		if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
		f->f_flags &= ~FET_FLOWFROMDRAIN;
		checkNode(f->f_source);
	    }
	}
    }
}


int
findStrength(node, value, beatThis)
register Node *node;		/* Node from which to start search. */
int value;			/* 0 means look for GND source, 1 means
				 * look for Vdd source.
				 */
int beatThis;			/* Consider only paths stronger than this. */

/*---------------------------------------------------------
 *	FindStrength looks for the strongest source of a zero
 *	or one signal in a (potentially) connected piece of circuit.
 *
 *	Results:
 *	The return value is the strength of the strongest path
 *	between node and a source of value.  Only paths stronger
 *	than beatThis are considered.  If no stronger path is found,
 *	then beatThis is returned.  If a negative value is returned,
 *	it means that we looked and looked and eventually gave up
 *	without finishing all the possibilities.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    register Pointer *p;
    register FET *f;
    register Node *n2;
    int strength, strength2;

    MarkSimSearch += 1;

    /* Abort gracefully if we can't finish the search with
     * a reasonable effort.
     */
    
    if (++MarkFSCount >= MarkFSLimit)
    {
	printf("Aborting simulation at %s:\n", PrintNode(node));
	printf("    It's taking too long to find all the potential\n");
	printf("    signal sources.  You probably need to add more\n");
	printf("    flow control info.  A listing of nodes in the\n");
	printf("    area follows:\n");
	return -1;
    }

    /* Since this procedure does a recursive search, mark the
     * nodes that are already pending so we don't search forever.
     */
    
    node->n_flags |= NODEINPATH;

    /* Check all the transistors whose sources or drains connect
     * to this node to see if they could potentially provide a
     * strong enough source of the right value.  This may involve
     * recursive calls.
     */
    
    for (p = node->n_pointer;  p != NULL;  p = p->p_next)
    {
	f = p->p_fet;
	if (f->f_flags & FET_FORCEDOFF) continue;
	if (value == 0) 
	    strength = TypeTable[f->f_typeIndex].t_strengthLo;
	else strength = TypeTable[f->f_typeIndex].t_strengthHi;
	if (strength <= beatThis) continue;
	if (f->f_source == node)
	{
	    n2 = f->f_drain;
	    if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	}
	else if (f->f_drain == node)
	{
	    n2 = f->f_source;
	    if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	}
	else continue;
	if (f->f_fp != NULL) if (!FlowLock(f, n2)) continue;

	/* If an adjacent node is fixed in value, or is an input,
	 * we know the strength right away:  it's the strength of
	 * the connecting transistor.  Otherwise, we have to find
	 * the weakest transistor in the path to a driven node.
	 * Be sure to consider only nodes driven to the right value.
	 * Note: we don't count busses the same way as inputs here,
	 * they don't necessarily provide a static source of charge.
	 */

	if (n2->n_flags & (NODE1ALWAYS|NODE0ALWAYS|NODEISINPUT))
	{
	    if (value == 0)
	    {
		if (n2->n_flags & NODE1ALWAYS) goto endLoop;
	    }
	    else
	    {
		if (n2->n_flags & NODE0ALWAYS) goto endLoop;
	    }
	}
	else
	{
	    if (n2->n_flags & NODEINPATH) goto endLoop;
	    strength2 = findStrength(n2, value, beatThis);
	    if (strength2 < 0)
	    {
		printf("    %s\n", PrintNode(node));
		node->n_flags &= ~NODEINPATH;
		if (f->f_fp != NULL) FlowUnlock(f, n2);
		return -1;
	    }
	    if (strength2 < strength) strength = strength2;
	}
	if (strength > beatThis) beatThis = strength;
	endLoop: if (f->f_fp != NULL) FlowUnlock(f, n2);
    }

    node->n_flags &= ~NODEINPATH;
    return beatThis;
}


MarkFlow()

/*---------------------------------------------------------
 *	This procedure scans the network structure, marking
 *	the direction of flow in transistors.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	FET_FLOWFROMSOURCE and FET_FLOWFROMDRAIN flags get
 *	set in transistors wherever it appears possible for
 *	information to flow in the given direction.  "Possible"
 *	means that there is a source of 0 or 1 on the "from"
 *	side and a gate or output on the "to" side of the
 *	transistor, and	there is no conflicting flow attribute.
 *	There's an additional kludge in this routine: we turn
 *	NENH transistors into NENHP if they don't attach to any
 *	loads and aren't inputs.  It doesn't exactly belong here,
 *	but I can't think of a better place for it.
 *---------------------------------------------------------
 */

{
    register Node *n;
    register Pointer *p;
    register FET *f;
    int gotLoad, gotGate;

    /* Go through every node in the hash table.  If the node
     * has a gate attached or is an output, then invoke checkFlowTo
     * to see if the node can be driven and set flow bits.  Note:
     * don't do any checks from input nodes (including Vdd and Ground),
     * unless the nodes are also output nodes.  Also, ignore gates
     * on load transistors.
     */
    
    for (n = FirstNode; n != NULL; n = n->n_next)
    {
	gotGate = FALSE;
	gotLoad = FALSE;
	for (p = n->n_pointer;  p != NULL;  p = p->p_next)
	{
	    f = p->p_fet;
	    if ((f->f_gate == n) && (f->f_source != n)
		&& (f->f_drain != n)) gotGate = TRUE;
	    if ((f->f_typeIndex == FET_NLOAD)
		|| (f->f_typeIndex == FET_NSUPER)) gotLoad = TRUE;
	}
	for (p = n->n_pointer; p != NULL; p = p->p_next)
	{
	    f = p->p_fet;
	    if (f->f_gate != n) continue;
	    if (gotLoad  || (n->n_flags & NODEISINPUT))
	    {
		if (f->f_typeIndex == FET_NENHP) f->f_typeIndex = FET_NENH;
	    }
	    else if (f->f_typeIndex == FET_NENH) f->f_typeIndex = FET_NENHP;
	}
	if (n->n_flags & NODEISOUTPUT)
	{
	    checkFlowTo(n);
	    continue;
	}
	if (n->n_flags & NODEISINPUT) continue;
	if (gotGate) checkFlowTo(n);
    }
}


int
checkFlowTo(node)
Node *node;

/*---------------------------------------------------------
 *	checkFlowTo is a recursive local procedure used in
 *	setting transistor flow bits.
 *
 *	Results:
 *	The return value is TRUE if the given node can be
 *	driven to either 0 or 1.
 *
 *	Side Effects:
 *	If a valid source of 0 or 1 is found, transistors
 *	from that source to node are marked to show the
 *	potential flow of information.
 *---------------------------------------------------------
 */

{
    register Pointer *p;
    register FET *f;
    int result, flag;
    Node *next;

    if (FlowPrintAll || (FlowPrint && isalpha(node->n_name[0])))
        printf("Checking flow on node %d: %s.\n", MarkFlowSearch,
	    PrintNode(node));

    MarkFlowSearch += 1;

    /* Set a flag in the node so we don't get into infinite
     * recursion through circular structures.
     */
    
    node->n_flags |= NODEINPATH;

    result = FALSE;
    for (p = node->n_pointer;  p != NULL;  p = p->p_next)
    {
	f = p->p_fet;

	/* Make sure the node hooks to the source or drain, and
	 * that we haven't already marked the flow in this
	 * transistor.  If there's an attribute conflicting
	 * with the flow through this transistor, just skip
	 * the transistor.
	 */
	
	if (f->f_source == node)
	{
	    next = f->f_drain;
	    flag = FET_FLOWFROMDRAIN;
	    if (f->f_flags & FET_NODRAININFO) continue;
	}
	else if (f->f_drain == node) 
	{
	    next = f->f_source;
	    flag = FET_FLOWFROMSOURCE;
	    if (f->f_flags & FET_NOSOURCEINFO) continue;
	}
	else continue;

	/* Make sure to ignore circular paths before considering
	 * this transistor.
	 */

	if (next->n_flags & NODEINPATH) continue;

	if (f->f_flags & flag)
	{
	    result = TRUE;
	    continue;
	}

	/* If the node on the other side of the fet is an input,
	 * Vdd, or Ground, we can set the flow in the FET right
	 * away.  Otherwise we have to call ourselves recursively
	 * to find out if that node connects to anything.
	 */
	
	if (next->n_flags & NODEISINPUT)
	{
	    f->f_flags |= flag;
	    result = TRUE;
	    continue;
	}
	if (checkFlowTo(next))
	{
	    f->f_flags |= flag;
	    result = TRUE;
	}
    }

    node->n_flags &= ~NODEINPATH;
    return result;
}
	    

/*---------------------------------------------------------
 *	Procedures on this page are used to set various flag
 *	bits in nodes.  Each procedure expands node names and
 *	then sets one or more flag bits.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The string parameter contains one or more node names
 *	separated by white space.  Each name is expanded and
 *	the node(s) gets flag bits set.
 *---------------------------------------------------------
 */

markNodeFlags(string, flags)
char *string;		/* One or more names of node to be marked. */
int flags;		/* Value to be OR'ed into node flags. */

{
    Node *n;
    char *p;

    while ((p = NextField(&string)) != NULL)
    {
	ExpandInit(p);
	while ((n = (Node *) ExpandNext(&NodeTable)) != (Node *) NULL)
	    n->n_flags |= flags;
    }
}

MarkBlock(string)
char *string;
{
    markNodeFlags(string, NODEISBLOCKED);
}

MarkInput(string)
char *string;
{
    markNodeFlags(string, NODEISINPUT);
}

MarkOutput(string)
char *string;
{
    markNodeFlags(string, NODEISOUTPUT);
}

MarkBus(string)
char *string;
{
    markNodeFlags(string, NODEISBUS);
}

MarkPrecharged(string)
char *string;
{
    markNodeFlags(string, NODEISPRECHARGED);
}

MarkPredischarged(string)
char *string;
{
    markNodeFlags(string, NODEISPREDISCHARGED);
}

MarkWatched(string)
char *string;
{
    markNodeFlags(string, NODEISWATCHED);
}


MarkFromString(string)
char *string;		/* String containing 0/1 value and node names. */

/*---------------------------------------------------------
 *	This procedure decodes a string to fix a node level.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	String contains a level (0 or 1) followed by one or
 *	more node names.  The node names are expanded, and all the
 *	nodes whose names match are set to be forced to the
 *	indicated level.  The indicated nodes are also marked
 *	as busses.
 *---------------------------------------------------------
 */

{
    char *p;
    int value;
    Node *n;

    p = NextField(&string);
    if ((p == NULL) || (sscanf(p, "%d", &value) != 1))
    {
	printf("No value given:  try 0 or 1.\n");
	return;
    }
    if ((value != 0) && (value != 1))
    {
	printf("Bad value given:  try 0 or 1.\n");
	return;
    }

    while ((p = NextField(&string)) != NULL)
    {
	ExpandInit(p);
	while ((n = (Node *) ExpandNext(&NodeTable)) != (Node *) NULL)
	{
	    n->n_flags |= NODEISBUS;
	    MarkNodeLevel(n, value, TRUE);
	}
    }
}


MarkDynamic()

/*---------------------------------------------------------
 *	This procedure finds and marks dynamic nodes.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	NODEISDYNAMIC flags are set in nodes.  A node is marked
 *	dynamic if a) all transistors connected to it are forced
 *	off;  b) it isn't an input node;  and c) it isn't forced
 *	to a particular value.
 *---------------------------------------------------------
 */

{
    Node *n;
    Pointer *p;
    FET *f;

    for (n = FirstNode; n != NULL; n = n->n_next)
    {
	if (n == 0) continue;
	n->n_flags &= ~NODEISDYNAMIC;
	if (n->n_flags & (NODEISINPUT|NODE0ALWAYS|NODE1ALWAYS)) continue;
	for (p = n->n_pointer;  p != NULL;  p = p->p_next)
	{
	    f = p->p_fet;
	    if (f->f_source == n)
	    {
		if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	    }
	    else if (f->f_drain == n)
	    {
		if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	    }
	    else continue;
	    if (!(f->f_flags & FET_FORCEDOFF)) goto nextNode;
	}
	n->n_flags |= NODEISDYNAMIC;
	MarkDynamicCount += 1;
	if (MarkPrintDynamic) printf("%s is dynamic.\n", PrintNode(n));
	nextNode: continue;
    }
}


MarkStats()

/*---------------------------------------------------------
 *	This procedure prints out the saved up statistics.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Statistics info is printed on standard output.
 *---------------------------------------------------------
 */

{
    printf("Number of nodes set to 0: %d.\n", Mark0Nodes);
    printf("Number of nodes set to 1: %d.\n", Mark1Nodes);
    printf("Number of dynamic memory nodes: %d.\n", MarkDynamicCount);
    printf("Number of simulation search calls: %d.\n", MarkSimSearch);
    printf("Number of flow search calls: %d.\n", MarkFlowSearch);
}
