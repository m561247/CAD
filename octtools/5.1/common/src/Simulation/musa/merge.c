/***********************************************************************
 * Transistor merging
 * Filename: merge.c
 * Program: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Design: Russell Segal
 * Organization:  University Of California, Berkeley
 *                Electronic Research Laboratory
 *                Berkeley, CA 94720
 * Routines:
 *	void move_connect() - move connection.
 *	void remove_connect() - remove connection.
 *	static int16 merge_inst() - merge instance.
 *	static int16 check_node() - get other 'inst' for series merging.
 *	static int try_series() - try series merging.
 *	static int try_parallel() - try parallel merging.
 *	static int try_merge() - try to merge transistors.
 *	int mark_important() - mark important nodes.
 *	int unmark_node() - unmark nodes.
 *	void merge_trans() - merge transistors.
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define MERGE_MUSA
#include "musa.h"

static int num_tried = 0, num_nodes = 0, num_merged = 0;

/************************************************************************
 * MOVE_CONNECT
 * This ugly process patches the data structure when a
 * connect is moved in memory.  (The connect list from the node is
 * patched.)
 */
void move_connect(from, to, inst)
	connect	*from, *to;
	musaInstance	*inst;
{
	node	*theNode;
	connect	*p;

	*to = *from;
	to->device = inst;
	theNode = from->node;
	if (theNode->conns == from) {
		theNode->conns = to;
	} else {
		for (p = theNode->conns; p->next != from; p = p->next) {} 
		p->next = to;
	} /* if... */
} /* move_connect... */

/*************************************************************************
 * REMOVE_CONNECT
 * This removes a connect from a node's list
 */
void remove_connect(conn)
	connect	*conn;
{
	node	*theNode;
	connect	*p;

	theNode = conn->node;
	if (theNode->conns == conn) {
		theNode->conns = conn->next;
	} else {
		for (p = theNode->conns; p->next != conn; p = p->next) {}
		p->next = conn->next;
	} /* if... */
} /* remove_connect... */

/*************************************************************************
 * MERGE_INST
 * Merge two transistors together.
 *
 * Parameters:
 *	target (musaInstance *) - the target transistor. (Input)
 *	other (musaInstance *) - the transistor to be merged into 'target'. (Input)
 *	parallel (int16) - parallel or series merging. (Input)
 *	theNode (node *) - if series merging, the the node that connects
 *		the two transistors. (Input)
 *
 * Return Value:
 *	TRUE, merging successful.
 *	FALSE, merging unsuccessful.
 */
