
/***********************************************************************
 * Routines to give the user feekback as to the state of the circuit.
 * Filename: info.c
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
 *	char *ConnectionType() - return string for given connection type.
 *	void DoNodeInfo () - show information about a node.
 *	void NodeInfo() - 'nodeinfo' command.
 *	int rpcMusaNodeInfo() - rpc interface to 'nodeinfo' command.
 *	void InstInfo() - 'instinfo' command.
 *	int rpcMusaInstInfo() - rpc interface to 'instinfo' command.
 *	void ConnectInfo() - display connection information.
 *	int	mos_info() - display information on mos instance.
 *	void btrace() - find nodes that contribute to a nodes value.
 *	void Backtrace() - 'backtrace' command.
 *	int rpcMusaBacktrace() - rpc interface to 'backtrace' command.
 * Modifications:
 *	Rodney Lai				commented						08/08/89
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define INFO_MUSA
#include "musa.h"

/**************************************************************************
 * CONNECTIONTYPE
 * Return string corresponding to connection type.
 *
 * Parameters:
 *	type (int) - the connection type. (Input)
 *
 * Return Value:
 *	The string corresponding to the specified connection type. (char *)
 */
char *ConnectionType(type)
	int		type;
{
	switch (type) {
		case TERM_OUTPUT:
			return ("OUT");
		case TERM_VSI:
			return ("VSI");
		case TERM_DSI:
			return ("DSI");
		case TERM_DLSIO:
			return ("DIO");
		default:
			return ("XXX");
	} /* switch... */
} /* ConnectionType... */

/************************************************************************
 * DONODEINFO
 * Print information about the specified node (element).
 *
 * Parameters:
 *	data (MusaElement *) - the node to print information about. (Input)
 */
void DoNodeInfo (data)
	MusaElement		*data;
{
    node		*theNode;
    char		*name;
    connect		*c;

    theNode = ElementToNode(data);
    MakeElemPath(theNode->rep, &name);
    (void) sprintf(msgbuf,
		   "%s : %c\n\t%d high drivers (strength %d)   %d low drivers (strength %d)\n",
		   name,LETTER(theNode),
		   theNode->numberOfHighDrivers,
		   theNode->highStrength,
		   theNode->numberOfLowDrivers,
		   theNode->lowStrength );
    MUSAPRINTINIT(msgbuf);
    FREE(name);
    for (c = theNode->conns; c != NIL(connect); c = c->next) {
	MakeConnPath(c, &name);
	if (c->hout) {
	    (void) sprintf(msgbuf,"    Driven HIGH (strength %d) by %s %s\n",
			   c->highStrength,deviceTable[(int) c->device->type].name,name);
	    MUSAPRINT(msgbuf);
	}		
	if (c->lout) {
	    (void) sprintf(msgbuf, "    Driven LOW  (strength %d) by %s %s\n",
			   c->lowStrength,deviceTable[(int) c->device->type].name,name);
	    MUSAPRINT(msgbuf);
	}		
	if (!c->hout && !c->lout) {
	    (void) sprintf(msgbuf, "    FANOUT %s %s\n", 
			   deviceTable[(int) c->device->type].name, name);
	    MUSAPRINT(msgbuf);
	}		
	FREE(name);
	if (sim_status & SIM_CMDINTERR) {
	    return;
	}
    }		
#ifdef RPC_MUSA
    rpcMusaPrint("Node Information");
#endif
}				/* DoNodeInfo... */

/**************************************************************************
 * NODEINFO
 * Display information about nodes.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - the arguements. (Input)
 *	cwd (musaInstance *) - the current working instance. (Input)
 */
