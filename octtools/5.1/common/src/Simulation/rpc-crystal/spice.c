/* spice.c -
 *
 * Copyright (C) 1983 by John K. Ousterhout
 * sccsid "@(#)spice.c	2.13 (Berkeley) 5/20/84"
 *
 * This file contains code to generate a SPICE deck for the
 * critical path through a circuit.
 */

#include "crystal.h"
#include "hash.h"

/* Imports from other Crystal modules: */

extern char *PrintNode();
extern Node *VddNode, *GroundNode;
extern HashTable NodeTable;
extern Type TypeTable[];
extern float Vdd, DelayBusThreshold;

/* The following constants define the minimum noticeable
 * resistance and capacitance values for nodes.  Below this
 * and we assume zero (this is intended to speed up SPICE).
 */

float SpiceMinR = 100.0;		/* In ohms. */
float SpiceMinC = .001;			/* In pfs. */

/* During the first part of SPICE deck generation, all the relevant
 * transistors and nodes are collected.  The transistors are recorded
 * in an array, and the nodes in a hash table.  In addition, two arrays
 * are used to record the initial value of every node, so that we can
 * output this information to SPICE.
 */

#define SPICEMAXFETS 500
static FET *(spiceFetTable[SPICEMAXFETS]);
static Node *sourceNode[SPICEMAXFETS];		/* Records, for each FET,
						 * the node that is source
						 * of info for that FET.
						 */
static int nSpiceFets;
static HashTable spiceNodeTable;
static nodeNum;

#define SPICEMAXNODES 500
static float initValueTable[SPICEMAXNODES];	/* SPICE initial values. */

/* The following is sort of a hack.  It keeps track of the nodes
 * that are driven from transistors that are output.  Nodes that
 * can't be driven (busses and inputs that form the voltage source
 * for stages) are replaced in the SPICE output with Vdd or Ground.
 */

static int hasDriver[SPICEMAXNODES];		/* TRUE means we'll output
						 * a transistor to drive node.
						 */

/* The SPICE deck file needs to be used everywhere, so it is kept
 * global.
 */

static FILE *spiceFile;

/* The following array defines the legal simulation step times in ns. */

static float tranTimes[] =
{.01, .02, .05, .1, .2, .5, 1, 2, 5, 10, 20, 50,
100, 200, 500, 1000, 2000, 5000, -1};


saveNode(node, initValue)
Node *node;			/* Node to be remembered for later. */
float initValue;			/* Initial value of node. */

/*---------------------------------------------------------
 *	This routine just remembers a node for later.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Node is recorded in spiceNodeTable for outputting during
 *	the second pass of SPICE deck generation.  The node is
 *	assigned a unique even number in the hash table.  If the
 *	node is fixed in value and is a chip input, then there's
 *	no need to record it, since we'll tie to Vdd or GND later.
 *	A comment line is output in the SPICE file for each node
 *	to tell what signal corresponds to each number.  We also
 *	save the initial values for nodes in a different table.
 *---------------------------------------------------------
 */

{
    HashEntry *h;
    int i;

    if ((node->n_flags & NODEISINPUT)
	&& (node->n_flags & (NODE1ALWAYS|NODE0ALWAYS))) return;
    h = HashFind(&spiceNodeTable, node->n_name);
    i = (int) HashGetValue(h);
    if (i == 0)
    {
	HashSetValue(h, nodeNum);
	fprintf(spiceFile, "* Nodes %d-%d correspond to %s\n",
	    nodeNum, nodeNum+1, PrintNode(node));
	if (nodeNum >= SPICEMAXNODES-1)
	    printf("Crystal bug:  need to enlarge SPICE node table.\n");
	else
	{
	    initValueTable[nodeNum] = initValue;
	    initValueTable[nodeNum+1] = initValue;
	}
	nodeNum += 2;
    }
}


saveFET(fet, source)
FET *fet;			/* Transistor to be remembered. */
Node *source;			/* Node from which information flow into FET */

