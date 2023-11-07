
/***********************************************************************
 * Process OCT Logic Gates 
 * Filename: logic.c
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
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define LOGIC_MUSA
#include "musa.h"

/* types for logic->type */
#define ZERO	0
#define ONE		1
#define INPUT	2
#define AND		3
#define OR		4

/* structure to hold gates (temporarily) */
typedef struct logicnode_struct logicnode, *logicnodeptr;
struct logicnode_struct {
	short type;
	short is_inv;		/* output is active low */
	union {
		short conn;			/* which connect (if INPUT) */
		logicnode *list;	/* list contained in AND or OR */
	} u;
	logicnode *next;	/* fellow input to AND or OR */
}; /* logicnode_struct... */

/***********************************************************************
 * MAKE_LOGICNODE
 */
make_logicnode(log)
	logicnode **log;
{
    *log = ALLOC(logicnode, 1);
    (*log)->is_inv = 0;
    (*log)->next = NIL(logicnode);
} /* make_logicnode... */

#ifndef NO_OCT
int count_lits();
static octObject outterm, *loginst, *infacet;
static musaInstance *parinst;
static int current_conn;		/* which connect we're on */
static musaInstance *newmusaInstance;

/*
 * create logic musaInstances with given functions
 */
read_oct_logic(function, output, inst, ifacet, cinst, suffix)
	char *function;			/* logic function */
	octObject *output;		/* output aterm (or interface term) */
	octObject *inst;		/* logic instance */
	octObject *ifacet;		/* interface of inst */
	musaInstance *cinst;			/* parent musaInstance */
	int suffix;				/* appended if two gates from one Oct inst */
{
    logicnode *log;
    char buffer[1024];
    int lits;			/* number of literals in the function */

    /* get actual term for output */
    outterm = *output;
    if (octIdIsNull(output->contents.term.instanceId)) {
	OH_ASSERT(octGetByName(inst, &outterm));
    }				/* if... */
    loginst = inst;		/* global variables */
    infacet = ifacet;		/* global variables */
    parinst = cinst;		/* global variables */

    /* Create a unique name for this logic instance */
    (void) sprintf(buffer, "%s#logic#%d", inst->contents.instance.name, suffix);

    lits = count_lits(function);
    make_leaf_inst(&newmusaInstance, DEVTYPE_LOGIC_GATE, buffer, lits+1);
    make_logicnode(&log);
    newmusaInstance->userData = (char *) log;
    connect_term(newmusaInstance, &outterm, 0);

    current_conn = 0;
    /* parse the function */
    if (!read_function(&function, log)) {
	(void) sprintf (errmsg, "Illegal logic function (%s) in %s",function,inst->contents.instance.name);
	FatalError(errmsg);
    }				/* if... */
}				/* read_oct_logic... */

/********************************************************************
 * COUNT_LITS
 * count the literals in the function
 */
int count_lits(function)
	char *function;
{
    int i = 0;
    register char *p;

    p = function;
    while (*p != '\0') {
	while (*p == ' ') p++;
	if (*p == '(') {
	    p++;
	    while (*p == ' ') p++;
	    if (*p != '\0') {
		p++;		/* skip !,0,1,+,* */
	    }	
	} else {
	    while ((*p == ' ') || (*p == ')')) p++;
	    if ((*p != '(') && (*p != '\0')) {
		i++;
		/* skip over the literal */
		while ((*p != ')') && (*p != '\0') && (*p != ' ')) p++;
	    }	
	}	
    }		
    return i;
}				/* count_lits... */

/*********************************************************************
 * GRAB_ELEM
 * get a term name, look up element and put it in log
 * return false if problem
 */
int grab_elem(function, log)
    char **function;
    logicnode *log;
{
    octObject term;
    register char *p;
    char buffer[1024];
    int i = 0;

    p = *function;
    while ((*p != ' ') && (*p != ')') && (*p != '\0')) {
	buffer[i++] = *(p++);
    }				/* while... */
    buffer[i] = '\0';
    *function = p;
    log->type = INPUT;
    log->u.conn = ++current_conn;

    if (ohGetByTermName(loginst, &term, buffer) != OCT_OK) return 0;
    connect_term(newmusaInstance, &term, current_conn);
    return 1;
}				/* GRAB_ELEM... */

