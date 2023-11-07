
/***********************************************************************
 * Handle lookup, insert, and unlink of elements and instances in hash table.
 * Filename: hash.c
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
 *	static int StrHash() - String hash function.
 *	void InstInsert() - Insert instance as child of parent instance.
 *	void InstUnlink() - Unlink instance from parent.
 *	int InstLookup() - Search for instance in given parent instance.
 *	void ElemInsert() - Insert element as child of parent instance.
 *	void ElemUnlink() - Unlink element from parent.
 *	int ElemLookup() - Search for element in given parent instance.
 * Modifications:
 *	Rodney Lai				Commented						1989
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define HASH_MUSA
#include "musa.h"

/************************************************************************
 * INSTINSERT
 * Make 'inst' a child of 'parent'
 *
 * Parameters:
 *	parent (musaInstance *) - parent instance. (Input)
 *	inst (musaInstance *) - the instance to attache as child of 'parent'. (Input)
 */
void InstInsert(parent, inst)
    musaInstance		*parent;
    musaInstance		*inst;
{
    if ( parent->hashed_children == 0 ) {
	parent->hashed_children = st_init_table( strcmp, st_strhash );
    }
    st_insert( parent->hashed_children, (char*)inst->name, (char*)inst );
    inst->parent = parent;
}

/*************************************************************************
 * INSTUNLINK
 * Unlink 'inst' from its parent.
 *
 * Parameters:
 *	inst (musaInstance *) - the instance to unlink. (Input)
 */
void InstUnlink(inst)
	musaInstance		*inst;
{
    musaInstance* parent =  inst->parent;
    
    if (  parent->hashed_children ) {
	char* name;
	st_delete( parent->hashed_children, &name, 0 );
    }
} /* InstUnlink... */

/************************************************************************
 * INSTLOOKUP
 * Search the children of 'parent' for the an instance, 'name'.
 *
 * Parameters:
 *	parent (musaInstance *) - the instance to seach in. (Input)
 *	name (char *) - the name of the instance to search. (Input)
 *	inst (musaInstance **) - the instance, if found. (Output)
 *
 * Return Value:
 *	TRUE, if sucessful.
 *	FALSE, if unsucessful.
 */
int InstLookup(parent, name, inst)
    musaInstance		*parent;
    char		*name;
    musaInstance		**inst;
{
    if ( parent->hashed_children == 0 ) return FALSE;
    if ( st_lookup( parent->hashed_children, name, (char**)inst ) == 1 ) {
	return TRUE;
    } else {
	return FALSE;
    }
}				/* InstLookup... */

/************************************************************************
 * ELEMINSERT
 * Make 'elem' a child of 'parent'.
 *
 * Parameters:
 *	parent (musaInstance *) - the parent instance. (Input)
 *	elem (MusaElement *) - the element to insert as child of 'parent'. (Input)
 */
void ElemInsert(parent, elem)
    musaInstance		*parent;
    MusaElement		*elem;
{
    if ( parent->hashed_elements == 0 ) {
	parent->hashed_elements = st_init_table( strcmp, st_strhash );
    }
    if ( st_insert( parent->hashed_elements, elem->name, (char*)elem ) == 1 ) {
	Warning( "Already in table" );
    }
    if ( parent->elems ) {
	parent->elems->data &= ~PARENT_PTR;
	parent->elems->u.sibling = elem;
    }
    parent->elems = elem;

    elem->data |= PARENT_PTR;
    elem->u.parentInstance = parent;
}				/* ElemInsert... */

/************************************************************************
 * ELEMUNLINK
 * Unlink 'elem' from its parent.
 *
 * Parameters:
 *	elem (MusaElement *) - the element to unlink. (Input)
 */
void ElemUnlink(elem)
    MusaElement		*elem;
{
    MusaElement		*p;
    musaInstance		*parent;

    fprintf( stderr, "in elem unlink (should we be here?)" );
    parent = getParentInstance( elem );
    p = getMainSiblingElement( elem );
    if ( parent->hashed_elements ) {
	char* name;
	st_delete( parent->hashed_elements, &name, 0 );
    }
    p->data |= PARENT_PTR;
    p->u.parentInstance = elem->u.parentInstance;
}				/* ElemUnlink... */

/*************************************************************************
 * ELEMLOOKUP
 * Lookup the MusaElement, 'name', in 'parent'
 *
 * Parameters:
 *	parent (musaInstance *) - the parent instance. (Input)
 *	name (char *) - the name of the element to find. (Input)
 *	elem (MusaElement **) - the element, if found. (Output)
 *
 * Return Value:
 *	TRUE, if sucessful.
 *	FALSE, if unsucessful.
 */
int ElemLookup(parent, name, elem)
	musaInstance	*parent;
	char		*name;
	MusaElement	**elem;
{
    if ( parent->hashed_elements == 0 ) return FALSE;
    if ( st_lookup( parent->hashed_elements, name ,(char**)elem ) == 1 ) {
	return TRUE;
    } else {
	return FALSE;
    }
}				/* ElemLookup... */