/*---------------------------------------------------------
 *	This procedure remembers a transistor for later.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The parameter transistor is stored in a table of transistors
 *	to be output to the SPICE deck during the second pass.  Also,
 *	the SPICE flag is set in the transistor (this flag is used
 *	to adjust node capacitances later on).
 *---------------------------------------------------------
 */

{
    if (fet == NULL) return;
    if (fet->f_flags & FET_SPICE)
    {
	printf("Crystal bug:  transistor has SPICE flag set.\n");
	return;
    }
    if (nSpiceFets >= SPICEMAXFETS-1)
    {
	printf("Crystal bug:  need to enlarge SPICE transistor table.\n");
	return;
    }
    fet->f_flags |= FET_SPICE;
    spiceFetTable[nSpiceFets] = fet;
    sourceNode[nSpiceFets] = source;
    nSpiceFets += 1;
}


int getSpiceNum(node)
Node *node;

/*---------------------------------------------------------
 *	This routine finds the SPICE number for a node,
 *	if any.
 *
 *	Results:
 *	If the node is in the SPICE node table, and is driven
 *	by a transistor, then its SPICE number is returned.
 *	If the node isn't driven, then the Vdd or Ground node
 *	number is returned, depending on the node's initial
 *	value.  If the node isn't in the table, -1 is returned.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    HashEntry *h;
    int result;

    if (node->n_flags & NODEISINPUT)
    {
	if (node->n_flags & NODE1ALWAYS) return 1;
	if (node->n_flags & NODE0ALWAYS) return 0;
    }
    h = HashFind(&spiceNodeTable, node->n_name);
    result = (int) HashGetValue(h);
    if (result == 0) return -1;
    if (!hasDriver[result])
    {
	if (initValueTable[result] > 0) return 1;
	else return 0;
    }
    return result;
}


int
findOppPath(from, trigger, level, flags)
Node *from;			/* Starting node for search. */
Node *trigger;			/* Node that must be gate of at least one
				 * transistor along path.
				 */
int level;			/* NODE1ALWAYS or NODE0ALWAYS:  indicates
				 * what level we want a source of.
				 */
int flags;			/* Flags that must be set in the fet whose
				 * gate is trigger.
				 */

/*---------------------------------------------------------
 *	This procedure is used to find an opposing path to
 *	one recorded in a stage.  This is used in two ways:
 *	a) when a load device is going to pull up a node,
 *	this procedure finds the path that initially pulls
 *	the node to ground (and which eventually gets turned
 *	off);  b) in CMOS, the stage records the path that
 *	eventually drives the output one way, and this procedure
 *	finds the transistors that initially drive the node
 *	the other way.
 *
 *	Results:
 *	TRUE is returned if a path was found from "from" to
 *	a source of "level", passing through a FET gated by
 *	"trigger" with "flags" set.  Note:  if trigger is NULL,
 *	then that level of qualification is ignored.
 *
 *	Side Effects:
 *	If TRUE is returned, then each of the nodes and transistors
 *	from "from" to the source of "level" is saved (not including
 *	"from" itself).
 *---------------------------------------------------------
 */