/***********************************************************************
 * READ_FUNCTION
 * read a parenthesized function, return false on syntax error
 */
read_function(function, log)
	char **function;
	logicnode *log;
{
    logicnode *csublog = NIL(logicnode); /* child log of log */
    char *p;

    p = *function;
    while (*p == ' ') p++;
    if (*p == '(') {
	p++;			/* skip paren */
    } else {
	return grab_elem(&p, log);
    }				/* if... */
    while (*p == ' ') p++;
    switch (*(p++)) {
    case '0':
	log->type = ZERO;
	break;
    case '1':
	log->type = ONE;
	break;
    case '!':
	log->is_inv = !log->is_inv;
	while (*p == ' ') p++;
	if (*p == '(') {
	    if (!read_function(&p, log)) return 0;
	} else {
	    if (!grab_elem(&p, log)) return 0;
	}			/* if... */
	break;
    case '*':
	log->type = AND;
	break;
    case '+':
	log->type = OR;
	break;
    default:
	return 0;
    }				/* switch... */
    if ((log->type == AND) || (log->type == OR)) {
	while (1) {
	    while (*p == ' ') p++;
	    if (*p == ')') break;
	    if (csublog == NIL(logicnode)) {
		make_logicnode(&log->u.list);
		csublog = log->u.list;
	    } else {
		make_logicnode(&csublog->next);
		csublog = csublog->next;
	    }			/* if... */
	    if (*p == '(') {
		if (!read_function(&p, csublog)) return 0;
	    } else {
		if (!grab_elem(&p, csublog)) return 0;
	    }			/* if... */
	}			/* while... */
    }				/* if... */

    while (*p == ' ') p++;
    if (*(p++) != ')') return 0;
    *function = p;
    return 1;
}				/* read_function... */
#endif /* NO_OCT */


/*********************************************************************
 * LINFO
 * print logic function for a list
 * return highest numbered input
 */
linfo(log, numin)
    logicnode *log;
    int *numin;		/* highest numbered input seen */
{
    logicnode *l;

    for (l = log; l != NIL(logicnode); l = l->next) {
	if (l->is_inv) {
	    MUSAPRINT(" (!");
	}			/* if... */
	switch(l->type) {
	case ZERO:
	    MUSAPRINT(" (0)");
	    break;
	case ONE:
	    MUSAPRINT(" (1)");
	    break;
	case INPUT:
	    (void) sprintf(msgbuf, " INPUT%d", l->u.conn);
	    MUSAPRINT(msgbuf);
	    *numin = l->u.conn;
	    break;
	case AND:
	    MUSAPRINT(" (*");
	    linfo(l->u.list, numin);
	    MUSAPRINT(")");
	    break;
	case OR:
	    MUSAPRINT(" (+");
	    linfo(l->u.list, numin);
	    MUSAPRINT(")");
	    break;
	}			/* switch... */
	if (l->is_inv) {
	    MUSAPRINT(")");
	}			/* if... */
    }				/* for... */
}				/* linfo... */

/**************************************************************************
 * LOGIC_INFO
 * special routine to give ii info for logic gates
 */
logic_info(inst)
	musaInstance *inst;
{
	int numin = 0;		/* highest numbered input */

	MUSAPRINT("    LOGICFUNCTION = ");
	linfo((logicnode *) inst->userData, &numin);
	MUSAPRINT("\n");
	ConnectInfo(inst, numin + 1);
} /* logic_info... */

static int *partial;	/* stack of partial results for logic */
static logicnode **lstack;	/* stack of logicnodes */
static int psize = 0;	/* current size of partial array */

/* quick table to accomplish AND and OR functions
 * bits <1:0> are one value (L_o L_i or L_x), bits <3:2> are the other
 * value, and bit <4> is 1 for AND and 0 for OR.
 * If STOP is true, evaluation need not be continued
 */
