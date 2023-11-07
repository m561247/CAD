/***********************************************************************
 * Routines to schedule and process events.
 * Filename: prop.c
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
 *	int sched_start() - initialize instance scheduling.
 *	void init_sched() - initialize all instance scheduling.
 *	void inst_driveSensitiveSchedule() - queue instance for evaluation.
 *	static void node_sched3() - queue node for evaluation.
 *	int	inst_normalSchedule() - queue instance for evaluation.
 *	static void get_composite() - update composite voltage value.
 *	void musaDriveHigh() - drive high.
 *	void musaUndriveHigh() - undrive high.
 *	void musaDriveLow() - drive low.
 *	void musaUndriveLow() - undrive low.
 *	void musaRailDrive() - drive rail.
 *	void musaRailUndrive() - undrive rail.
 *	int16 events_pending() - check events pending.
 *	void dump_events() - clear queues.
 *	static void reset_cycle_flags() - reset cycle flags.
 *	void Evaluate() - evaluate instances in queues.
 *	int rpcMusaEvaluate() - rpc interface to 'evaluate' command.
 *	void SetBreak() - 'setbreak' command.
 * Modifications:
 *	Rodney Lai 					commented				08/11/89
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define PROP_MUSA
#include "musa.h"
#include "queue.h"

typedef struct iqueue_struct iqueue;
struct iqueue_struct {
	musaInstance	*inst;
	iqueue	*next;
}; /* iqueue_struct... */

typedef struct nqueue_struct nqueue;
struct nqueue_struct {
	node	*theNode;
	nqueue	*next;
}; /* nqueue_struct... */

musaQueue queue1;
musaQueue queue2;
musaQueue queue3;
musaQueue queueCycle;

/************************************************************************
 * SCHED_START
 * Schedule instances with sched flag set
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 */
void sched_start(inst)
	musaInstance *inst;
{
    (*deviceTable[(int) inst->type].startup_routine)(inst);
}

/***********************************************************************
 * INIT_SCHED
 * Initialize scheduling variables.
 *
 * Parameters:
 *	None.
 */
void init_sched()
{
    musaQueueInit( &queue1, 0 );
    musaQueueInit( &queue2, 1 );
    musaQueueInit( &queue3, 2 );
    musaQueueInit( &queueCycle, 3 );

    /* schedule insts that need to be forced to fire as part of startup */
    ForEachMusaInstance(sched_start);

    /* drive VddNodes and GndNodes */
    init_all_power_nodes();
}				/* init_sched... */

/******************************************************************************
 * INST_DRIVESENSITIVESCHEDULE
 * Queue an instance or node (in appropriate priority queue)
 * for later evaluation
 *
 * TERM_DLSIO inputs only -> for re-evaluating transistor networks
 *
 * Parameters:
 *	theInst (musaInstance *) - the instance. (Input)
 */
void inst_driveSensitiveSchedule( theInst )
	musaInstance *theInst;
{
    musaQueueEnq( &queue1, (scheduleItem*)theInst );
}

/****************************************************************************
 * NODE_SCHED2
 * nodes that may have changed due to drive or undrive
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 */
static void node_sched2( theNode )
    node *theNode;
{
    if ( disable_sched2 )  return;
    musaQueueEnq( &queue2, (scheduleItem*)theNode );
}

/***************************************************************************
 * INST_NORMALSCHEDULE
 * Run of the mill scheduling.  TERM_VSI and TERM_DSI inputs
 *
 * Parameters:
 *	theInst (musaInstance *) - the instance. (Input)
 */
int inst_normalSchedule(theInst)
	musaInstance *theInst;
{
    theInst->schedule &= ~SKIP_SCHED3;
    musaQueueEnq( &queue3, (scheduleItem*)theInst );
}

