/***********************************************************************
 * Handle allocation of misc structures.
 * Filename: alloc.c
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
 *	Rodney Lai					Commented				1989
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define ALLOC_MUSA
#include "musa.h"

#define MALLOC_SIZE				16320	/* slightly under 16K */ 
#define BLOCK_ARRAY_INCREMENT	200
#define MUSAINSTANCE_BLOCK_SIZE		MALLOC_SIZE/sizeof(musaInstance)
#define NODE_BLOCK_SIZE			MALLOC_SIZE/sizeof(node)
#define ELEM_BLOCK_SIZE			MALLOC_SIZE/sizeof(MusaElement)
#define CONN_BLOCK_SIZE			MALLOC_SIZE/sizeof(connect)

static int		fresh_musaInstance;		/* next available new musaInstance in block */
static int		fresh_musaInstance_block;	/* last alloced musaInstance block */
static int		last_musaInstance_block;	/* end of musaInstance_blocks table */
static musaInstance	**musaInstance_blocks;		/* array of musaInstance blocks */
static musaInstance	*freed_musaInstance;		/* beginning of free list */

static int		fresh_node;			/* next available new node in block */
static int		fresh_node_block;	/* last alloced node block */
static int		last_node_block;	/* end of node_blocks table */
static node		**node_blocks;		/* array of node blocks */
static node		*freed_node;		/* beginning of free list */

static int		fresh_elem;			/* next available new elem in block */
static MusaElement	*fresh_elem_block;	/* last alloced elem block */
static MusaElement	*freed_elem;		/* beginning of free list */

static int		fresh_conn;			/* next available new conn in block */
static connect	*fresh_conn_block;	/* last alloced conn block */
static connect	*(freed_conn[7]);	/* beginning of free list */

static char		*istring_block;		/* block to hold strings for insts */
static int		ifree;				/* first free space */
static char		*estring_block;		/* block to hold strings for insts */
static int		efree;				/* first free space */

/***********************************************************************
 * INITLISTS
 * Initialize lists used by program.
 *
 * Parameters:
 *	None.
 */
void InitLists()
{
    ledList = lsCreate();
    segList = lsCreate();
    graphList = st_init_table(strcmp,st_strhash);
    vectors = st_init_table(strcmp,st_strhash);
    watchlists = st_init_table(strcmp,st_strhash);
    constants = st_init_table(strcmp,st_strhash);
}	

/***********************************************************************
 * INITALLOCS
 * Initialize allocation.
 *
 * Parameters:
 *	None.
 */
void InitAllocs()
{
    int16	i;

    freed_musaInstance = NIL(musaInstance);
    fresh_musaInstance = MUSAINSTANCE_BLOCK_SIZE;
    musaInstance_blocks = ALLOC(musaInstanceptr, BLOCK_ARRAY_INCREMENT);
    last_musaInstance_block = BLOCK_ARRAY_INCREMENT - 1;
    fresh_musaInstance_block = -1;
    
    freed_node = NIL(node);
    fresh_node = NODE_BLOCK_SIZE;
    node_blocks = ALLOC(nodeptr, BLOCK_ARRAY_INCREMENT);
    last_node_block = BLOCK_ARRAY_INCREMENT - 1;
    fresh_node_block = -1;
    number_of_nodes = 0;
    
    freed_elem = NIL(MusaElement);
    fresh_elem = ELEM_BLOCK_SIZE;
    fresh_elem_block = NIL(MusaElement);

    /* only use free lists 2 thru 6 */
    for (i=2; i<=6; i++) {
	freed_conn[i] = NIL(connect);
    }				/* for... */
    fresh_conn = CONN_BLOCK_SIZE;
    fresh_conn_block = NIL(connect);

    estring_block = ALLOC(char, MALLOC_SIZE);
    istring_block = ALLOC(char, MALLOC_SIZE);
    efree = ifree = 0;
}				/* InitAllocs... */

/*************************************************************************
 * MUSAINSTANCEALLOC
 * Routine to allocate 'musaInstance' structure.
 *
 * Parameters:
 *	None.
 *
 * Return Value:
 *	Pointer to the newly allocated 'musaInstance' structure. (musaInstance *)
 */
