
/***********************************************************************
 * Routines to handle power nodes.
 * Filename: vdd.c
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
 *	static void InitPowerNodes() - initialize power node lists.
 *	int16 RecognizePowerNode() - check if power node and register as such.
 *	void ConnectPowerNodes() - connect power nodes.
 *	void DeletePowerNode() - delete power node.
 *	static void AddPowerNode() - add power node.
 *	int16 IsSupply() - is node a supply node?
 *	int16 IsGround() - is node a ground node?
 *	enum st_retval init_power_node() - drive power node.
 *	void init_all_power_nodes() - initialize all power nodes.
 * Modifications:
 *	Rick Spickelmier			bug fix					??/??/??
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define VDD_MUSA
#include "musa.h"

static st_table *power_nodes = NIL(st_table);
static int supplyNodes = 0;
static int groundNodes = 0;

/*************************************************************************
 * INITPOWERNODES
 * Initialize power nodes.
 *
 * Parameters:
 *	None.
 */
static void InitPowerNodes()
{
	if (power_nodes == NIL(st_table)) {
		power_nodes = st_init_table(st_ptrcmp, st_ptrhash);
		if (power_nodes == NIL(st_table)) {
			FatalError("Out of memory");
		} /* if... */
		VerboseMessage("Hash table for power nodes has been initialized");
		supplyNodes = 0;
		groundNodes = 0;
	} /* if... */
} /* InitPowerNodes... */

/************************************************************************
 * ADDPOWERNODE
 * Add power node.
 *
 * Parameters:
 *	netOrTerm (octObject *) - the oct net or terminal object. (Input)
 *	node (theNode *) - the power node. (Input)
 *	value (int) - supply or ground? (Input)
 *	nameRecognition (int16) - power node recognize by name? (Input)
 */
static void AddPowerNode(netOrTerm,theNode,value,nameRecognition)
	octObject	*netOrTerm;
	node		*theNode;
	int			value;
	int16		nameRecognition;
{
    if (netOrTerm != NIL(octObject)) {
	(void) sprintf(errmsg,"%s node: %s", value ? "SUPPLY":"GROUND", ohFormatName(netOrTerm));
	if (nameRecognition) {
	    (void) strcat(errmsg, " regognized by its name");
	} 
	VerboseMessage(errmsg);
    }
    (void) st_insert(power_nodes, (char *) theNode , (char *) value);
    if (value == 0) {
	theNode->is_ground = 1;
	groundNodes++;
    } else if (value == 1) {
	theNode->is_supply = 1;
	supplyNodes++;
    }				/* if... */
}				/* AddPowerNode... */

/*************************************************************************
 * RECOGNIZEPOWERNODE
 * Check if 'elem' is a power node if it is then register as power node.
 *
 * Parameters:
 *	netOrTerm (octObject *) - the oct object of the net or term. (Input)
 *	elem (MusaElement *) - the element. (Input)
 *
 * Return Value:
 *	TRUE, success.
 *	FALSE, element has already be registered as a power node.
 */
int16 RecognizePowerNode(netOrTerm, elem)
	octObject	*netOrTerm;
	MusaElement		*elem;
{
	octObject	prop;
	node		*theNode;
	char		*c,name[4];

	InitPowerNodes();
	theNode = ElementToNode(elem);
	if (st_is_member(power_nodes, (char *) theNode)) {
		return(FALSE);
	} /* if... */
	if (netOrTerm->type == OCT_NET) {
		if (ohGetByPropName(netOrTerm,&prop,"NETTYPE")==OCT_OK)  {
			if (StrPropEql(&prop,"SUPPLY")) {
				AddPowerNode(netOrTerm,theNode,1,FALSE);
			} else if (StrPropEql(&prop,"GROUND")) {
				AddPowerNode(netOrTerm,theNode,0,FALSE);
			} /* if... */
		} else {
			/* Do this by name */
			(void) strncpy(name, netOrTerm->contents.net.name, 3);
			name[3] = '\0';
			for (c = &name[0]; *c != '\0' ; c++) *c = tolower(*c);
			if (strcmp(name,"vdd") == 0) {
				AddPowerNode(netOrTerm,theNode,1,TRUE);
			} else if ( strcmp(name,"vss")==0 || strcmp(name,"gnd")==0) {
				AddPowerNode(netOrTerm,theNode,0,TRUE);
			} /* if... */
		} /* if... */
	} else if(netOrTerm->type == OCT_TERM) {
		/* Do this by name */
		(void) strncpy(name, netOrTerm->contents.term.name, 3);
		name[3] = '\0';
		for (c = &name[0]; *c != '\0' ; c++) *c = tolower(*c);
		if ( strcmp(name,"vdd")==0) {
			AddPowerNode(netOrTerm,theNode,1,TRUE);
		} else if ( strcmp(name,"vss")==0 || strcmp(name,"gnd")==0) {
			AddPowerNode(netOrTerm,theNode,0 ,TRUE);
		} /* if... */
	} /* if... */

	return(TRUE);			/* OK */
} /* RecognizePowerNode... */