void NodeInfo(argc, argv, cwd)
	int			argc;
	char		**argv;
	musaInstance		*cwd;
{
	MusaElement		*data;
	int16		i;

#ifndef RPC_MUSA
	if (argc < 1) {
		Usage("nodeinfo node1 [node2 ...]\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	for (i = 0; i < argc; i++) {
		gnSetDir(cwd);
		if (gnGetElem(argv[i], &data)) {
			DoNodeInfo(data);
		} /* if... */
	} /* for... */
} /* NodeInfo... */

#ifdef RPC_MUSA
/**************************************************************************
 * RPCMUSANODEINFO
 * Rpc interface to the 'nodeinfo' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaNodeInfo (spot,cmdList,cominfo)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
	int32		cominfo; 
{
	char			*argList[256];

    (void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	if (lsLength(cmdList) == 0) {
		if (rpcMusaUnderSpot(spot,1,argList,0,TRUE,FALSE,NodeInfo)) {
			return(RPC_OK);
		} else {
			Format("musa-nodeinfo",(COMINFO *) cominfo);
			return (RPC_OK);
		} /* if... */
	} /* if... */
	rpcMusaRunRoutine (spot,cmdList,1,argList,0,TRUE,FALSE,NodeInfo);
	return (RPC_OK);
} /* rpcMusaNodeInfo... */
#endif
					
/**************************************************************************
 * INSTINFO
 * Display information about instance.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - the arguements. (Input)
 *	cwd (musaInstance *) - the current working instance. (Input)
 */
void InstInfo(argc, argv, cwd)
    int			argc;
    char		**argv;
    musaInstance	*cwd;
{
    musaInstance	*inst;
    char		*name;
    int16		i;
    int16		junk;

#ifndef RPC_MUSA
    if (argc < 1) {
	Usage("instinfo inst1 [inst2 ...]\n");
	sim_status |= SIM_USAGE;
	return;
    }		
#endif
    for (i = 0; i < argc; i++) {
	gnSetDir(cwd);
	if (gnInstPath(argv[i], &inst, TRUE, &junk)) {
	    MakePath(inst, &name);
	    if (inst->type == DEVTYPE_MODULE) {
		(void) sprintf(msgbuf, "Module instance %s\n", name);
		MUSAPRINTINIT(msgbuf);
#ifdef RPC_MUSA
		rpcMusaPrint("Instance Information");
#endif
		FREE(name);
	    } else {
		(void) sprintf(msgbuf,
			       "%s %s", deviceTable[(int) inst->type].name, name);
		MUSAPRINTINIT(msgbuf);
		FREE(name);
		(*deviceTable[(int)inst->type].info_routine)(inst);
#ifdef RPC_MUSA
		rpcMusaPrint("Instance Information");
#endif
	    }	
	}	
    }		
}		


char* strengthInWords( strength )
    int strength;
{
    static char array[8][32];
    static int index = 0;

    if ( strength == 0 ) {
	return "to rail";
    } else if ( strength == 1 ) {
	return "very strong";
    } else if ( strength >= 254 ) {
	return "no strength";
    } else {
	index++;
	index %= 8;
	if ( strength > 128 ) {
	    sprintf( array[index], "weakly(%d)", strength );
	} else {
	    sprintf( array[index], "strongly(%d)", strength );
	}
	return array[index];
    }
}


void printNodeDrive( dirStr, strength )
    char* dirStr;
    unsigned int strength;
{
    if ( strength < 254 ) {
	(void) sprintf(msgbuf,"%s %s", dirStr, strengthInWords( strength ));
	MUSAPRINT(msgbuf);
    }  
}