{
    Pointer *p;
    FET *f;
    Node *next, *nextTrigger;
    int ok, wrongLevel;

    /* Watch out (as always) for circular paths. */

    from->n_flags |= NODEINPATH;

    /* Chase through all transistors attached to the starting node. */

    if (level == NODE1ALWAYS) wrongLevel = NODE0ALWAYS;
    else wrongLevel = NODE1ALWAYS;
    for (p = from->n_pointer; p != NULL; p = p->p_next)
    {
	f = p->p_fet;

	/* Make sure information can flow in this direction. */

	if (f->f_source == from)
	{
	    next = f->f_drain;
	    if (!(f->f_flags & FET_FLOWFROMDRAIN)) continue;
	}
	else if (f->f_drain == from)
	{
	    next = f->f_source;
	    if (!(f->f_flags & FET_FLOWFROMSOURCE)) continue;
	}
	else continue;
	if (next->n_flags & NODEINPATH) continue;
	if (next->n_flags & wrongLevel) continue;
	if (f->f_flags & FET_FORCEDOFF) continue;
	if (f->f_fp != NULL) if (!FlowLock(f, next)) continue;

	/* See if we've found the trigger transistor. */

	if ((f->f_gate == trigger)
	    && ((f->f_flags & flags) == flags)) nextTrigger = NULL;
	else nextTrigger = trigger;

	/* See if we've found the end of the path.  Otherwise, call
	 * ourselves recursively to find the remaining piece of the
	 * path.  Note:  there's no point in looking past a bus or
	 * input, since they are both sources of signal.
	 */

	ok = FALSE;
	if ((next->n_flags & level) && (nextTrigger == NULL)) ok = TRUE;
	if ((next->n_flags &
	    (NODEISINPUT|NODEISBUS|NODEISPRECHARGED|NODEISPREDISCHARGED)) ||
	    (next->n_C >= DelayBusThreshold))
	{
	    if (nextTrigger == NULL) ok = TRUE;
	    else goto skipThis;
	}
	if (!ok) ok = findOppPath(next, nextTrigger, level, flags);
	skipThis: if (f->f_fp != NULL) FlowUnlock(f, next);
	if (ok) goto gotPath;
    }
    from->n_flags &= ~NODEINPATH;
    return FALSE;

    /* At this point, we've found an OK path, so save the transistor
     * and the following node.  Note:  DON'T save the gate of the
     * transistor:  unless this node is saved for some other reason,
     * we'll just tie it to a power rail in the deck.
     */
    
    gotPath: saveFET(f, next);
    if (level == NODE1ALWAYS) saveNode(next, Vdd);
    else saveNode(next, (float) 0.0);
    from->n_flags &= ~NODEINPATH;
    return TRUE;
}


SpiceDeck(file, stage)
FILE *file;		/* Where to put SPICE information. */
Stage *stage;		/* Linked list of stages describing path */

/*---------------------------------------------------------
 *	This routine generates a SPICE deck corresponding
 *	to a given path through the circuit.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	A SPICE deck is written in file.  It includes circuit
 *	cards, and cards to invoke an analysis and plot results.
 *	The deck does NOT include device model cards.  Node 0 is
 *	GND, node 1 is Vdd, and node 2 is body bias.
 *
 *	Approximations:
 *	Side paths are ignored, just like in the rest of Crystal.
 *	Each node is approximated by a lumped resistance and
 *	capacitance, and the resistance is made to flow through
 *	all of the capacitance.
 *
 *	Design:
 *	This routine runs in two passes.  In the first pass, we scan
 *	through the list of stages to locate all the nodes and fets
 *	that will have to output into the SPICE deck.  In the second
 *	pass, all the saved up information is output (it turns out
 *	that we need to know everything that's going to be output
 *	before we can actually output any of it).
 *---------------------------------------------------------
 */

