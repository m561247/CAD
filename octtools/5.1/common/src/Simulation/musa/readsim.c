/***********************************************************************
 * Routines to read sim file structure.
 * Filename: readsim.c
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
 *	void init_node() - initalize node.
 *	void make_musaInstance() - make instance.
 *	void equiv_elem() - make equivalent elements.
 *	void make_conns() - make transistor connections.
 *	void make_element() - make element.
 *	void read_sim() - read sim file.
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define READSIM_MUSA
#include "musa.h"

/***************************************************************************
 * INIT_NODE
 * Initialize node.
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 */
void init_node(theNode, element)
    node *theNode;
    MusaElement* element;
{
    static int  countnum = 0;

    theNode->schedule = 0;
    theNode->conns = NIL(connect);
    theNode->highStrength = 255;
    theNode->lowStrength = 255;
    theNode->numberOfHighDrivers = 0;
    theNode->numberOfLowDrivers = 0; 
    theNode->data = 0;
    theNode->voltage = L_x;
    theNode->count = countnum++ % 256;			/* make counts not be synced */
    theNode->rep = element;
    theNode->is_supply = 0;
    theNode->is_ground = 0;
}

/****************************************************************************
 * MAKE_MUSAINSTANCE
 * Define a fresh musaInstance (assume internal node for now)
 *
 * Parameters:
 *	inst (musaInstance **) - the new instance. (Output)
 *	parent (misnt *) - the parent instance. (Input)
 *	name (char *) - the name of the new instance. (Input)
 */
void make_musaInstance(inst, parent, name)
	musaInstance	**inst;
	musaInstance	*parent;
	char	*name;
{
    *inst = MusaInstanceAlloc();
    (*inst)->schedule = 0; 
    (*inst)->userData = 0;
    (*inst)->terms = 0;
    (*inst)->connArray = 0;
    (*inst)->type = DEVTYPE_MODULE;
    (*inst)->data = 0;
    (*inst)->child = NIL(musaInstance);
    (*inst)->elems = NIL(MusaElement);
    (*inst)->hashed_children = 0;
    (*inst)->hashed_elements = 0;

    if (parent == NIL(musaInstance)) {
	(*inst)->name = NIL(char); 			/* This is the root musaInstance */
	(*inst)->parent = NIL(musaInstance);
    } else {
	(*inst)->name = util_strsav( name );		/* This is a normal inst */
	InstInsert(parent, *inst);
    }	
}				/* make_musaInstance... */

/*************************************************************************
 * EQUIV_ELEM
 * Add elem2 to elem1's list of equiv's.
 *
 * Parameters:
 *	el1, el2 (element *) - the two equivalent elements.
 */
void equiv_elem(el1, el2)
	MusaElement *el1, *el2;
{
    NodeFree(el2->equiv.node);	/* should be nothing attached to node */
    el2->equiv.node = el1->equiv.node;
    el2->c.equiv = el1->c.equiv;
    el1->c.equiv = el2;
} /* equiv_elem... */

/*************************************************************************
 * MAKE_CONNS
 * Set up the connections for transistor instances.
 *
 * Parameters:
 *	newmusaInstance (musaInstance *) - the transistor instance. (Input)
 *	terms (node *[3]) - the connection nodes. (Output)
 *	type (deviceType) - the device type. (Input)
 *	on (int) - is the transistor "always on". (Input)
 *	source_in, drain_in (int) - transistor direction. (Input)
 */