#define STOP 32
#define R_0	4	/* result of 0 */
#define R_1	8	/* result of 1 */
#define R_X	12	/* result of X */
#define ISAND	16	/* function is AND */
static int logic_lookup[] = {
    0, 0, 0, 0,
    0, R_0, R_1|STOP, R_X,
    0, R_1|STOP, R_1|STOP, R_1|STOP,
    0, R_X, R_1|STOP, R_X,
    0, 0, 0, 0,
    0, R_0|ISAND|STOP, R_0|ISAND|STOP, R_0|ISAND|STOP,
    0, R_0|ISAND|STOP, R_1|ISAND, R_X|ISAND,
    0, R_0|ISAND|STOP, R_X|ISAND, R_X|ISAND,
};


/*************************************************************************
 * LOGIC
 * simulate a logic gate
 * Take great pains to avoid the overhead of recursive calls
 */
logic(inst)
    musaInstance *inst;
{
    int plevel = 0;		/* level in the partial stack */
    int temp;

    if (psize == 0) {
	psize = 50;
	partial = ALLOC(int, 50);
	lstack = ALLOC(logicnodeptr, 50);
    }				/* if... */
    lstack[0] = (logicnode *) inst->userData;
    /* root level will be OR of one element */
    partial[0] = R_0;

    while (plevel >= 0) {
	while (lstack[plevel] != NIL(logicnode)) {
	    if ((partial[plevel] & STOP) != 0) break;
	    switch(lstack[plevel]->type) {
	    case ZERO:
		if (lstack[plevel]->is_inv) {
		    partial[plevel] = logic_lookup[L_i | partial[plevel]];
		} else {
		    partial[plevel] = logic_lookup[L_o | partial[plevel]];
		}		/* if... */
		lstack[plevel] = lstack[plevel]->next;
		break;

	    case ONE:
		if (lstack[plevel]->is_inv) {
		    partial[plevel] = logic_lookup[L_o | partial[plevel]];
		} else {
		    partial[plevel] = logic_lookup[L_i | partial[plevel]];
		}		/* if... */
		lstack[plevel] = lstack[plevel]->next;
		break;

	    case INPUT:
		temp = inst->connArray[lstack[plevel]->u.conn].node->voltage;
		temp &= L_VOLTAGEMASK;
		if (lstack[plevel]->is_inv) {
		    if (temp == L_LOW) {
			temp = L_HIGH;
		    } else if (temp == L_HIGH) {
			temp = L_LOW;
		    }		/* if... */
		}		/* if... */
		partial[plevel] = logic_lookup[temp | partial[plevel]];
		lstack[plevel] = lstack[plevel]->next;
		break;

	    case AND:
		partial[plevel+1] = R_1|ISAND;
		lstack[plevel+1] = lstack[plevel]->u.list;
		plevel++;
		/* make sure there is no chance of lstack overrun */
		if (plevel+1 >= psize) {
		    psize += 50;
		    partial = REALLOC(int, partial, psize);
		    lstack = REALLOC(logicnodeptr, lstack, psize);
		}		/* if... */
		break;

	    case OR:
		partial[plevel+1] = R_0;
		lstack[plevel+1] = lstack[plevel]->u.list;
		plevel++;
		/* make sure there is no chance of lstack overrun */
		if (plevel+1 >= psize) {
		    psize += 50;
		    partial = REALLOC(int, partial, psize);
		    lstack = REALLOC(logicnodeptr, lstack, psize);
		}		/* if... */
		break;
	    }			/* switch... */
	}
	/* done with this level */
	partial[plevel] = (partial[plevel] >> 2) & L_VOLTAGEMASK;
	if ((plevel > 0) && lstack[plevel-1]->is_inv) {
	    if (partial[plevel] == L_o) {
		partial[plevel] = L_i;
	    } else if (partial[plevel] == L_i) {
		partial[plevel] = L_o;
	    }			/* if... */
	}			/* if... */
	if (plevel > 0) {
	    partial[plevel-1] = logic_lookup[partial[plevel] |
					     partial[plevel-1]];
	    lstack[plevel-1] = lstack[plevel-1]->next;
	}			/* if... */
	plevel--;
    }				/* while... */
    drive_output(&(inst->connArray[0]), partial[0]);
}				/* logic... */
