/***********************************************************************
 * Routines to read oct file structure.
 * Filename: readoct.c
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
 *	static void ReadOctFTerm2() - read formal terminals from internal instance.
 *	static void ReadOctFTerm() - read formal terminals from root facet.
 *	void get_oct_elem() - get element associated with terminal.
 *	void connect_oct_elem() - connect two elements.
 *	static void read_oct_elems() - read oct elements.
 *	static void read_oct_insts() - read oct instances.
 *	void read_oct() - read oct.
 *	int16 mos_read() - read mos.
 *	int16 logic_read() - read logic.
 * Modifications:
 *	Rodney Lai				bug fix						2/10/89
 ***********************************************************************/

#define READOCT_MUSA
#include "musa.h"
#include "vov.h"

extern void read_oct_insts();

/************************************************************************
 * READOCTFTERM2
 * Read oct formal terminals from instance.
 *
 * Parameters:
 *	facet (octObject *) - the facet containing formal terminals to read. (Input)
 *	cinst (musaInstance *) - the current instance. (Input)
 */
static void ReadOctFTerm2 (facet, cinst)
	octObject	*facet;
	musaInstance		*cinst;
{
    MusaElement		*elem, *oldelem;
    octGenerator	gen;
    octObject		term, net;

    /* process the formal terminals */
    OH_ASSERT(octInitGenContents(facet, OCT_TERM_MASK, &gen));
    while (octGenerate(&gen, &term) == OCT_OK) {
	if (octGenFirstContainer(&term, OCT_NET_MASK, &net) != OCT_OK) {
	    (void) sprintf(errmsg, "Formal terminal %s is not attached to a net",term.contents.term.name);
	    VerboseMessage(errmsg);
	    /* Is there already an element with the same name ? */
	    if (ElemLookup(cinst, term.contents.term.name, &oldelem)) {
		(void) sprintf(errmsg,"Name conflict between formal term %s",term.contents.term.name);
		Warning(errmsg);
		FatalError("\
\tThe above formal terminal has the same name\n\
\tas a net but it is not attached to it.\n\
\tThis is too confusing for me");
	    }			/* if... */
	    make_element(&elem, cinst, term.contents.term.name);
	    (void) RecognizePowerNode(&term,elem);
	}			/* if... */
    }				/* while... */
}				/* ReadOctFTerm2... */

/*************************************************************************
 * READOCTFTERM
 * Read oct formal terminal from root facet.
 *
 * Parameters:
 *	facet (octObject *) - the facet containing formal terminals to read. (Input)
 *	cinst (musaInstance *) - the current instance. (Input)
 */
static void ReadOctFTerm (facet, cinst)
    octObject	*facet;
    musaInstance	*cinst;
{
    MusaElement *elem, *oldelem;
    octGenerator gen;
    octObject term, net;

    /* process the formal terminals */
    OH_ASSERT(octInitGenContents(facet, OCT_TERM_MASK, &gen));
    while (octGenerate(&gen, &term) == OCT_OK) {
	if (octGenFirstContainer(&term, OCT_NET_MASK, &net) != OCT_OK) {
	    (void) sprintf(errmsg, "Formal terminal %s is not attached to a net",term.contents.term.name);
	    VerboseMessage(errmsg);
	    /* Is there already an element with the same name ? */
	    if (ElemLookup(cinst, term.contents.term.name, &oldelem)) {
		(void) sprintf(errmsg,"Name conflict for formal term %s",term.contents.term.name);
		Warning(errmsg);
		FatalError("\
\tThe above formal terminal has the same name\n\
\tas a net but it is not attached to it.\n\
\tThis is too confusing for me");
	    }			/* if... */
	    make_element(&elem, cinst, term.contents.term.name);
	    (void) RecognizePowerNode(&term,elem);
	    continue;
	}			/* if... */
	if (!ElemLookup(cinst, net.contents.net.name, &oldelem)) {
	    InternalError("read_oct. Missing net from element table");
	}			/* if... */
	if (!ElemLookup(cinst, term.contents.term.name, &elem)) {
	    make_element(&elem, cinst, term.contents.term.name);
	    equiv_elem(oldelem, elem);
	    (void) sprintf(errmsg,"making %s the same as %s",term.contents.term.name,net.contents.net.name);
	    VerboseMessage(errmsg);
	} else {
	    (void) sprintf(errmsg,"ignoring formal terminal %s since it already exists as a net",term.contents.term.name);
	    VerboseMessage(errmsg);
	}			/* if... */
    }				/* while... */
}				/* ReadOctFTerm... */