musaInstance *MusaInstanceAlloc()
{
    musaInstance	*temp;

    if (freed_musaInstance == NIL(musaInstance)) {
	/*** Use a fresh musaInstance ***/
	if (fresh_musaInstance == MUSAINSTANCE_BLOCK_SIZE) {
	    /*** Alloc a new block ***/
	    if (fresh_musaInstance_block == last_musaInstance_block) {
		/* expand the block table */
		last_musaInstance_block += BLOCK_ARRAY_INCREMENT;
		musaInstance_blocks = REALLOC(musaInstanceptr, musaInstance_blocks,last_musaInstance_block);
	    }
	    fresh_musaInstance_block++;
	    musaInstance_blocks[fresh_musaInstance_block] = ALLOC(musaInstance, MUSAINSTANCE_BLOCK_SIZE);
	    fresh_musaInstance = 0;
	}	
	return(&(musaInstance_blocks[fresh_musaInstance_block][fresh_musaInstance++]));
    } else {
	/* use a musaInstance from the free list */
	temp = freed_musaInstance;
	freed_musaInstance = (musaInstance *) freed_musaInstance->name;
	return(temp);
    }				/* if... */
}				/* MusaInstanceAlloc... */

/***********************************************************************
 * MUSAINSTANCEFREE
 * Free 'musaInstance' structure.
 *
 * Parameters:
 *	inst (musaInstance *) - the 'musaInstance' to free. (Input/Output)
 */
void MusaInstanceFree(inst)
	musaInstance	*inst;
{
    inst->type = DEVTYPE_ILLEGAL;
    inst->name = (char *) freed_musaInstance;
    freed_musaInstance = inst;
}				/* MusaInstanceFree... */

/*************************************************************************
 * FOREACHMUSAINSTANCE
 * Calls "routine" on all non-module instances.
 *
 * Parameters:
 *	routine (int (*)()) - the routine to call. (Input)
 */
void ForEachMusaInstance(routine)
	int			(*routine)();
{
    int			i,j;
    deviceType	type;		/* type of inst */

    if (fresh_musaInstance_block == -1) return;	/* no insts */
    for (i=0; i<fresh_musaInstance_block; i++) {
	for (j=0; j<MUSAINSTANCE_BLOCK_SIZE; j++) {
	    type = musaInstance_blocks[i][j].type;
	    if ((type != DEVTYPE_ILLEGAL) && (type != DEVTYPE_MODULE)) {
		routine(&(musaInstance_blocks[i][j]));
	    }			/* if... */
	}			/* for... */
    }				/* for... */
    for (j=0; j<fresh_musaInstance; j++) {
	type = musaInstance_blocks[fresh_musaInstance_block][j].type;
	if ((type != DEVTYPE_ILLEGAL) && (type != DEVTYPE_MODULE)) {
	    type = musaInstance_blocks[i][j].type;
	    routine(&(musaInstance_blocks[fresh_musaInstance_block][j]));
	}			/* if... */
    }				/* for... */
}				/* ForEachMusaInstance... */

/************************************************************************
 * NODEALLOC
 * Allocate 'node' structure.
 *
 * free node has numberOfHighDrivers == -1,  conns is free list pointer ]
 */
node *NodeAlloc()
{
    node *temp;

    number_of_nodes++;
    if (freed_node == NIL(node)) {
	/* use a fresh node */
	if (fresh_node == NODE_BLOCK_SIZE) {
	    /* alloc a new block */
	    if (fresh_node_block == last_node_block) {
		/* expand the block table */
		last_node_block += BLOCK_ARRAY_INCREMENT;
		node_blocks = REALLOC(nodeptr, node_blocks,last_node_block);
	    }			/* if... */
	    fresh_node_block++;
	    node_blocks[fresh_node_block] = ALLOC(node, NODE_BLOCK_SIZE);
	    fresh_node = 0;
	}			/* if... */
	return &(node_blocks[fresh_node_block][fresh_node++]);
    } else {
	/* use a node from the free list */
	temp = freed_node;
	freed_node = (node *) freed_node->conns;
	return temp;
    }				/* if... */
}				/* NodeAlloc... */

/********************************************************************
 * NODEFREE
 * Free the node only if it is not already freed 
 */
void NodeFree(theNode)
    node *theNode;		
{
    if (theNode->is_supply || theNode->is_ground) {
	DeletePowerNode(theNode);
    } 
    if (theNode->numberOfHighDrivers != -1) {
	number_of_nodes--;
	theNode->numberOfHighDrivers = -1;
	theNode->conns = (connect *) freed_node;
	freed_node = theNode;
    } 
} 

/********************************************************************
 * FOREACHNODE
 * calls "routine" on all transistor network node
 */