#ifdef RPC_MUSA
/**************************************************************************
 * RPCMUSAINSTINFO
 * Rpc interface to 'instinfo' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaInstInfo (spot,cmdList,cominfo)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
	int32		cominfo;
{
	char			*argList[256];

    (void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	if (lsLength(cmdList) == 0) {
		if (rpcMusaUnderSpot(spot,1,argList,0,TRUE,TRUE,InstInfo)) {
			return(RPC_OK);
		} else {
			Format("musa-instinfo",(COMINFO *) cominfo);
			return (RPC_OK);
		} /* if... */
	} /* if... */
	rpcMusaRunRoutine (spot,cmdList,1,argList,0,TRUE,TRUE,InstInfo);
	return (RPC_OK);
} /* rpcMusaInstInfo... */
#endif

					
void singleConnectInfo( conn, buf2 )
    connect* conn;
    char* buf2;
{
    char* name;
    MakeElemPath(conn->node->rep, &name);
    (void) sprintf(msgbuf, " %8s:%s: %c :%-14s:",
		   buf2, ConnectionType(conn->ttype), LETTER(conn->node),name);
    MUSAPRINT(msgbuf);
    FREE(name);
    if (conn->hout) {
	printNodeDrive( "DRIVES HI", conn->highStrength );
    } else {
	printNodeDrive( "driven HI", conn->node->highStrength);
    }
    if (conn->lout) {
	printNodeDrive( "DRIVES LO", conn->lowStrength );
    } else {
	printNodeDrive( "driven LO", conn->node->lowStrength );
    }
    MUSAPRINT( "\n" );
    
}
/**********************************************************************
 * CONNECTINFO
 * give information on connections (for ii)
 * Synthesize terminal name for expandables (WHAT ???)
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 *	numconns (int) - the number of connections to instance. (Input)
 */
void ConnectInfo(inst, numconns)
	musaInstance	*inst;
	int		numconns;	/* number of connections to list */
{
    connect		*conn;
    char 		buf2[50];
    int			expand, i;

    expand = deviceTable[(int) inst->type].expandable;

    for (i = 0; i < numconns; i++) {
	/* find the name for this connection */
	if ((expand == 0) || (i < expand)) {
	    (void) strcpy(buf2, deviceTable[(int)inst->type].term[i].name);
	} else {
	    (void) sprintf(buf2, "%s%d", deviceTable[(int)inst->type].term[expand].name, i - expand + 1);
	}

	conn = &(inst->connArray[i]);
	singleConnectInfo( conn, buf2 );
    } 
}				/* ConnectInfo... */

/*********************************************************************
 * MOS_INFO
 * special routine to give ii info for mos network
 *
 * Parameters:
 *	inst (musaInstance *) - the mos instance. (Input)
 */
int    mos_info(inst)
    musaInstance *inst;
{
    int		numconns;
    (void) sprintf(msgbuf, "    %s -- currently %s\n",
		       network[inst->data & SUBTYPE].name, (inst->data & IS_ON) ? "ON" : "OFF"  );
    MUSAPRINT(msgbuf);
    if (MosTransistor(inst->type)) {
	numconns = 2 + network[inst->data & SUBTYPE].numin;
    } else {
	numconns = 1 + network[inst->data & SUBTYPE].numin;
    }	
    ConnectInfo(inst, numconns);
}

/*************************************************************************
 * BTRACE
 * Find all nodes that might contribute to theNode's Value
 *
 * Parameters:
 *	shown (st_table *) -
 *	theNode (node *) - the node. (Input)
 *	level (int) - the number of levels to backtrace. (Input)
 *	termname
 */