/**************************************************************************
 * GET_OCT_ELEM
 * Get element associated with aterm.
 * It could be either a net or a formal terminal
 * which is not attached to a net.
 * RETURN element
 * make a new elem if necessary
 *
 * Parameters:
 *	aterm (octObject *) - the actual terminal. (Input)
 *	ifacet (octObject *) - interface of instance. (Input)
 *	cinst (musaInstance *) - the instance. (Input)
 *	elem (MusaElement **) - the element associated with 'aterm'. (Output)
 */
void get_oct_elem(aterm, cinst, elem)
    octObject *aterm;		/* term to attach */
    musaInstance *cinst;
    MusaElement **elem;
{
    octObject ifacet;
    octObject inst;		/* instance of aterm */
    octObject net;		/* net to attach to */
    octObject formal_term;	/* or term to attach to */
    octObject iterm;		/* interface term corresponding to aterm */
    octObject joined;		/* joined bag of aterm */
    octObject joinedterm, ajoinedterm;
    octObject testnet;
    octGenerator termgen, netgen;
    char buf[1024];

    if (octIdIsNull((inst.objectId = aterm->contents.term.instanceId))) {
	errRaise( "MUSA", 1, "connect_term terminal must be an instance terminal");
    }	
    OH_ASSERT(octGetById(&inst));
    OH_ASSERT(octInitGenContainers(aterm, OCT_NET_MASK, &netgen));
    if (octGenerate(&netgen, &net) == OCT_OK) {
	if (octGenerate(&netgen, &testnet) != OCT_GEN_DONE) {
	    (void) sprintf(errmsg, "multiple nets connected to %s",ohFormatName(aterm));
	    FatalError(errmsg);
	}	
	if (!ElemLookup(cinst, net.contents.net.name, elem)) {
	    InternalError("get_oct_elem(1)");
	}	
	octFreeGenerator( &netgen );
	return;
    } 

    if (octGenFirstContainer(aterm, OCT_TERM_MASK, &formal_term)==OCT_OK) {
	if (!ElemLookup(cinst, formal_term.contents.term.name, elem)) {
	    InternalError("get_oct_elem(2)");
	}	
	return;
    }	
    (void) sprintf(errmsg,"No connection for %s", ohFormatName(aterm));
    VerboseMessage(errmsg);

    /* XXX joined terms stuff is now done by VEM */

    /* tremendous pain to check all of the joined terms */
    iterm = *aterm;

    OH_ASSERT( ohOpenMaster( &ifacet, &inst, "interface", "r" ) );
    VOVinputFacet( &ifacet );
    OH_ASSERT(octGetByName(&ifacet, &iterm));
    joined.contents.bag.name = "JOINED";
    joined.type = OCT_BAG;
    if (octGetContainerByName(&iterm, &joined) != OCT_OK) {
	goto give_up;
    }		
    OH_ASSERT(octInitGenContents(&joined, OCT_TERM_MASK, &termgen));
    while (octGenerate(&termgen, &joinedterm) == OCT_OK) {
	if (octIdsEqual(joinedterm.objectId, iterm.objectId)) {
	    continue;
	}	
	ajoinedterm = joinedterm;
	OH_ASSERT(octGetByName(&inst, &ajoinedterm));
	if (octGenFirstContainer(&ajoinedterm, OCT_NET_MASK, &net) == OCT_OK) {
	    if (!ElemLookup(cinst, net.contents.net.name, elem)) {
		InternalError("get_oct_elem(3)");
	    }	
	    if ( verbose ) {
		(void) sprintf(errmsg, "\tUsing JOINEDTERM information %s of %s\n",
			       ohFormatName( &iterm ), ohFormatName( &ifacet ) ); 
		Message( errmsg );
	    }
	    return;
	} else {
	    /* The joined term could be attached to a forma terminal */
	    if (octGenFirstContainer(&ajoinedterm, OCT_TERM_MASK, &formal_term)==OCT_OK) {
		if (!ElemLookup(cinst, formal_term.contents.term.name, elem)) {
		    InternalError("get_oct_elem(4)");
		}
		return;
	    }		
	}		
    }			

 give_up:

    /* create a new elem, unless it has been created already */
    (void) sprintf(buf, "%s#%s", inst.contents.instance.name, aterm->contents.term.name);
    if (!ElemLookup(cinst, buf, elem)) {
	(void) sprintf(errmsg,"Creating a spurious element for %s",ohFormatName(aterm));
	VerboseMessage(errmsg);
	make_element(elem, cinst, buf);
    }				/* if... */
}				