/*************************************************************************
 * GET_COMPOSITE
 * Update the composite value of a node, and schedule TERM_VSI's and TERM_DSI's
 * as appropriate.
 * theNode->count is incremented and a warning is printed if an
 * oscillation is detected.
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 */
static void get_composite(theNode)
    node* theNode;
{
    int oldcomp;		/* former composite value of node */
    int newcomp;		/* new composite value of node */
    register int mask;		/* mask to choose conns to sched */
    register connect *p;
    char *name;

    oldcomp = theNode->voltage;
    newcomp = LEVEL(theNode);
    if (oldcomp == newcomp) {
	return;			/* No change in node. */
    } else if ((newcomp & 0x3) == (oldcomp & 0x3)) {
	mask = TERM_DSI;	/* Consider drive sensitive terminals only */
    } else {
	mask = TERM_DSI | TERM_VSI; /* Consider all terminals  */
    }
    if (theNode->count++ == 0) {
	if ( ! musaInQueue(&queueCycle, theNode ) ) {
	    musaQueueEnq( &queueCycle, (scheduleItem*)theNode );
	} else { 
	    if ((theNode->data & CYCLE_REPORTED) == 0) {
		/* This is the second wrap -- report problem */
		MakeElemPath(theNode->rep, &name);
		(void) sprintf(errmsg, "Node %s is part of a cycle.", name);
		Warning(errmsg);
		theNode->data |= CYCLE_REPORTED;
	    } else {
		/* This is the third wrap around */
		Warning("All cycle nodes reported.");
		sim_status |= SIM_CYCLE;
	    }		
	}		
    }			
    theNode->voltage = newcomp;
    for (p = theNode->conns; p != NIL(connect); p = p->next) {
	if ((p->ttype & mask) != 0) {
	    inst_normalSchedule(p->device);
	}
    }		
}		

/******************************************************************************
 * DRIVEH
 * Add a driver or increase drive of high level
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 *	theConn (connect *) - the connection to the node. (Input)
 */
void musaDriveHigh(theNode, theConn)
	node	*theNode;
	connect	*theConn;	/* connection to drive */
{
    int	     level; /* level to drive */
    int	     thresh;	/* current level of node */
    register connect	*p;

    if (theConn == NIL(connect)) {
	level = 0;
    } else {
	level = theConn->highStrength;
	theConn->hout = 1;	/* Mark connection as driving. */
    }

    thresh = theNode->highStrength;
    if (level == thresh) {
	theNode->numberOfHighDrivers++;
    } else if (level < thresh) {
	/* new driver is biggest drive (lowest level) */
	theNode->highStrength = level;
	theNode->numberOfHighDrivers = 1;
	for (p = theNode->conns; p != NIL(connect); p = p->next) {
	    if (((p->ttype & TERM_DLSIO) != 0) && (p != theConn)) {
		inst_driveSensitiveSchedule(p->device);
	    }
	}	
	node_sched2(theNode);
    }		
}				/* musaDriveHigh... */

/*************************************************************************
 * MUSAUNDRIVEHIGH
 * Remove a high driver from a node
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 *	theConn (connect *) - the connection to the node. (Input)
 */
void musaUndriveHigh(theNode, theConn)
    node	*theNode;
    connect	*theConn;	/* connection that stopped driving */
{
    connect	*p;
    int		level;		/* level of driver to be removed */
    int		thresh;		/* current level of node */
    connect	*term;		/* opposite terminal on transitor */

    if (theConn == NIL(connect)) {
	level = 0;
    } else {
	level = theConn->highStrength;
	if (theConn->hout) {
	    theConn->hout = 0;
	} else {
	    InternalError("undrive from input connection");
	}
    }

    thresh = theNode->highStrength;
    if (level > thresh) return;

    if (--theNode->numberOfHighDrivers == 0) {
	node_sched2(theNode);
	if (theNode->lowStrength < 254) {
	    theNode->highStrength = (thresh = 255);
	} else {
	    /* node will be charged high */
	    theNode->highStrength = (thresh = 254);
	    theNode->lowStrength = 255;
	}

	for (p = theNode->conns; p != NIL(connect); p = p->next) {
	    if (p == theConn) continue;

	    if (p->hout) {
		/* this connection is currently driving */
		/* set highStrength and numberOfHighDrivers accordingly */
		level = p->highStrength;
		if (level == thresh) {
		    theNode->numberOfHighDrivers++;
		} else if (level < thresh) {
		    /* new driver is biggest drive (lowest level) */
		    theNode->highStrength = (thresh = level);
		    theNode->numberOfHighDrivers = 1;
		}		

	    } else if ((p->ttype & TERM_DLSIO) != 0) {
		/* find opposite terminal (source=>drain drain=>source) */
		if ((term = &(p->device->connArray[1])) == p) {
		    term = &(p->device->connArray[0]);
		}		
		/* must undrive all that follow in chain */
		if (term->hout) {
		    musaUndriveHigh(term->node, term);
		}		
		inst_driveSensitiveSchedule(p->device);
	    }			
	}			
    }				
}				/* musaUndriveHigh... */