void make_conns(newmusaInstance, terms, type, on, source_in, drain_in)
    musaInstance	*newmusaInstance;
    node		*terms[3];
    deviceType		type;						/* nmos, pmos, etc. */
    int			on;						/* is transistor "always on" */
    int			source_in, drain_in;				/* transistor direction */
{
    int			i, numterms = 3;

    if (source_in && drain_in) {
	(void) sprintf(errmsg, "GATE is \"%s\"\n", terms[2]->rep->name);
	Warning( errmsg );
	FatalError("Both source and drain of transistor are marked in$");
    }			

    /* check for pullup or pulldown */
    if (!no_pulls) {
	if (IsGround(terms[0])) {
	    if (IsSupply(terms[1])) {
		if ((terms[2]->rep != NIL(MusaElement)) && (terms[2]->rep->name != NIL(char))) {
		    (void) sprintf(errmsg, "GATE is \"%s\"\n", terms[2]->rep->name);
		    Warning( errmsg );
		}
		FatalError("One transistor between power and ground");
	    }
	    terms[0] = terms[1];
	    terms[1] = terms[2];
	    numterms = 2;
	    type = FindPullDown(type);
	} else if (IsSupply(terms[0])) {
	    if (IsSupply(terms[1])) {
		if ((terms[2]->rep != NIL(MusaElement)) && (terms[2]->rep->name != NIL(char))) {
		    (void) sprintf(errmsg, "GATE is %s\n", terms[2]->rep->name);
		    Warning( errmsg );
		}		
		FatalError("One transistor between power and ground");
	    }			
	    terms[0] = terms[1];
	    terms[1] = terms[2];
	    numterms = 2;
	    type = FindPullUp(type);
	} else if (IsGround(terms[1])) {
	    terms[1] = terms[2];
	    numterms = 2;
	    type = FindPullDown(type);
	} else if (IsSupply(terms[1])) {
	    terms[1] = terms[2];
	    numterms = 2;
	    type = FindPullUp(type);
	}			/* if... */
    }				/* if... */

    if (on) {
	numterms--;
	newmusaInstance->data |= 1;	/* subtype = always on */
    } else {
	newmusaInstance->data |= 3;	/* regular transistor */
    } 
    newmusaInstance->type = type;
    newmusaInstance->connArray = ConnAlloc(numterms);
    newmusaInstance->terms = numterms;
    for (i=0; i<numterms; i++) {
	newmusaInstance->connArray[i].device = newmusaInstance;
	newmusaInstance->connArray[i].ttype = deviceTable[(int) type].term[i].ttype;
	newmusaInstance->connArray[i].highStrength =
	  newmusaInstance->connArray[i].lowStrength = 255;
	newmusaInstance->connArray[i].hout = newmusaInstance->connArray[i].lout = 0;
	newmusaInstance->connArray[i].node = terms[i];
	newmusaInstance->connArray[i].next = terms[i]->conns;
	terms[i]->conns = &(newmusaInstance->connArray[i]);
    }				/* for... */
    if (MosTransistor(newmusaInstance->type)) {
	if (source_in) {
	    newmusaInstance->connArray[1].ttype = TERM_OUTPUT;
	}			/* if... */
	if (drain_in) {
	    newmusaInstance->connArray[0].ttype = TERM_OUTPUT;
	}			/* if... */
    }				/* if... */
}				/* make_conns... */

/***************************************************************************
 * MAKE_ELEMENT
 * define a fresh element
 * Each new element is alloced a node.  May be merged away.
 *
 * Parameters:
 *	elem (MusaElement **) - the new element. (Output)
 *	parent (musaInstance *) - the parent instance. (Input)
 *	name (char *) - the name of the new element. (Input)
 */
void make_element(elem, parentInstance, name)
    MusaElement **elem;
    musaInstance *parentInstance;
    char *name;
{
    *elem = ElemAlloc();
    (*elem)->name = util_strsav( name );
    (*elem)->data = NODE_PTR;
    (*elem)->equiv.node = NodeAlloc();
    ElemInsert(parentInstance, *elem);

    init_node((*elem)->equiv.node, *elem);


    if (sim_format) {
	(*elem)->c.equiv = NIL(MusaElement);
    } else {
	(*elem)->c.child = NIL(MusaElement);
    }				
}				/* make_element... */

#ifndef RPC_MUSA
node	*VddNodes, *GndNodes;
char	*Vdd_name = "Vdd";
char	*GND_name = "GND";

/***************************************************************************
 * READ_SIM
 * Read sim file.
 * 
 * Parameters:
 *	None.
 */