void  mergeNodes( keepnode, oldnode )
    node *keepnode;
    node *oldnode;
{
    connect *c;

    if ( keepnode != oldnode ) {
	ConnectPowerNodes(keepnode,oldnode);
	/* move stuff on oldnode to keepnode */
	if (oldnode->conns != NIL(connect)) {
	    for (c = oldnode->conns; c->next != NIL(connect); c = c->next) {
		c->node = keepnode;
	    }
	    c->node = keepnode;
	    c->next = keepnode->conns;
	    keepnode->conns = oldnode->conns;
	}
	NodeFree(oldnode);
    }
}

/***********************************************************************
 * MERGE_OCT_ELEMS
 *
 * Parameters:
 *	elem (MusaElement *) - One element. (Input)
 *	subelem (MusaElement *) - the other element. (Input)
 */
static void merge_oct_elems(elem, subelem)
    MusaElement *elem, *subelem;
{
    node *keepnode, *oldnode;


    if (subelem == elem->c.child) {
	Warning("Already a child: this is stupid");
	return;
    }

    /* get node of elem */
    keepnode = ElementToNode(elem);
    oldnode  = ElementToNode(subelem);

    if ( keepnode == oldnode ) {
	Warning( "Already same node");
	return;
    }
    
    if ( ! isSiblingElement( elem, subelem ) ) {
	Warning( "Cannot merge non sibling elements" );
    }

    if ( elem->data & NODE_PTR ) {
	
    } else if ( elem->data & EPARENT_PTR ) {
	subelem->equiv.sibling = elem;
	subelem->data &= ~( NODE_PTR | EPARENT_PTR );
    } else {
	
    }

    mergeNodes( keepnode, oldnode );
    
}	/* merge_oct_elems... */

/***********************************************************************
 * CONNECT_OCT_ELEMS
 * Make subelem a echild of elem (for Oct, this is a global net
 * attached to a sub net).
 * NOT for sim format
 *
 * Parameters:
 *	elem (MusaElement *) - the parent element. (Input)
 *	subelem (MusaElement *) - the child element. (Input)
 */
static void connect_oct_elems(elem, subelem)
    MusaElement *elem, *subelem;
{
    node *keepnode, *oldnode;


    if (subelem == elem->c.child) {
	Warning("Already a child: this is stupid");
	return;
    }

    /* get node of elem */
    keepnode = ElementToNode(elem);
    oldnode  = ElementToNode(subelem);

    
    /*** Make parent-child attachment ***/
    if (elem->c.child ==  NIL(MusaElement)) {
				/* elem has no children, make 'subelem' its child ***/
	elem->c.child = subelem;
	subelem->data &= ~NODE_PTR;
	subelem->data |= EPARENT_PTR;
	subelem->equiv.parent = elem;
    } else {
	subelem->data &= ~(NODE_PTR | EPARENT_PTR);		/* ESIBLING */
	subelem->equiv.sibling = elem->c.child;			/* children of elem is siblings of subelem */
	elem->c.child = subelem;				/*** Make subelem the child of 'elem' ***/
    }	

    mergeNodes ( keepnode, oldnode );
}	/* connect_oct_elems... */

/*************************************************************************
 * READ_OCT_ELEMS
 * Get the elements of an inst (elements are nets)
 *
 * Parameters:
 *	facet (octObject *) - the facet containing elements to read. (Input)
 *	cinst (musaInstance *) - the current instance. (Input)
 */
static void read_oct_elems(facet, cinst)
    octObject *facet;		/* where to get nets from */
    musaInstance *cinst;		/* current musaInstance (where to put nets) */
{
    MusaElement *newelem;
    octObject net;
    octGenerator netgen;

    /* make elements for all nets */
    OH_ASSERT(octInitGenContents(facet, OCT_NET_MASK, &netgen));
    while (octGenerate(&netgen, &net) == OCT_OK) {
	make_element(&newelem, cinst, net.contents.net.name);
	(void) RecognizePowerNode(&net,newelem);
    }				
}				

