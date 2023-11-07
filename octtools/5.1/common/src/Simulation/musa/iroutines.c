/***********************************************************************
 * Drive nodes
 * Filename: iroutines.c
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
 *	void drive_output() - drive a connection's output.
 *	void weakdrive_output() - drive a connection's output weakly.
 *	void undrive_output() - undrive a connection's output.
 *	void make_leaf_inst() - create a new instance as a child of 'parent_inst'.
 *	void connect_term() - connect terminal to terminal of an instance.
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define IROUTINES_MUSA
#include "musa.h"

/*************************************************************************
 * DRIVE_OUTPUT
 * Drive a connection at a specific value.
 * outlevel == L_0 => low,  L_1 => high,  L_X => unknown
 *
 * Parameters:
 *	conn (connect *) - connection terminal. (Input/Output)
 *	outlevel (int) - output voltage level of terminal. (Input)
 */
void drive_output(conn, outlevel)
	connect	*conn;
	int		outlevel;
{
    switch (outlevel & L_VOLTAGEMASK) {
    case L_o:
	if (conn->hout) {
	    musaUndriveHigh(conn->node, conn);
	}	
	if (!conn->lout || (conn->lowStrength > 1)) {
	    conn->lowStrength = 1;
	    musaDriveLow(conn->node, conn);
	}	
	break;
    case L_i:
	if (conn->lout) {
	    musaUndriveLow(conn->node, conn);
	}	
	if (!conn->hout || (conn->highStrength > 1)) {
	    conn->highStrength = 1;
	    musaDriveHigh(conn->node, conn);
	}	
	break;
    case L_x:
	if (!conn->lout || (conn->lowStrength > 1)) {
	    conn->lowStrength = 1;
	    musaDriveLow(conn->node, conn);
	}	
	if (!conn->hout || (conn->highStrength > 1)) {
	    conn->highStrength = 1;
	    musaDriveHigh(conn->node, conn);
	}	
	break;
    default:
	errRaise( "MUSA", 1, "Illegal argument %d in drive_output", outlevel);
    }		
}				/* drive_output... */

/***********************************************************************
 * WEAKDRIVE_OUTPUT
 * Drive a connection weakly at a specific value.
 * outlevel == L_0 => low,  L_1 => high,  L_X => unknown
 *
 * Parameters:
 *	conn (connect *) - connection terminal. (Input/Output)
 *	outlevel (int) - output voltage level of terminal. (Input)
 */
void weakdrive_output(conn, outlevel)
	connect	*conn;
	int		outlevel;
{
    switch (outlevel & L_VOLTAGEMASK) {
    case L_o:
	if (conn->hout) {
	    musaUndriveHigh(conn->node, conn);
	}
	if (conn->lout && (conn->lowStrength < 128)) {
	    musaUndriveLow(conn->node, conn);
	}
	if (!conn->lout || (conn->lowStrength > 128)) {
	    conn->lowStrength = 128;
	    musaDriveLow(conn->node, conn);
	}
	break;
    case L_i:
	if (conn->lout) {
	    musaUndriveLow(conn->node, conn);
	}
	if (conn->hout && (conn->highStrength < 128)) {
	    musaUndriveHigh(conn->node, conn);
	}
	if (!conn->hout || (conn->highStrength > 128)) {
	    conn->highStrength = 128;
	    musaDriveHigh(conn->node, conn);
	}
	break;
    case L_x:
	if (conn->lout && (conn->lowStrength < 128)) {
	    musaUndriveLow(conn->node, conn);
	}
	if (!conn->lout || (conn->lowStrength > 128)) {
	    conn->lowStrength = 128;
	    musaDriveLow(conn->node, conn);
	}
	if (conn->hout && (conn->highStrength < 128)) {
	    musaUndriveHigh(conn->node, conn);
	}
	if (!conn->hout || (conn->highStrength > 128)) {
	    conn->highStrength = 128;
	    musaDriveHigh(conn->node, conn);
	}
	break;
    default:
	FatalError("Illegal argument in drive_output");
    }		
}				/* weakdrive_output... */

/*************************************************************************
 * UNDRIVE_OUTPUT
 * Take any drive off of an output
 *
 * Parameters:
 *	conn (connect *) - connection terminal. (Input/Output)
 */
void undrive_output(conn)
	connect *conn;
{
    if (conn->hout) {
	musaUndriveHigh(conn->node, conn);
    }	
    if (conn->lout) {
	musaUndriveLow(conn->node, conn);
    }	
}				/* undrive_output... */


void connInit( conn, ttype, inst )
    connect* conn;
    int ttype;			/* Connection type. */
    musaInstance* inst;
{
    conn->device = inst;
    conn->ttype = ttype;
    conn->highStrength = 255;
    conn->lowStrength = 255;
    conn->hout = 0;
    conn->lout = 0;
}

/**********************************************************************
 * MAKE_LEAF_INST
 * Create a new instance as a child of 'parent_inst'.
 *
 * Parameters:
 *	instptr (musaInstance **) - the new instance. (Output)
 *	type (deviceType) - the instance device type. (Input)
 *	name (char *) - the instance name. (Input)
 *	terms (int) - the number of terminals. (Input)
 */
void make_leaf_inst(instptr, type, name, terms)
	musaInstance	**instptr;
	deviceType	type;			/* instance type */
	char		*name;
	int		terms;			/* number of terminals on new instance */
{
    musaInstance		*inst;
    int			expand, i;

    make_musaInstance(instptr, parent_inst, name);
    inst = *instptr;
    inst->type = type;

    expand = deviceTable[(int) type].expandable;
    inst->connArray = ConnAlloc(terms);
    inst->terms = terms;
    if ( deviceTable[(int)type].term ) {
	for (i = 0; i < terms; i++) {
	    int ttype;		/* Connection type. */
	    if ((expand == 0) || (i < expand)) {
		ttype = deviceTable[(int) type].term[i].ttype;
	    } else {
		ttype = deviceTable[(int) type].term[expand].ttype;
	    }
	    connInit( &inst->connArray[i], ttype, inst );
	}			
    }
}		

/*************************************************************************
 * CONNECT_TERM
 * Connect node associate with 'aterm', an oct actual terminal,
 * to the given terminal of 'inst'.
 *
 * Global Variables:
 *	inter_facet (octObject *) - interface facet of 'aterm'.
 *	parent_inst (musaInstance *) - the parent instance which contains
 *		element corresponding to 'aterm'.
 *
 * Parameters:
 *	inst (musaInstance) - the instance to connect 'aterm' to. (Input)
 *	aterm (octObject *) - the oct actual terminal to connect. (Input)
 *	term (int) - the terminal in the instance to connect to. (Input)
 */
void connect_term(inst, aterm, term)
    musaInstance *inst;
    octObject	 *aterm;
    int		 term;
{
    MusaElement		*elem;
    node		*theNode;

    get_oct_elem(aterm, parent_inst, &elem);
    theNode = ElementToNode(elem);
    inst->connArray[term].node = theNode;
    inst->connArray[term].next = theNode->conns;
    theNode->conns = &(inst->connArray[term]);
}