static int16 merge_inst(target, other, parallel, theNode)
	musaInstance	*target, *other;
	int16	parallel;
	node	*theNode;		/* the common node in a series merge */
{
	int		ttype, otype;	/* subtypes of target and other */
	musaInstance	*little, *big;
	int		ltype, btype;	/* subtypes of little and big */
	int		addtype;
	deviceType newtype;
	int		newsubtype;	/* what target will become */
	connect	*newconns;
	MusaElement	*e;
	int		term = 0;	/* connect we are to copy next */
	int		i;

	ttype = target->data & SUBTYPE;
	otype = other->data & SUBTYPE;
	if (ttype < otype) {
		little = target;
		ltype = ttype;
		big = other;
		btype = otype;
	} else {
		little = other;
		ltype = otype;
		big = target;
		btype = ttype;
	} /* if... */
	addtype = ltype - 3;
	if ((addtype < 0) || (addtype > 2)) {
		return(FALSE);
	} /* if... */
	if (parallel) {
		newsubtype = network[btype].paradd[addtype];
	} else {
		newsubtype = network[btype].seradd[addtype];
	} /* if... */
	if (newsubtype == -1) {
		return(FALSE);
	} /* if... */
	num_merged++;
	if (parallel) {
		newtype = target->type;
		if (MosTransistor(target->type)) {
			remove_connect(&(other->connArray[0]));
			remove_connect(&(other->connArray[1]));
			newconns = ConnAlloc(2 + network[newsubtype].numin);
			move_connect(&(target->connArray[0]), &(newconns[term++]),
				target);
			move_connect(&(target->connArray[1]), &(newconns[term++]),
				target);
		} else {
			remove_connect(&(other->connArray[0]));
			newconns = ConnAlloc(1 + network[newsubtype].numin);
			move_connect(&(target->connArray[0]), &(newconns[term++]),
				target);
		} /* if... */
	} else {
		if (!MosTransistor(target->type)) {
			newtype = target->type;
			remove_connect(&(target->connArray[0]));
			newconns = ConnAlloc(1 + network[newsubtype].numin);
		} else if (!MosTransistor(other->type)) {
			newtype = other->type;
			remove_connect(&(other->connArray[0]));
			newconns = ConnAlloc(1 + network[newsubtype].numin);
		} else {
			newtype = target->type;
			newconns = ConnAlloc(2 + network[newsubtype].numin);
		} /* if... */
		if (MosTransistor(target->type)) {
			if (target->connArray[0].node == theNode) {
				remove_connect(&(target->connArray[0]));
				move_connect(&(target->connArray[1]), &(newconns[term++]),
					target);
			} else {
				remove_connect(&(target->connArray[1]));
				move_connect(&(target->connArray[0]), &(newconns[term++]),
					target);
			} /* if... */
		} /* if... */
		if (MosTransistor(other->type)) {
			if (other->connArray[0].node == theNode) {
				remove_connect(&(other->connArray[0]));
				move_connect(&(other->connArray[1]), &(newconns[term++]),
					target);
			} else {
				remove_connect(&(other->connArray[1]));
				move_connect(&(other->connArray[0]), &(newconns[term++]),
					target);
			} /* if... */
		} /* if... */
		/* fix term type if target just became a pull network */
		if (!MosTransistor(newtype)) {
			newconns[0].ttype = TERM_OUTPUT;
		} /* if... */

		/* free up "theNode" and all elements that point to it */
		for (e = theNode->rep; e != NIL(MusaElement); e = NextEquiv(e, TRUE)) {}
		NodeFree(theNode);
	} /* if... */
	for (i=0; i<network[btype].numin; i++) {
		if (MosTransistor(big->type)) {
			move_connect(&(big->connArray[i+2]), &(newconns[term++]),
				target);
		} else {
		    move_connect(&(big->connArray[i+1]), &(newconns[term++]),
			    target);
		} /* if.... */
	} /* for... */
	for (i=0; i<network[ltype].numin; i++) {
		if (MosTransistor(little->type)) {
			move_connect(&(little->connArray[i+2]), &(newconns[term++]),
				target);
		} else {
			move_connect(&(little->connArray[i+1]), &(newconns[term++]),
				target);
		} /* if... */
	} /* for... */
	if (!MosTransistor(target->type)) {
		ConnFree(target->connArray, network[ttype].numin + 1);
	} else {
		ConnFree(target->connArray, network[ttype].numin + 2);
	} /* if... */
	if (!MosTransistor(other->type)) {
		ConnFree(other->connArray, network[otype].numin + 1);
	} else {
		ConnFree(other->connArray, network[otype].numin + 2);
	} /* if... */
	target->type = newtype;
	target->data &= ~SUBTYPE;
	target->data |= newsubtype;
	target->connArray = newconns;

	/* unlink and free "other" */
	InstUnlink(other);
	MusaInstanceFree(other);
	return(TRUE);
} /* merge_inst... */

/************************************************************************
 * CHECK_NODE
 * Get other inst for series merge.  Return false if merge won't
 * work.
 *
 * Parameters:
 *	inst (musaInstance *) - the known transistor. (Input)
 *	theNode (node *) - the node connecting the two transistors. (Input)
 *	other (musaInstance **) - the other transistor. (Output)
 *
 * Return Value:
 *	TRUE, 'other' transistor found.
 *	FALSE, 'other' transistor not found, merging not possible.
 */
static int16 check_node(inst, theNode, other)
	musaInstance	*inst;
	node	*theNode;
	musaInstance	**other;
{
	connect	*second;

	if ((theNode->data & NO_SERIES) != 0) {
		return(FALSE);
	} /* if... */
	second = theNode->conns->next;
	if ((second == NIL(connect)) || (second->next != NIL(connect))) {
		/* only two insts must be incident for series */
		return(FALSE);
	} /* if... */
	if (second->device == inst) {
		*other = theNode->conns->device;
	} else {
		*other = second->device;
	} /* if... */
	return(TRUE);
} /* check_node... */