/************************************************************************
 * READ_OCT_INSTS
 * Recursively incorporate all insts of the oct hierarchy into data
 * structure
 *
 * Parameters:
 *	facet (octObject *) - the facet containing instances to read. (Input)
 *	cinst (musaInstance *) - the current instance. (Input)
 */

static tr_stack *stack = 0;


int readContents( inst, cinst ) 
    octObject* inst;
    musaInstance *cinst;			/* current musaInstance */
{
    octObject  		cfacet;
    octObject		fterm, aterm, net;
    octGenerator	termgen;
    musaInstance	*newmusaInstance;
    MusaElement			*elem, *subelem, *newelem;
    int16			ftermCount;
    
    st_generator* gen;
    char* key;

    if (ohOpenMaster(&cfacet, inst, "contents", "r") < OCT_OK) {
	FatalOctError(ohFormatName(inst));
    }								/* if... */
    OH_ASSERT(myUniqNames(&cfacet, OCT_INSTANCE_MASK|OCT_NET_MASK));
    make_musaInstance(&newmusaInstance, cinst, inst->contents.instance.name);
    read_oct_elems(&cfacet, newmusaInstance);
    ReadOctFTerm2(&cfacet, newmusaInstance);

    /*
     * For each element in newmusaInstance that is exported
     * connect it to the correct element in cinst 
     */
    subelem = NIL(MusaElement);
    st_foreach_item( newmusaInstance->hashed_elements, gen, &key, (char**)&subelem ) {
	/* printf( "Doing subelement %s\n", key ); */
	/* see if there is an fterm on the net */
	if (ohGetByNetName(&cfacet, &net, subelem->name) == OCT_OK) {
	    OH_ASSERT(octInitGenContents(&net,OCT_TERM_MASK,&termgen));
	    ftermCount = 0;
	    while (octGenerate(&termgen, &fterm) == OCT_OK) {
		if ( ! octIdIsNull(fterm.contents.term.instanceId) ) {
		    continue;					/* Skip actual terminals. */
		}

		aterm = fterm;
		OH_ASSERT( octGetByName(inst, &aterm) );	/* Find terminal in instance. */

		if (++ftermCount == 1) {
		    /*** Connect fterm to master instance ***/
		    get_oct_elem(&aterm, cinst, &elem);
		    connect_oct_elems(elem, subelem);
		} else {
		    /* 
		     * If subsequent fterms don't attach to the same
		     * element in the master then connect to new element
		     */
		    get_oct_elem(&aterm, cinst, &newelem);
		    if (elem != newelem) {
			(void) sprintf (errmsg,
"\n\tNet '%s' and net '%s' should be merged,\n\
\tbecause they are electrically equivalent\n\
\tin %s\n\n\
\tI cannot do this at this time.\n\
\t(The alternative to this message is a segmentation fault)\n\n\
\tThis is an unfortunate limitation of this simulator,\n\
\twhich we hope to fix as soon as possible.\n\
\tTo continue, please change your bdnet files\n\
\tso that the two nets are merged into one.",
					elem->name,newelem->name,
					ohFormatName(&cfacet) );
			FatalError (errmsg);
			/* merge_oct_elems(elem, newelem); */
		    }	
		}	
	    }		
	} else {
	    /* printf( "does this ever happen?" ); */ /* Yes it does */
	    if (ElemLookup(cinst, subelem->name, &elem)) {
		connect_oct_elems(elem, subelem);
	    }
	}		
    }	
    tr_push(stack);
    tr_add_transform(stack, &inst->contents.instance.transform, 0);
    read_oct_insts(&cfacet, newmusaInstance);		/* Recursive call. */
    tr_pop(stack);
}