/*************************************************************************
 * MUSADRIVELOW
 * Add a driver or increase drive of low level
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 *	theConn (connect *) - the connection to the node. (Input)
 */
void musaDriveLow(theNode, theConn)
	node	*theNode;
	connect	*theConn;	/* connection that drives */
{
    int					level; /* level to drive */
    int					thresh;	/* current level of node */
    register connect	*p;

    if (theConn == NIL(connect)) {
	level = 0;
    } else {
	level = theConn->lowStrength;
	theConn->lout = 1;
    }		

    thresh = theNode->lowStrength;
    if (level == thresh) {
	theNode->numberOfLowDrivers++;
    } else if (level < thresh) {
	/* new driver is biggest drive (lowest level) */
	theNode->lowStrength = level;
	theNode->numberOfLowDrivers = 1;
	for (p = theNode->conns; p != NIL(connect); p = p->next) {
	    if (((p->ttype & TERM_DLSIO) != 0) && (p != theConn)) {
		inst_driveSensitiveSchedule(p->device);
	    }			
	}			
	node_sched2(theNode);
    }				/* else {ignore it} */
}				/* musaDriveLow... */

/**************************************************************************
 * MUSAUNDRIVELOW
 * Remove a low driver from a node
 * if theConn = NIL, this is a level 0 undrive
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 *	theConn (connect *) - the connection to the node. (Input)
 */
void musaUndriveLow(theNode, theConn)
	node	*theNode;
	connect	*theConn;	/* connection that stopped driving */
{
    connect	*p;
    connect	*term;		/* opposite terminal of trans */
    int		level;		/* level of driver to be removed */
    int		thresh;		/* current level of node */

    if (theConn == NIL(connect)) {
	level = 0;		/* To rail. */
    } else {
	level = theConn->lowStrength;
	if (theConn->lout) {
	    theConn->lout = 0;
	} else {
	    InternalError("undrive from input connect");
	}	
    }		

    thresh = theNode->lowStrength;
    if (level > thresh) return;	      /* Was not the maximum driver. */

    if ( --theNode->numberOfLowDrivers == 0) {
	node_sched2(theNode);
	if (theNode->highStrength < 254) { /* Node being driven hi? */
	    theNode->lowStrength = thresh = 255; /* No lo drive */
	} else {
	    /* node will be charged low */
	    theNode->lowStrength = (thresh = 254);
	    theNode->highStrength = 255;
	}			

	for (p = theNode->conns; p != NIL(connect); p = p->next) {
	    if (p == theConn) continue;

	    if (p->lout) {
		/* this connection is currently driving */
		/* set lowStrength and numberOfLowDrivers accordingly */
		level = p->lowStrength;
		if (level == thresh) {
		    theNode->numberOfLowDrivers++;
		} else if (level < thresh) { /* new driver is biggest drive (lowest level) */
		    theNode->lowStrength = (thresh = level);
		    theNode->numberOfLowDrivers = 1;
		}

	    } else if ((p->ttype & TERM_DLSIO) != 0) {
		/* find opposite terminal (source=>drain drain=>source) */
		if ((term = &(p->device->connArray[1])) == p) {
		    term = &(p->device->connArray[0]);
		}
		/* must undrive all that follow in chain */
		if (term->lout) {
		    musaUndriveLow(term->node, term);
		}
		inst_driveSensitiveSchedule(p->device);
	    }
	}
    }
}				/* musaUndriveLow... */