/*************************************************************************
 * TRY_SERIES
 * try merge inst with series neighbor.  (Neighbor must be
 * same type and only inst sharing theNode.)
 * return true if success and there is a chance for parallel to succeed.
 *
 * Parameters:
 *	inst (musaInstance *) - the transistor to merge with neighbor. (Input)
 *
 * Return Value:
 *	TRUE, if success and parallel merging possible.
 */
static int try_series(inst)
	musaInstance	*inst;
{
	musaInstance	*other;
	node	*theNode;
	int		retval = 0;

	while (1) {		/* stay in this loop until there is a failure */
		if (!MosTransistor(inst->type)) {
			/* Pullup or pulldown network -- one terminal to check */
			theNode = inst->connArray[0].node;
			if (!check_node(inst, theNode, &other)) break;
			/* Don't merge two pull networks in series */
			if (!MosTransistor(other->type)) break;
			if (!merge_inst(inst, other, FALSE, theNode)) break;
			retval = 1;
		} else {
			/* Network with source and drain */
			/* fail only if both source and drain fail */
			theNode = inst->connArray[0].node;
			if (!check_node(inst, theNode, &other)) goto oppterm;
			if (merge_inst(inst, other, FALSE, theNode)) {
				retval = 1;
				continue;
			} /* if... */
oppterm:
			theNode = inst->connArray[1].node;
			if (!check_node(inst, theNode, &other)) break;
			if (!merge_inst(inst, other, FALSE, theNode)) break;
			retval = 1;
		} /* if... */
	} /* while... */
	return(retval && (network[inst->data & SUBTYPE].numin < 5));
} /* try_series... */

/***********************************************************************
 * TRY_PARALLEL
 * try merge merge inst with parallel neighbor.  (Neighbor must be
 * same type and share source and drain.)
 * return true if success and there is a chance for series to succeed.
 * When a parallel merge is successful, care must be taken to reset
 * the value of "*p" to the begin.  The linked list being traversed
 * has changed.
 * Usually terminal zero of inst is used to search for parallel
 * transistors.  If the node at term 0 is INEFFicient, try using
 * term 1 (on non-pull instances).
 *
 * Parameters:
 *	inst (musaInstance *) - the transistor to merge with neighbor. (Input)
 *
 * Return Value:
 *	TRUE, if success and series merging possible.
 */
static int try_parallel(inst)
	musaInstance	*inst;
{
	node	*theNode;
	connect	*p;
	musaInstance	*other;		/* inst that we are attempting to merge */
	node	*oppnode;	/* other incident node of "other" */
	int		connectnum;	/* terminal used for searching for parallel */
	int		retval = 0;

	if ((inst->connArray[0].node->data & INEFF_NODE) == 0) {
		theNode = inst->connArray[0].node;
		connectnum = 0;
	} else if (((inst->connArray[1].node->data & INEFF_NODE) == 0) &&
				(MosTransistor(inst->type))) {
		theNode = inst->connArray[1].node;
		connectnum = 1;
	} else {
		return(FALSE);
	} /* if... */

	do {	/* repeat this loop whenever there is a success */
		for (p = theNode->conns; p != NIL(connect); p = p->next) {
			if (p == &(inst->connArray[connectnum])) {
				continue;
			} /* if... */
			other = p->device;
			if (other->type != inst->type) {
				continue;
			} /* if... */
			if (other == inst) {
				continue;
			} /* if... */

			/* check if other inst's term is a SOURCE or DRAIN
			 * (or TERM_OUTPUT) */
			if (p->ttype != inst->connArray[connectnum].ttype) {
				continue;
			} /* if... */

			if (MosTransistor(inst->type)) {
				/* check if the opposite connection is common */
				if (&(other->connArray[0]) == p) {
					oppnode = other->connArray[1].node;
				} else {
					oppnode = other->connArray[0].node;
				} /* if... */
				if (inst->connArray[1 - connectnum].node != oppnode) {
					continue;
				} /* if... */
			} /* if... */

			/* all tests have been passed, attempt merge */
			if (merge_inst(inst, other, TRUE, NIL(node))) {
				retval = 1;
				break;		/* break out of "for" loop to get a safe
							 * value for "p" */
			} /* if... */
		} /* for... */
	} while (p != NIL(connect));
	return(retval && (network[inst->data & SUBTYPE].numin < 5));
} /* try_parallel... */