static void read_oct_insts(facet, cinst)
    octObject		*facet;			/* facet containing insts to read */
    musaInstance	*cinst;			/* current musaInstance */
{
    octObject		ifacet;				/* master facets of an inst */
    octObject		inst;
    octObject		celltype;
    octObject		prop;
    octObject		ibag;
    octGenerator	instgen;
    int				match, i;

    if (!stack) {
	stack = tr_create();
    }

    OH_ASSERT(ohGetOrCreateBag(facet, &ibag, "INSTANCES"));

    /*** Generate list of just instances from INSTANCES bag ***/
    OH_ASSERT(octInitGenContents(&ibag, OCT_INSTANCE_MASK, &instgen));
    /*** Read instances ***/
    while(octGenerate(&instgen, &inst) == OCT_OK) {
	/*
	 * transform instance coordinates so leds and 7-seg 
	 * displays work 
	 */
	tr_transform(stack,
		     &inst.contents.instance.transform.translation.x,
		     &inst.contents.instance.transform.translation.y);

	parent_inst = cinst;
	/*** Open interface facet ***/
	if (ohOpenMaster(&ifacet, &inst, "interface", "r") < OCT_OK) {
	    FatalOctError(inst.contents.instance.master);
	}							/* if... */
	inter_facet = &ifacet;
	if (ohGetByPropName(&ifacet, &celltype, "CELLTYPE") != OCT_OK) {
	    celltype.contents.prop.value.string =  "";
	}							/* if... */

	/* A cell can satisfy more than 1 reading routine
	 * Example: bidirectional pad is both a combinational cell as well as
	 * a tristate driver
	 */
	match = 0;
	for (i = 0; i < number_of_reads; i++) {
	    match += read_routine[i](&ifacet,&inst,celltype.contents.prop.value.string);
	}							/* for... */
	if (match) {
	    if (match > 1) {
		(void) sprintf(errmsg,"\n\t%s\n\tsatisfies %d readers\n\t(but I cannot say which)",
			       ohFormatName(&inst), match);
		VerboseMessage(errmsg);
	    }	
	    continue;
	}

	if (ohGetByPropName(&ifacet,&prop,"CELLCLASS") != OCT_OK) {
	    (void) sprintf(errmsg,"CELLCLASS property is missing in %s.\n\tI am assuming it is MODULE",
			   ohFormatName(&ifacet));
	    VerboseWarning(errmsg);
	} else {
	    if (StrPropEql(&prop,"LEAF")) {
		(void) sprintf(errmsg,"Non simulatable %s", ohFormatName(&inst));
		Warning(errmsg);
		continue;
	    }	
	}	
	
	readContents( &inst, cinst );
	/* OH_ASSERT(octCloseFacet(&cfacet)); */
    }								/* while... */
}								/* read_oct_inst */

/************************************************************************
 * READ_OCT
 * Read Oct File
 *
 * Parameters:
 *	rootfacet (octObject *) - the root facet. (Input)
 */
void read_oct(rootfacet)
    octObject *rootfacet;
{
    OH_ASSERT(myUniqNames(rootfacet, OCT_INSTANCE_MASK|OCT_NET_MASK));

    /* make top of hierarchy */
    make_musaInstance(&topmusaInstance, NIL(musaInstance), NIL(char));

    read_oct_elems(rootfacet, topmusaInstance); /* read all the nets */

    ReadOctFTerm(rootfacet, topmusaInstance);

    read_oct_insts(rootfacet, topmusaInstance);
}				/* read_oct... */

/**************************************************************************
 * MOS_READ
 * Try to see if we can read a MOSFET from Oct
 * Return 0 if this is not a mosfet
 *
 * Parameters:
 *	ifacet (octObject *) - interface facet. (Input)
 *	inst (octObject *) - contents facet. (Input)
 *	celltype (char *) - the cell type. (Input)
 *
 * Return Value:
 *	TRUE, success.
 *	FALSE, unsuccessful.
 */