void btrace(shown, theNode, level, termname, limit)
    st_table		*shown;
    node		*theNode;
    int			level;		/* distance from the source node */
    char		*termname;
    int			limit;		/* limit for level */
{
    musaInstance *inst;
    connect	 *c;
    char 	 *name;
    int		 i;

    MakeElemPath(theNode->rep, &name);
    (void) sprintf(msgbuf, "%*s%s = %s = %c\n", level*3, "", termname,
		   name, LETTER(theNode));
    MUSAPRINT(msgbuf);
    FREE(name);

    if (level == limit) return;     /* Don't go beyond limit */


    /* Make sure nodes are not repeated */
    if (st_is_member(shown, (char *) theNode)) return;
    (void) st_insert(shown, (char *) theNode, NIL(char));

    /* don't search beyond a rail */
    if ((theNode->highStrength == 0) || (theNode->lowStrength == 0)) return;

    for (c = theNode->conns; c != NIL(connect); c = c->next) {
	if (sim_status & SIM_CMDINTERR) break;
	if (c->hout || c->lout) {
	    /* fix (possibly) incorrect levels */
	    if (!c->hout) {
		c->highStrength = 255;
	    }			
	    if (!c->lout) {
		c->lowStrength = 255;
	    }			

	    inst = c->device;

	    /* Make sure instances are not repeated */
	    if (st_is_member(shown, (char *) inst)) continue;
	    (void) st_insert(shown, (char *) inst, NIL(char));

	    /* print info about device */
	    for (i=0; i< inst->terms; i++) {
		if (&(inst->connArray[i]) == c) break;;
	    }			/* for... */
	    MakePath(inst, &name);
	    (void) sprintf(msgbuf, "%*s%s of %s drives %c\n", level*3+2, "",
			   deviceTable[(int) inst->type].term[i].name, name,
			   letter_table[LEVEL(c)]);
	    MUSAPRINT(msgbuf);
	    FREE(name);

	    /* call btrace recursively */
	    for (i=0; i< inst->terms; i++) {
		if (sim_status & SIM_CMDINTERR) break;
		if (&(inst->connArray[i]) == c) continue;
		btrace(shown,inst->connArray[i].node, level+1,
		       deviceTable[(int)inst->type].term[i].name, limit);
	    }			/* for... */
	}			/* if... */
    }				/* for... */
}				/* btrace... */

/*************************************************************************
 * BACKTRACE
 * Backtrace command
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - the arguements. (Input)
 *	cwd (musaInstance *) - the current working instance. (Input)
 */
void Backtrace(argc, argv, cwd)
	int			argc;
	char		**argv;
	musaInstance		*cwd;
{
	MusaElement		*data;
	node		*theNode;
	int			limit = 1;
	st_table	*shown = NIL(st_table);

#ifndef RPC_MUSA
	if ((argc < 1) || (argc > 2)) {
		Usage("backtrace node [levels]\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	gnSetDir(cwd);
	if (!gnGetElem(argv[0], &data)) return;
	theNode = ElementToNode(data);
	if (argc > 1) {
		limit = atoi(argv[1]);
	} /* if... */
#ifdef RPC_MUSA
	(void) sprintf (msgbuf,"Node: %s\n",argv[0]);
	MUSAPRINTINIT (msgbuf);
#endif
	shown = st_init_table(st_ptrcmp, st_ptrhash);
	btrace(shown, theNode, 0, "ORIGIN", limit);
	st_free_table(shown);
#ifdef RPC_MUSA
	rpcMusaPrint("Backtrace");
#endif
} /* Backtrace... */

#ifdef RPC_MUSA
/**************************************************************************
 * RPCMUSABACKTRACE
 * Rpc interface to 'backtrace' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaBacktrace(spot,cmdList,cominfo)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
	int32		cominfo;
{
	struct RPCArg	*arg;
	char			*argList[2];

    (void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	if (lsLength(cmdList) == 0) {
		if (rpcMusaUnderSpot(spot,1,argList,0,FALSE,FALSE,Backtrace)) {
			return(RPC_OK);
		} else {
			Format("musa-backtrace",(COMINFO *) cominfo);
			return (RPC_OK);
		} /* if... */
	} /* if... */
	if ((lsLastItem(cmdList,(lsGeneric *) &arg, LS_NH) == LS_OK) &&
		(arg->argType == VEM_TEXT_ARG) &&
		(atoi(arg->argData.string) > 0)) {
		(void) lsDelEnd (cmdList, (lsGeneric *) &arg);
		argList[1] = arg->argData.string;
		if (lsLength(cmdList) == 0) {
			(void) rpcMusaUnderSpot(spot,2,argList,0,FALSE,FALSE,Backtrace);
		} else {	
			rpcMusaRunRoutine (spot,cmdList,2,argList,0,FALSE,FALSE,Backtrace);
		} /* if... */
	} else {
		rpcMusaRunRoutine (spot,cmdList,1,argList,0,FALSE,FALSE,Backtrace);
	} /* if... */
	return (RPC_OK);
} /* rpcMusaBacktrace... */
#endif