/*************************************************************************
 * MUSARAILDRIVE
 * Set a node to rail value
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 *	high (int16) - if TRUE, set high, else low. (Input)
 */
void musaRailDrive(theNode, high)
	node	*theNode;
	int16	high;
{
    if (high) {
	if (theNode->highStrength != 0) {
	    /* undrive low since high rail breaks low chain of trans's */
	    theNode->numberOfLowDrivers = 1;
	    musaUndriveLow(theNode, NIL(connect));
	    musaDriveHigh(theNode, NIL(connect));
	    get_composite(theNode);
	}	
    } else {
	if (theNode->lowStrength != 0) {
	    /* undrive high since low rail breaks high chain of trans's */
	    theNode->numberOfHighDrivers = 1;
	    musaUndriveHigh(theNode, NIL(connect));
	    musaDriveLow(theNode, NIL(connect));
	    get_composite(theNode);
	}	
    }		
}				/* musaRailDrive... */

/***************************************************************************
 * MUSARAILUNDRIVE
 * Undrive a rail
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 *	high (int16) - if TRUE, undrive high, else low. (Input)
 */
void musaRailUndrive(theNode, high)
    node	*theNode;
    int16	high;
{
    if (high) {
	if (theNode->highStrength == 0) {
	    theNode->numberOfHighDrivers = 1;
	    musaUndriveHigh(theNode, NIL(connect));
	    get_composite(theNode);
	}			/* if... */
    } else {
	if (theNode->lowStrength == 0) {
	    theNode->numberOfLowDrivers = 1;
	    musaUndriveLow(theNode, NIL(connect));
	    get_composite(theNode);
	}
    }		
}		

/*************************************************************************
 * EVENTS_PENDING
 * Check if events are pending.
 *
 * Parameters:
 *	None.
 *
 * Return Value:
 *	TRUE, if events are pending. (int16)
 */
int events_pending()
{
    return queue1.count || queue2.count || queue3.count ;
}

/*************************************************************************
 * DUMP_EVENTS
 * Clear all the event queues
 *
 * Parameters:
 *	None.
 */
void dump_events()
{
    musaQueueClear( &queue1 );
    musaQueueClear( &queue2 );
    musaQueueClear( &queue3 );
}

/***************************************************************************
 * RESET_CYCLE_FLAGS
 * Reset flags in nodes that are in the ncycle queue
 *
 * Parameters:
 *	None.
 */
static void reset_cycle_flags()
{
    musaQueueClear( &queueCycle );
}

clearSchedule( theNode )
    node* theNode;
{
    if ( theNode->schedule ) {
	/* printf( "node/instance with unempty schedule %d\n", theNode->schedule ); */
	theNode->schedule = 0;
    }
}


/****************************************************************************
 * EVALUATE
 * Evaluate instances in the queue.
 *
 * Parameters:
 *	argc (int) - the argument count. (Input)
 *	argv (char **) - the arguments. (Input)
 *	initialize (int16) - if TRUE, clear queues and initialize eval. (Input)
 */