int16 mos_read(ifacet, inst, celltype)
	octObject	*ifacet, *inst;
	char		*celltype;
{
    octObject	prop, aterm;
    octObject	moslength, moswidth;
    musaInstance		*newmusaInstance;
    MusaElement		*elem;
    node		*terms[3];
    int16		source_in = FALSE, drain_in = FALSE;
    int16		always_on;
    deviceType	type;

    if (strcmp(celltype, "MOSFET") != 0) {
	return FALSE;
    }				/* if... */

    /* identify device type */
    if (ohGetByPropName(ifacet, &prop, "MOSFETTYPE") != OCT_OK) {
	(void) sprintf(errmsg, "missing MOSFETTYPE in  %s:%s",inst->contents.instance.master,inst->contents.instance.view);
	FatalError(errmsg);
    }				/* if... */
    if (strcmp(prop.contents.prop.value.string, "NDEP") == 0) {
	type = DEVTYPE_WEAK_NMOS;
	always_on = TRUE;
    } else if (strcmp(prop.contents.prop.value.string, "PDEP") == 0) {
	type = DEVTYPE_WEAK_PMOS;
	always_on = TRUE;
    } else if (strcmp(prop.contents.prop.value.string, "NENH") == 0) {
	type = DEVTYPE_NMOS;
	always_on = FALSE;
    } else if (strcmp(prop.contents.prop.value.string, "PENH") == 0) {
	type = DEVTYPE_PMOS;
	always_on = FALSE;
    }				/* if... */
    if ((ohGetByPropName(ifacet, &prop, "BDSIM_WEAK") == OCT_OK) || (ohGetByPropName(inst, &prop, "BDSIM_WEAK") == OCT_OK) ) {
	(void) sprintf(msgbuf,"%s: BDSIM_WEAK is out of date, use MUSA_WEAK.",ohGetName(inst));
	Warning(msgbuf);
	if (type == DEVTYPE_PMOS) {
	    type = DEVTYPE_WEAK_PMOS;
	}			/* if... */
	if (type == DEVTYPE_NMOS) {
	    type = DEVTYPE_WEAK_NMOS;
	}			/* if... */
    }				/* if... */
    if ((ohGetByPropName(ifacet, &prop, "MUSA_WEAK") == OCT_OK) || (ohGetByPropName(inst, &prop, "MUSA_WEAK") == OCT_OK) ) {
	if (type == DEVTYPE_PMOS) {
	    type = DEVTYPE_WEAK_PMOS;
	}			/* if... */
	if (type == DEVTYPE_NMOS) {
	    type = DEVTYPE_WEAK_NMOS;
	}			/* if... */
    }				/* if... */
    if ((ohGetByPropName(inst, &moswidth, "MOSFETWIDTH") != OCT_OK) && (ohGetByPropName(ifacet, &moswidth, "MOSFETWIDTH") != OCT_OK)) {
	(void) sprintf(errmsg, "missing MOSFETWIDTH in  %s:%s",inst->contents.instance.master,inst->contents.instance.view);
	FatalError(errmsg);
    }				/* if... */
    if ((ohGetByPropName(inst, &moslength, "MOSFETLENGTH") != OCT_OK) && (ohGetByPropName(ifacet, &moslength, "MOSFETLENGTH") != OCT_OK)) {
	(void) sprintf(errmsg, "missing MOSFETLENGTH in  %s:%s",inst->contents.instance.master,inst->contents.instance.view);
	FatalError(errmsg);
    }				/* if... */
#ifdef notdef
    (void) fprintf(stderr, "W/L is %g/%g (%g), weak_thresh is %g\n",
		   moswidth.contents.prop.value.real,
		   moslength.contents.prop.value.real,
		   moswidth.contents.prop.value.real/moslength.contents.prop.value.real,
		   weak_thresh);
#endif
    if (moslength.contents.prop.value.real > 0.0) {
	if (((moswidth.contents.prop.value.real / moslength.contents.prop.value.real) < weak_thresh)) {
	    if (type == DEVTYPE_PMOS) {
		type = DEVTYPE_WEAK_PMOS;
	    }			/* if... */
	    if (type == DEVTYPE_NMOS) {
		type = DEVTYPE_WEAK_NMOS;
	    }			/* if... */
	}			/* if... */
    }				/* if... */

    /* create the musaInstance */
    make_musaInstance(&newmusaInstance, parent_inst, inst->contents.instance.name);
    if (ohGetByTermName(inst, &aterm, "SOURCE") != OCT_OK) {
	(void) sprintf(errmsg, "MOSFET %s has no SOURCE", ohFormatName(inst));
	FatalError(errmsg);
    }				/* if... */
    if (ohGetByPropName(&aterm, &prop, "BDSIM_IN") == OCT_OK) {
	Warning("Obsolete prop BDSIM_IN: you should use DIRECTION (INPUT or OUTPUT)");
	source_in = TRUE;
    }				/* if... */
    if (ohGetByPropName(&aterm, &prop, "DIRECTION") == OCT_OK) {
	if (StrPropEql(&prop,"INPUT")) {
	    source_in = TRUE;
	} else if (StrPropEql(&prop,"OUTPUT")) {
	    drain_in = TRUE;
	} else {
	    (void) sprintf(errmsg,"Illegal value (%s) for DIRECTION in SOURCE of %s",prop.contents.prop.value.string,ohFormatName(inst));
	    Warning(errmsg);
	}			/* if... */
    }				/* if... */
    get_oct_elem(&aterm, parent_inst, &elem);
    terms[0] = ElementToNode(elem);
    if (ohGetByTermName(inst, &aterm, "DRAIN") != OCT_OK) {
	(void) sprintf(errmsg, "MOSFET %s has no DRAIN", ohFormatName(inst));
	FatalError(errmsg);
    }				/* if... */
    if (ohGetByPropName(&aterm, &prop, "BDSIM_IN") == OCT_OK) {
	Warning("Obsolete prop BDSIM_IN: you should use DIRECTION (INPUT or OUTPUT)");
	drain_in = TRUE;
    }				/* if... */
    if (ohGetByPropName(&aterm, &prop, "DIRECTION") == OCT_OK) {
	if (StrPropEql(&prop,"INPUT")) {
	    drain_in = TRUE;
	} else if (StrPropEql(&prop,"OUTPUT")) {
	    source_in = TRUE;
	} else {
	    (void) sprintf(errmsg,"Illegal value (%s) for DIRECTION in DRAIN of %s",prop.contents.prop.value.string,ohFormatName(inst));
	    Warning(errmsg);
	}			/* if... */
    }				/* if... */
    get_oct_elem(&aterm, parent_inst, &elem);
    terms[1] = ElementToNode(elem);
    if (ohGetByTermName(inst, &aterm, "GATE") != OCT_OK) {
	(void) sprintf(errmsg, "MOSFET `%s' (%s:%s) has no GATE\n",
		       inst->contents.instance.name,
		       inst->contents.instance.master,
		       inst->contents.instance.view);
	FatalError(errmsg);
    }				/* if... */
    get_oct_elem(&aterm, parent_inst, &elem);
    terms[2] = ElementToNode(elem);

    if (drain_in && source_in) {
	FatalError("Cannot have both SOURCE and DRAIN marked as INPUTS");
    }				/* if... */

    make_conns(newmusaInstance, terms, type, always_on, source_in, drain_in);


    /* Print stuff if reading large circuits */
    tcount++;			/* Count transistors. */
    if (update && (tcount/1000*1000 == tcount)) {
	(void) fprintf(stderr, "%d ", tcount);
    }		
    return TRUE;
}				/* mos_read... */