/*************************************************************************
 * CONNECTPOWERNODES
 * Connect two power nodes.
 *
 * Parameters:
 *	keep (node *) - the power node to keep. (Input)
 *	old (node *) - the node to be connected and disposed. (Input)
 */
void ConnectPowerNodes(keep,old)
    node *keep,*old;
{
    if (keep->is_supply && old->is_ground || keep->is_ground && old->is_supply) {
	(void) sprintf(errmsg,"Supply and ground are shorted:\n\tNodes %s and %s",keep->rep->name,old->rep->name);
	FatalError(errmsg);
    }	
    if (old->is_supply && ! keep->is_supply) {
	AddPowerNode((octObject *) 0, keep , 1 ,FALSE);
    }	
    if (old->is_ground && ! keep->is_ground) {
	AddPowerNode((octObject *) 0, keep , 0 ,FALSE);
    }	
}	

/************************************************************************
 * DELETEPOWERNODE
 * Delete power node.
 *
 * Parameters:
 *	theNode (node *) - the power node. (Input)
 */
void DeletePowerNode(theNode)
	node *theNode;
{
	int *value = 0;

	if (st_delete(power_nodes, (char **) &theNode , (char **) &value)==0) {
		Warning("Removing unexisting entry. This is a bug");
	} else {
		theNode->is_supply = theNode->is_ground = 0;
		if (value) {
			supplyNodes--;
		} else {
			groundNodes--;
		} /* if... */
	} /* if... */
} /* DeletePowerNode... */

/**************************************************************************
 * ISSUPPLY
 * Is 'theNode' a supply node?
 *
 * Parameters:
 *	theNode (node *) - the node to check.
 *
 * Return Value:
 *	TRUE, node is a supply node.
 */
int16 IsSupply(theNode)
	node *theNode;
{
	char *value = (char *) 0;

	if (power_nodes != NIL(st_table)) {
		(void) st_lookup(power_nodes, (char *) theNode , &value);
	} /* if... */
	return((int16) value == 1);
} /* IsSupply... */

/**************************************************************************
 * ISGROUND
 * Is 'theNode' a ground node?
 *
 * Return Value:
 *	TRUE, node is a ground node.
 */
int16 IsGround(theNode)
	node *theNode;
{
	char *value = (char *) 1;

	if (power_nodes != NIL(st_table)) {
		(void) st_lookup(power_nodes, (char *) theNode , &value);
	} /* if... */
	return((int16) value == 0);
} /* IsGround... */

/**************************************************************************
 * INIT_POWER_NODE
 * Drive power node.
 *
 * Parameters:
 *	theNode (node *) - the power node. (Input)
 *	value (char *) - supply or ground? (Input)
 *	dummy (char *) - ignored. (Input)
 *
 * Return Value:
 *	ST_CONTINUE, success.
 */
enum st_retval init_power_node(theNode, value, dummy)
	node	*theNode;
	char	*value;
	char	*dummy;
{
    (void) ignore(dummy);	/* make saber and lint happy */

    musaRailDrive( theNode,(int16) value);
    return ST_CONTINUE;
}

/*************************************************************************
 * INIT_ALL_POWER_NODES
 * Drive all power nodes.
 *
 * Parameters:
 *	None.
 */
void init_all_power_nodes()
{
    (void) sprintf(errmsg,"There are %d SUPPLY and %d GROUND nodes",supplyNodes,groundNodes);
    VerboseMessage(errmsg);
    if (supplyNodes == 0) {
	Warning("There are no SUPPLY nodes");
    }	
    if (groundNodes == 0) {
	Warning("There are no GROUND nodes");
    }	
    if (power_nodes != NIL(st_table)) {
	(void) st_foreach(power_nodes, init_power_node , NIL(char));
    }	
}	