void Evaluate(argc, argv, initialize)
	int			argc;
	char		**argv;
	int16		initialize;
{
    static int was_interrupted = FALSE; /* evaluationwas interrupted */
    deviceType	type;
    musaInstance* inst;
    node  *theNode;
    int (*func)();
    

    (void) ignore((char *) argc);
    (void) ignore((char *) argv);

    NextMacroTime();		/*  */
    HandleXEvents();

    if (initialize) {
	reset_cycle_flags();
	was_interrupted = FALSE;
    } else {
	if (!was_interrupted) {
	    return;
	}			/* if... */
    }				/* if... */

 labelqueue1:
    while ( musaQueueDeq( &queue1, &inst ) ) {
	type = inst->type;
	func = deviceTable[(int) type].sim_routine;
	(*func)(inst,
		deviceTable[(int)type].param,
		deviceTable[(int)type].param2,
		deviceTable[(int)type].param3);
    }

 labelqueue2:
    /* re-evaluate node voltages -> take appropriate action if change */
    while ( musaQueueDeq(  &queue2, &theNode ) ){
	get_composite( theNode );
    }	

    while ( musaQueueDeq( &queue3, &inst ) ) {
	if (sim_status != SIM_OK) {
	    /* Break out for interrupt, break or cycle detect */
	    was_interrupted = TRUE;
	    break;
	}			/* if... */
	if ((inst->schedule & SKIP_SCHED3) == 0) {
	    inst->schedule |= IS_SCHED3; /* Mark this as normal scheduling. */
	    type = inst->type;
	    func = deviceTable[(int) type].sim_routine;
	    (*func)(inst,
		    deviceTable[(int) type].param,
		    deviceTable[(int) type].param2,
		    deviceTable[(int) type].param3);
	    inst->schedule &= ~IS_SCHED3; /* No longer normal scheduling. */
	} else {
	    inst->schedule &= ~SKIP_SCHED3;
	}			

	if ( queue1.count ) goto labelqueue1;
	if ( queue2.count ) goto labelqueue2;
    }

    if (st_count(graphList) > 0) {
	UpdatePlot();
    }
    musaQueueClear( &queue1 );
    musaQueueClear( &queue2 );
    musaQueueClear( &queue3 );
    
    ForEachNode( clearSchedule );
    ForEachMusaInstance( clearSchedule );
}


#ifdef RPC_MUSA
/****************************************************************************
 * RPCMUSAEVALUATE
 * Rpc interface to 'evaluate' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	uow (int32) - unused. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaEvaluate (spot,cmdList,uow)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		uow;
{
	(void) ignore((char *) spot);
	(void) ignore((char *) &cmdList);
	(void) ignore((char *) &uow);

	Evaluate (0,NIL(char *),TRUE);
	return(RPC_OK);
} /* rpcMusaEvaluate... */
#endif

/****************************************************************************
 * SETBREAK
 * setbreak command
 * create a special instance that fires at appropriate time
 *
 * Parameters:
 *	argc (int) - the argument count. (Input)
 *	argv (char **) - the arguments. (Input)
 *	cwd (musaInstance *) - current working instance. (Input)
 */
void SetBreak(argc, argv, cwd)
	int argc;
	char **argv;
	musaInstance *cwd;
{
	(void) ignore((char *) cwd);
	(void) ignore((char *) argc);
	(void) ignore((char *) argv);

	Warning("setbreak not yet implemented (I'm sorry)!!");
	sim_status |= SIM_BREAK;
	return;
#ifdef NOT_YET_IMPLEMENTED
	MusaElement *data;
	char connect_mode = 'V';
	char ttype;			/* type of connection to node */
	int badusage = 0;

	if (argc == 0) {
		Warning("Break!!");
		sim_status |= SIM_BREAK;
		return;
	} /* if... */
	if (argc == 2) {
		connect_mode = *argv[1];
	} /* if... */
	switch (connect_mode) {
		case 'V': case 'v':
			ttype = TERM_VSI;
			break;
		case 'D',case 'd':
			ttype = TERM_DSI;
			break;
		case 'O': case 'o': case '0':
			ttype = OUTPUT;	/* never fires => setbreak off */
			break;
		default:
			badusage = 1;
	} /* switch... */
	if ((argc < 1) || badusage) {
		Usage("setbreak element [V | D | O]\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
	gnSetDir(cwd);
	if (!gnGetElem(argv[0], &data)) return;
	theNode = ElementToNode(data);
#endif
} /* SetBreak... */