/************************************************************************
 * TRY_MERGE
 * If 'inst' is a transistor, then try to merge 'inst' with other transistors.
 *
 * Parameters:
 *	inst (musaInstance *) - the transistor to merge with neighbors. (Input)
 */
static int try_merge(inst)
	musaInstance *inst;
{
	int subtype;

	num_tried++;
	if (update && (num_tried/1000*1000 == num_tried)) {
		(void) fprintf(stderr, "%d ", num_tried);
		(void) fflush(stderr);
	} /* if... */
	if (!MosInstance(inst->type)) return;
	subtype = inst->data & SUBTYPE;
	if ((network[subtype].numin < 1) || network[subtype].numin > 4) return;

	(void) try_parallel(inst);
	while(try_series(inst) && try_parallel(inst)) { }
	/* the while loop is equivalent to --
			do {
				if (ssuccess = try_series(inst)) {
					psuccess = try_parallel(inst);
				} 
			} while (psuccess && ssuccess);
	*/
} /* try_merge... */

/***********************************************************************
 * MARK_IMPORTANT
 * This routine marks the nodes which SHOULD NOT be destroyed by a
 * series merge.  node->data |= IMPORTANT_NODE  => don't merge
 * This also marks nodes which are inefficient to attempt merging on
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 */
int mark_important(theNode)
	node			*theNode;
{
	connect			*p;
	deviceType		transtype;		/* type of transistor incident here */
	int16			gottype = FALSE;
	int				conncount = 0;	/* how many connections to node */

	num_nodes++;
	if (update && (num_nodes/1000*1000 == num_nodes)) {
		(void) fprintf(stderr, "%d ", num_nodes);
		(void) fflush(stderr);
	} /* if... */
	if ((theNode->data & IMPORTANT_NODE) != 0) return;
	if (IsSupply(theNode) || IsGround(theNode)) {
		theNode->data |= IMPORTANT_NODE;
		return;
	} /* if... */
	for (p = theNode->conns; p != NIL(connect); p = p->next) {
		if (conncount++ > 10) {
			theNode->data |= INEFF_NODE;
			return;
		} /* if... */
		if ((p->ttype != TERM_OUTPUT) && (p->ttype != TERM_DLSIO)) {
			/* this node is important */
			theNode->data |= IMPORTANT_NODE;
		} /* if... */
		if (!MosInstance(p->device->type)) {
			theNode->data |= IMPORTANT_NODE;
		} /* if... */
		if (!gottype) {
			transtype = BasicTransistorType(p->device->type);
			gottype = TRUE;
		} else {
			if (transtype != BasicTransistorType(p->device->type)) {
				/* two incident transistors of differing type */
				theNode->data |= IMPORTANT_NODE;
			} /* if... */
		} /* if... */
	} /* for... */
} /* mark_important... */

/*************************************************************************
 * UNMARK_NODE
 * Unmark node.
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 */
int unmark_node(theNode)
	node *theNode;
{
	theNode->data &= ~NO_SERIES;
} /* unmark_node... */

/*************************************************************************
 * MERGE_TRANS
 * Try to merge transistors with all transistors in network.
 *
 * Parameters:
 *	None.
 */
void merge_trans()
{
	if (update) {
		Message("Starting merge (marking) ... ");
	} /* if... */
	ForEachNode(mark_important);
	if (update) {
		Message("... done with marking nodes");
		Message("Starting real merge ...");
	} /* if... */
	ForEachMusaInstance(try_merge);
	if (update) {
		(void) sprintf(errmsg, "... done with merge (%d merges completed)", num_merged);
		Message(errmsg);
	} /* if... */
	ForEachNode(unmark_node);
} /* merge_trans... */