{
    Stage *s;
    HashSearch hs;
    HashEntry *h;
    int i, j, capNum, resNum, fetNum, overallDriver;
    float initValue;
    Pointer *p;
    FET *f;
    double dbl;
    Node *node;

    if (stage == NULL) return;

    /* Save all sorts of stuff around in static variables where other
     * procedures can get at it.
     */

    spiceFile = file;
    nSpiceFets = 0;
    nodeNum = 4;
    HashInit(&spiceNodeTable, 16);
    capNum = 1;
    resNum = 1;
    fetNum = 1;

    /* Pass 1:  go through each stage in the path and save away
     * all the fets and nodes along each stage.
     */
    
    for (s = stage;  s != NULL;  s = s->st_prev)
    {
	if (s->st_rise) initValue = 0;
	else initValue = Vdd;
	saveNode(s->st_piece2Node[s->st_piece2Size-1], initValue);
	for (i = s->st_piece2Size-2; i>=0; i-=1)
	{
	    saveNode(s->st_piece2Node[i], initValue);
	    saveFET(s->st_piece2FET[i], s->st_piece2Node[i]);
	}
	for (i = s->st_piece1Size-1; i>=0; i-=1)
	{
	    saveNode(s->st_piece1Node[i], Vdd-initValue);
	    saveFET(s->st_piece1FET[i], s->st_piece1Node[i]);
	}

	/* Now we've accounted for the piece of the stage that will
	 * drive the stage to its final value.  However, it's also
	 * important to find and output the piece of circuit that
	 * drives the stage to its initial value.  We do this by
	 * scanning along the stage's piece2 and checking for loads
	 * and/or calling findOppPath.  The only time this isn't
	 * necessary is when there is no piece1 and the first node
	 * in piece2 isn't fixed at a value.
	 */
	
	if ((s->st_piece1Size == 0) &&
	    !(s->st_piece2Node[0]->n_flags & (NODE1ALWAYS|NODE0ALWAYS)))
	    continue;
	
	for (i=0;  i<s->st_piece2Size; i+=1)
	{
	    Node *other, *trigger;
	    Stage *s2;
	    int level;

	    /* First, check all the transistors attached to this
	     * node for a load that can drive it.  If we find one,
	     * then we're done.  Warning:  sometimes the first node
	     * in piece2 is fixed in value (e.g. when an nMOS load
	     * is pulling up the target).  Be sure to skip such nodes.
	     */

	    node = s->st_piece2Node[i];
	    if (node->n_flags & (NODE1ALWAYS|NODE0ALWAYS)) continue;
	    for (p = node->n_pointer;  p != NULL;  p = p->p_next)
	    {
		f = p->p_fet;
		if (!(f->f_flags & (FET_FORCEDON|FET_ONALWAYS))) continue;
		if (f->f_source == node) other = f->f_drain;
		else if (f->f_drain == node) other = f->f_source;
		else continue;
		if (s->st_rise)
		{
		    if (!(other->n_flags & NODE0ALWAYS)) continue;
		    saveNode(other, (float) 0.0);
		}
		else
		{
	    	    if (!(other->n_flags & NODE1ALWAYS)) continue;
		    saveNode(other, Vdd);
		}
		saveFET(f, other);
		goto gotOpp;
	    }

	    /* No load in sight.  Now try for a path that forces this
	     * stage to its initial value when the trigger for the stage
	     * has its initial value.
	     */
	    
	    if (s->st_rise) level = NODE0ALWAYS;
	    else level = NODE1ALWAYS;
	    s2 = s->st_prev;
	    if (s2 == NULL)
	    {
		printf("Crystal bug: previous stage is NULL.\n");
		continue;
	    }
	    trigger = s2->st_piece2Node[s2->st_piece2Size-1];
	    if (s2->st_rise)
	    {
		if (findOppPath(node, trigger, level, FET_ON0)) goto gotOpp;
	    }
	    else if (findOppPath(node, trigger, level, FET_ON1)) goto gotOpp;
	}
    gotOpp: continue;
    }

    /* Pass 1.5:  This is where we figure out which nodes are actually
     * driven by transistors, and which are busses and inputs that
     * provide the voltage source for stages but aren't directly
     * driven.
     */
    
    for (i=0;  i<=nodeNum;  i++) hasDriver[i] = FALSE;
    for (i=0;  i<nSpiceFets;  i++)
    {
	f = spiceFetTable[i];
	if (f->f_source == sourceNode[i]) node = f->f_drain;
	else node = f->f_source;
	j = (int) HashGetValue(HashFind(&spiceNodeTable, node->n_name));
	if (j == 0) continue;
	hasDriver[j] = TRUE;
	hasDriver[j|1] = TRUE;
    }

    /* The node in the first stage is automatically driven, since it
     * is the input.  Also, we only use the odd node of the pair (ignore
     * resistance or capacitance on the input).
     */
    
    for (s = stage;  s->st_prev != NULL;  s = s->st_prev) /*loop*/;
    node = s->st_piece2Node[s->st_piece2Size-1];
    h = HashFind(&spiceNodeTable, node->n_name);
    overallDriver = ((int) HashGetValue(h)) | 1;
    HashSetValue(h, overallDriver);
    hasDriver[overallDriver] = TRUE;

    /* Pass 2: we've got all the information, it's just a matter of
     * printing it out in SPICE format (simple, huh?).  First, print
     * out all the nodes.  Note:  each node is really going to be
     * 2 nodes:  the odd one is the one that the node's capacitance
     * attaches to.  The odd one is connected to the even one by a
     * resistor;  transistors that supply information to the node are
     * on the even side.  If the node has a very small internal resistance,
     * then we don't output any resistance and just use the odd node
     * for everything.  If the node has very little capacitance, don't
     * output any at all.  There's another little thing to worry about.
     * SPICE computes its own channel and gate-terminal capacitances
     * for transistors, whereas Crystal lumps all this onto the relevant
     * node capacitances.  Therefore, we have to subtract out such node
     * capacitances for any transistors that we are going to output in
     * the SPICE deck (this is a little bit tricky... the code here is
     * almost the exact inverse of the code in build.c).
     */
    
    fprintf(spiceFile, "\n");
    HashStartSearch(&hs);
    while ((h = HashNext(&spiceNodeTable, &hs)) != NULL)
    {
	float c;

	node = (Node *) HashGetValue(HashFind(&NodeTable, h->h_name));
	i = (int) HashGetValue(h);
	if (!hasDriver[i]) continue;
	if (i == overallDriver) continue;   /* No R or C for overall driver. */
	if (node->n_R < SpiceMinR)
	{
	    i |= 1;
	    HashSetValue(h, i);
	}
	else fprintf(spiceFile, "R%d %d %d %.0f\n",
	    resNum++, i, i|1, node->n_R);

	c = node->n_C;
	for (p = node->n_pointer;  p != NULL; p = p->p_next)
	{
	    f = p->p_fet;
	    if (!(f->f_flags & FET_SPICE)) continue;
	    if ((f->f_source == node) && (f->f_gate != node))
	    {
		dbl = f->f_area/f->f_aspect;
		c -= sqrt(dbl)*TypeTable[f->f_typeIndex].t_cPerWidth;
	    }
	    if ((f->f_drain == node) && (f->f_gate != node))
	    {
		dbl = f->f_area/f->f_aspect;
		c -= sqrt(dbl)*TypeTable[f->f_typeIndex].t_cPerWidth;
	    }
	    if (f->f_gate == node)
	    {
		if ((f->f_source != node) && (f->f_drain != node))
		    c -= f->f_area*TypeTable[f->f_typeIndex].t_cPerArea;
		if (f->f_source != node)
		{
		    dbl = f->f_area/f->f_aspect;
		    c -= sqrt(dbl)*TypeTable[f->f_typeIndex].t_cPerWidth;
		}
		if (f->f_drain != node)
		{
		    dbl = f->f_area/f->f_aspect;
		    c -= sqrt(dbl)*TypeTable[f->f_typeIndex].t_cPerWidth;
		}
	    }
	}
	if (c > SpiceMinC)
	    fprintf(spiceFile, "C%d %d 0 %.3fpf\n", capNum++, i|1, c);
	
	/* Note:  for the output node, we always generate at least
	 * a teensy-tiny little capacitor to make sure the node has
	 * at least two connections.  Otherwise, SPICE barfs.
	 */

	else if ((i|1) == 5)
	    fprintf(spiceFile, "C%d 5 0 .00001pf\n", capNum++);
    }

    /* Now output the transistors.  There's a trick here, too.
     * All the sources and drains are in the node table, but not
     * necessarily all of the gates.  If the gate of a transistor
     * is in the node table, so much the better.  Otherwise, set
     * the gate to either Vdd or Ground, whichever it takes to
     * force the transistor on.  Also, use node 2 for body bias
     * for n-type transistors, node 3 for body bias for p-type
     * transistors.
     */
    
    for (i=0;  i<nSpiceFets;  i++)
    {
	int sNum, dNum, gNum;
	float length, width;

	f = spiceFetTable[i];
	dbl = f->f_area/f->f_aspect;
	width = sqrt(dbl);
	length = f->f_area/width;
	sNum = getSpiceNum(f->f_source);
	if (sNum < 0)
	    printf("Crystal bug: lost SPICE node!\n");
	else if ((f->f_source == sourceNode[i]) && (sNum > 3)) sNum |= 1;
	dNum = getSpiceNum(f->f_drain);
	if (dNum < 0)
	    printf("Crystal bug: lost SPICE node!\n");
	else if ((f->f_drain == sourceNode[i]) && (dNum > 3)) dNum |= 1;
	gNum = getSpiceNum(f->f_gate);
	if ((gNum > 3) && (f->f_gate != f->f_source)
	    && (f->f_gate != f->f_drain)) gNum |= 1;
	if (gNum < 0)
	{
	    if (f->f_flags & FET_ON0) gNum = 0;
	    else gNum = 1;
	}
        fprintf(spiceFile, "M%d %d %d %d %d %c l=%.1fu w=%.1fu\n",
	    fetNum++, dNum, gNum, sNum,
	    TypeTable[f->f_typeIndex].t_spiceBody,
	    TypeTable[f->f_typeIndex].t_spiceType,
	    length, width);
    }

    /* Set initial conditions for all nodes in the table that
     * are driven.
     */

    fprintf(spiceFile, "\n* Initial conditions:\n");
    HashStartSearch(&hs);
    while ((h = HashNext(&spiceNodeTable, &hs)) != NULL)
    {
	i = (int) HashGetValue(h);
	if (i == 0) continue;
	if (!hasDriver[i]) continue;
	if (i == overallDriver) continue;
	fprintf(spiceFile, ".ic v(%d)=%f\n", i, initValueTable[i]);
	if (!(i&1))
	    fprintf(spiceFile, ".ic v(%d)=%f\n", i|1, initValueTable[i|1]);
    }

    /* Output a card for Vdd.  Then find the node that drives the
     * whole path and generate transient analysis cards.  Guess at
     * a good limit and time step for the transient analysis (use
     * powers of ten such that 200 data points will be at least
     * twice as long as our estimate of delay).
     */

    fprintf(spiceFile, "\n");
    fprintf(spiceFile, "vdd 1 0 %.1f\n", Vdd);
    fprintf(spiceFile, "vin %d 0 pulse(", overallDriver);
    if (s->st_rise) fprintf(spiceFile, "0 5 0ns 0ns 0ns)\n");
    else fprintf(spiceFile, "5 0 0ns 0ns 0ns)\n");

    for (i=0; tranTimes[i] > 0; i++)
    {
	dbl = tranTimes[i];
	if ((150*dbl) > stage->st_time) break;
    }
    fprintf(spiceFile, ".tran %.2fns %.0fns\n", dbl, 200.0*dbl);

    i = getSpiceNum(stage->st_piece2Node[stage->st_piece2Size-1]);
    fprintf(spiceFile, ".plot tran V(%d) (0,5)\n", i|1);
    fprintf(spiceFile, ".end\n");

    /* Last but not least, clean up:  clear all the SPICE flags in
     * FETs, and delete the hash table.
     */
    
    for (i=0; i<nSpiceFets; i+=1)
	spiceFetTable[i]->f_flags &= ~FET_SPICE;
    HashKill(&spiceNodeTable);
}