/*************************************************************************
 * LOGIC_READ
 * Try to see if we can read a logic gate from Oct
 * Return 0 if this is not a logic gate.
 *
 * Parameters:
 *	ifacet (octObject *) - interface facet. (Input)
 *	inst (octObject *) - contents facet. (Input)
 *	celltype (char *) - the cell type. (Input)
 *
 * Return Value:
 *	TRUE, success.
 *	FALSE, unsuccessful.
 */
int16 logic_read(ifacet, inst, celltype)
	octObject		*ifacet, *inst;
	char			*celltype;
{
    octObject		prop, aterm;
    octGenerator	termgen;
    int				count = 0; /* no logic function yet */

    if (strcmp(celltype, "COMBINATIONAL") != 0) {
	return(FALSE);
    }				/* if... */
    prop.type = OCT_PROP;
    prop.contents.prop.name = "LOGICFUNCTION";
    OH_ASSERT(octInitGenContents(inst, OCT_TERM_MASK, &termgen));
    while (octGenerate(&termgen, &aterm) == OCT_OK) {
	if (octGetByName(&aterm, &prop) == OCT_OK) {
	    read_oct_logic(prop.contents.prop.value.string,&aterm, inst, ifacet, parent_inst, ++count);
	}
    }
    if (count == 0) {
	OH_ASSERT(octInitGenContents(ifacet, OCT_TERM_MASK, &termgen));
	while (octGenerate(&termgen, &aterm) == OCT_OK) {
	    if (octGetByName(&aterm, &prop) == OCT_OK) {
		read_oct_logic(prop.contents.prop.value.string,&aterm, inst, ifacet, parent_inst, ++count);
	    }	
	}
    }		
    if (count == 0) {
	(void) sprintf(errmsg,"No LOGIC terminal in %s",ohFormatName(inst));
	Warning(errmsg);
    }
    return (count != 0);
}