void ForEachNode(routine)
    void  (*routine)();
{
    int i,j;

    if (fresh_node_block == -1) {
	return;			/* no nodes */
    }				/* if... */
    for (i=0; i<fresh_node_block; i++) {
	for (j=0; j<NODE_BLOCK_SIZE; j++) {
	    if (node_blocks[i][j].numberOfHighDrivers != -1) {
		routine(&(node_blocks[i][j]));
	    }			/* if... */
	}			/* for... */
    }				/* for... */
    for (j=0; j<fresh_node; j++) {
	if (node_blocks[fresh_node_block][j].numberOfHighDrivers != -1) {
	    routine(&(node_blocks[fresh_node_block][j]));
	}			/* if... */
    }				/* for... */
}				/* ForEachNode... */

/**********************************************************************
 * ELEMALLOC
 * routines to ALLOC elems
 *
 * free elem has name == NIL(char),  u.parentInstance is free list pointer 
 */
MusaElement *ElemAlloc()
{
    MusaElement *temp;

    if (freed_elem == NIL(MusaElement)) {
	/* use a fresh elem */
	if (fresh_elem == ELEM_BLOCK_SIZE) {
	    fresh_elem_block = ALLOC(MusaElement, ELEM_BLOCK_SIZE);
	    fresh_elem = 0;
	}			/* if... */
	return &(fresh_elem_block[fresh_elem++]);
    } else {
	/* use a elem from the free list */
	temp = freed_elem;
	freed_elem = freed_elem->u.sibling;
	return temp;
    }				/* if... */
}				/* ElemAlloc... */

/**********************************************************************
 * ELEMFREE
 */
void ElemFree(elem)
	MusaElement *elem;
{
    elem->name = NIL(char);
    elem->u.sibling = freed_elem;
    freed_elem = elem;
}				/* ElemFree... */

/***********************************************************************
 * ISTRINGALLOC
 * routines to alloc strings.  MusaElement strings and instance strings
 * are stored seperately to limit thrashing
 */
char *IStringAlloc(length)
	int		length;
{
	char	*temp;

	if (length > MALLOC_SIZE) {
		FatalError("Instance name is too long");
	} /* if... */
	if (ifree + length > MALLOC_SIZE) {
		istring_block = ALLOC(char, MALLOC_SIZE);
		ifree = 0;
	} /* if... */
	temp = &(istring_block[ifree]);
	ifree += length;
	return(temp);
} /* IStringAlloc... */

/*********************************************************************
 * ESTRINGALLOC
 */
char *EStringAlloc(length)
	int length;
{
	char *temp;

	if (length > MALLOC_SIZE) {
		FatalError("Element name is too long");
	} /* if... */
	if (efree + length > MALLOC_SIZE) {
		estring_block = ALLOC(char, MALLOC_SIZE);
		efree = 0;
	} /* if... */
	temp = &(estring_block[efree]);
	efree += length;
	return(temp);
} /* EStringAlloc... */

/**********************************************************************
 * CONNALLOC
 * routines to alloc connects.  node == NIL(node) implies free
 * ->next is free list ptr.
 * Only keep free bins for 2 3 4 5 and 6 conns
 */
connect *ConnAlloc(number)
	int number;
{
    connect *temp;

    if (number == 1) {
	number = 2;
    }				/* if... */
    if ((number <= 6) && (freed_conn[number] != NIL(connect))) {
	/* use a conn from the free list */
	temp = freed_conn[number];
	freed_conn[number] = freed_conn[number]->next;
	return temp;
    } else {
	/* use a fresh conns */
	if (fresh_conn + number > CONN_BLOCK_SIZE) {
	    fresh_conn_block = ALLOC(connect, CONN_BLOCK_SIZE);
	    fresh_conn = 0;
	}			/* if... */
	temp = &(fresh_conn_block[fresh_conn]);
	fresh_conn += number;
	return temp;
    }				/* if... */
}				/* ConnAlloc... */

/***********************************************************************
 * CONNFREE
 */
void ConnFree(conns, number)
	connect *conns;
	int number;
{
    if (number == 1) {
	number = 2;
    }				/* if... */
    conns->node = NIL(node);
    while (number > 6) {
	conns->next = freed_conn[4];
	freed_conn[4] = conns;
	number -= 4;
	conns = &(conns[4]);
	conns->node = NIL(node);
    }				/* while... */
    conns->next = freed_conn[number];
    freed_conn[number] = conns;
}				/* ConnFree... */