void read_sim()
{
    musaInstance		*newmusaInstance;
    MusaElement		*elem, *elem2;
    char		*token, *first_letter, buf[512], *prop;
    short		devtype;
    static int	dname = 0;	/* give transistors numbers for names */
    node		*terms[3]; /* drain, source, gate */
    int			always_on;
    int			source_in, drain_in;
    int			i, length, width;

    /* make top of hierarchy */
    if (topmusaInstance == NIL(musaInstance)) {
	make_musaInstance(&topmusaInstance, NIL(musaInstance), NIL(char));
    }				/* if... */

    /* define VddNodes and GndNodes nodes */
    if (!ElemLookup(topmusaInstance, Vdd_name, &elem)) {
	make_element(&elem, topmusaInstance, Vdd_name);
	VddNodes = elem->equiv.node;
    }				/* if... */
    if (!ElemLookup(topmusaInstance, GND_name, &elem)) {
	make_element(&elem, topmusaInstance, GND_name);
	GndNodes = elem->equiv.node;
    }				/* if... */

    while ((first_letter = yylex()) != NIL(char)) {
	source_in = drain_in = 0;

	/* identify device type or other command */
	switch (*first_letter) {
	case 'n':
	case 'e':
	    devtype = 1;
	    always_on = 0;
	    break;
	case 'p':
	    devtype = 3;
	    always_on = 0;
	    break;
	case 'd':
	    devtype = 2;
	    always_on = 1;
	    break;
	case '=':
	    devtype = -1;
	    token = yylex();
	    if (!ElemLookup(topmusaInstance, token, &elem)) {
		/* make a new element */
		make_element(&elem, topmusaInstance, token);
	    }			
	    token = yylex();
	    if (ElemLookup(topmusaInstance, token, &elem2)) {
		(void) sprintf(errmsg, "Warning: aliased name %s already defined: alias ignored\n", token);
		Warning( errmsg );
		yylf();
		break;
	    }			
	    /* make a new element */
	    make_element(&elem2, topmusaInstance, token);
	    equiv_elem(elem, elem2);
	    yylf();
	    break;
	default:		/* ignore it */
	    devtype = -1;
	    yylf();
	    break;
	}			/* switch... */
	if (devtype == -1) continue;
	(void) sprintf(buf, "%d", ++dname);
	make_musaInstance(&newmusaInstance, topmusaInstance, buf);

	i = 2;
	{			/* gate */
	    token = yylex();
	    if (!ElemLookup(topmusaInstance, token, &elem)) {
		/* make a new element */
		make_element(&elem, topmusaInstance, token);
	    }			/* if... */
	    terms[i] = elem->equiv.node;
	}
	for (i=0; i<=1; i++) {	/* source, drain */
	    token = yylex();
	    if (!ElemLookup(topmusaInstance, token, &elem)) {
		/* make a new element */
		make_element(&elem, topmusaInstance, token);
	    }			/* if... */
	    terms[i] = elem->equiv.node;
	}			/* for... */

	length = atoi(yylex());
	width = atoi(yylex());
	if (((double) width / length < weak_thresh) && ((devtype == 1) || (devtype == 3))) {
	    /* make a weak device */
	    devtype++;
	}			/* if... */	
	/*** skip over x and y ***/
	(void) yylex(); 
	(void) yylex();
	token = yylex();
	if (*token == 'g') {
	    token = &(token[2]);
	    do {
		prop = token;
		token = strchr(token, ',');
		if (token != NIL(char)) {
		    *(token++) = '\0';
		}		/* if... */
		if ((strcmp("weak", prop) == 0) && ((devtype == 1) || (devtype == 3))) {
		    /* make a weak device */
		    devtype++;
		}		/* if... */
	    } while (token != NIL(char));
	    token = yylex();
	}			/* if... */
	if (*token == 's') {
	    token = &(token[2]);
	    do {
		prop = token;
		token = strchr(token, ',');
		if (token != NIL(char)) {
		    *(token++) = '\0';
		}		/* if... */
		if (((strcmp("in", prop) == 0) || (strcmp("Cr:In", prop) == 0)) && (devtype < 5)) {
		    /* make a unidirectional device */
		    source_in = 1;
		}		/* if... */
	    } while (token != NIL(char));
	    token = yylex();
	}			/* if... */
	if (*token == 'd') {
	    token = &(token[2]);
	    do {
		prop = token;
		token = strchr(token, ',');
		if (token != NIL(char)) {
		    *(token++) = '\0';
		}		/* if... */
		if (((strcmp("in", prop) == 0) || (strcmp("Cr:In", prop) == 0)) && (devtype < 5)) {
		    /* make a unidirectional device */
		    drain_in = 1;
		}		/* if... */
	    } while (token != NIL(char));
	    token = yylex();
	}			/* if... */
	make_conns(newmusaInstance, terms, devtype, always_on, source_in, drain_in);
	tcount++;
	if (update && (tcount/1000*1000 == tcount)) {
	    (void) fprintf(stderr, "%d ", tcount);
	    (void) fflush(stderr);
	}			
	yylf();
    }				
}				/* read_sim... */
#endif

