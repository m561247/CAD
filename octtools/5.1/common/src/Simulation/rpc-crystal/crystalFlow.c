/*
 * crystalFlow.c
 *
 * Interface to the flow facility of crystal.
 *
 * NOTE: rpc.h cannot be included in this file because crystal.h and rpc.h
 *       both define a type called "Pointer".
 */

#include "crystal.h"
#include "utility.h"
#include "st.h"
#include "cOct.h"

extern octObject oCrystalFlowBag, oFacet;
extern st_table *OctIdTable; /* maps FET addresses to octIds		*/
extern char *sthelp_findkey();

static char CFbuf[ 512 ];

char *rpc_crystal_get_flow();
char *rpc_crystal_check_flow_term();
char *rpc_crystal_inst_to_fet();

/*
 * rpc_crystal_add_term_to_flow
 *
 * Add the given actual term to the named flow.
 * Make sure the term is a MOSFET term with a known name, then link
 * the term into the CRYSTAL_FLOWS structure appropriatly,
 * then locate the crystal FET struct and do the appropriate crystal
 * data structure stuff.
 */
char *
rpc_crystal_add_term_to_flow( oTerm, flowName, doCrystal)
octObject *oTerm;
char *flowName;
int doCrystal;
{
    octObject oInst, oFlowBag;
    FET *fet;
    char *errstr;

    /* check instance type, term name, bag connectivity, etc		*/
    if(( errstr = rpc_crystal_check_flow_term( oTerm, &oInst)) != NIL(char)){
	return( errstr);
    }

    /* if crystal data struct built, get the fet			*/
    if( doCrystal == TRUE){
	if(( errstr = rpc_crystal_inst_to_fet( &oInst, &fet)) != NIL(char)){
	    return( errstr);
	}
    }

    /* get or create the flow bag					*/
    if(( errstr = rpc_crystal_get_flow( flowName, &oFlowBag, TRUE))
	!= NIL(char)){
	return( errstr);
    }

    if( octAttachOnce( &oFlowBag, oTerm) < OCT_OK){
	sprintf(CFbuf,
	    "Couldn't attach terminal \"%s\" to the \"%s\" flow bag: %s",
	    oTerm->contents.term.name, flowName, octErrorString());
	return( CFbuf);
    }

    /* handle the crystal data structure stuff				*/
    if( doCrystal){
	cOctSetTermFlow( fet, oTerm, flowName);
    }

    return( NIL(char));
}

/*
 * rpc_crystal_check_flow_term
 *
 * check the term and instance to make sure its sensible to
 * attache it to a flow.
 */
char *
rpc_crystal_check_flow_term( oTerm, oInst)
octObject *oTerm, *oInst;
{
    octObject oProp;

    /* check instance type and term name for corectness			*/
    oInst->objectId = oTerm->contents.term.instanceId;
    if( octGetById( oInst) < OCT_OK){
	sprintf( CFbuf, "Couldn't get instance for term \"%s\".",
		oTerm->contents.term.name);
	return( CFbuf);
    }
    oProp.type = OCT_PROP;
    oProp.contents.prop.name = "CELLTYPE";
    if( !cOctGetInheritedProp( oInst, &oProp, OCT_STRING)
	|| (strcmp( oProp.contents.prop.value.string, "MOSFET"))){
	sprintf( CFbuf,
		"term must be part of an instance of type MOSFET.");
	return( CFbuf);
    }
    if( strcmp( oTerm->contents.term.name, "SOURCE")
	&& strcmp( oTerm->contents.term.name, "DRAIN")){
	sprintf( CFbuf, "term must be named SOURCE or DRAIN.");
	return( CFbuf);
    }

    return( NIL(char));
}

/*
 * rpc_crystal_get_flow
 *
 * Use or create the CRYSTAL_FLOWS bag, and then get or create
 * the flow bag with the specified name.
 */
char *
rpc_crystal_get_flow( flowName, oFlowBag, doCreate)
char *flowName;
octObject *oFlowBag;
int doCreate; /* boolean to decide if bag should be created		*/
{

    /* get flow the bag							*/
    if (octIdIsNull(oCrystalFlowBag.objectId)) {
	if( doCreate){
	    if( octCreate( &oFacet, &oCrystalFlowBag) < OCT_OK){
		sprintf( CFbuf,"Couldn't create CRYSTAL_FLOWS bag in facet: %s",
		    octErrorString());
		return( CFbuf);
	    }
	} else {
	    sprintf( CFbuf, "Couldn't find bag named \"%s\".",
		    oCrystalFlowBag.contents.bag.name);
	    return( CFbuf);
	}
    }
    oFlowBag->type = OCT_BAG;
    oFlowBag->contents.bag.name = flowName;
    if( octGetByName( &oCrystalFlowBag, oFlowBag) == OCT_NOT_FOUND){
	if( doCreate){
	    if( octCreate( &oCrystalFlowBag, oFlowBag) != OCT_OK){
		sprintf( CFbuf, "Couldn't create flow bag \"%s\": %s.",
		    flowName, octErrorString());
		return( CFbuf);
	    }
	} else {
	    sprintf( CFbuf, "Couldn't find flow bag named \"%s\".", flowName);
	    return( CFbuf);
	}
    }
    return( NIL(char));
}

/*
 * rpc_crystal_inst_to_fet
 *
 * Map an instance into a crystal FET.  This only works for MOSFET instances
 * at the top level.
 */
char *
rpc_crystal_inst_to_fet( oInst, fet)
octObject *oInst;
FET **fet;
{
    /* find the FET struct coresponding to the instance			*/
    *fet = (FET *) sthelp_findkey( OctIdTable, (char *)oInst->objectId);
    if( *fet == NIL(FET)){
	sprintf(CFbuf, "%s.",
	    "Couldn't find crystal's representation of this MOSFET instance");
	return( CFbuf);
    }

    return( NIL(char));
}

/*
 * rpc_crystal_check_set_flow_args
 *
 * Checks flow name and set string for correctness in crystal.
 * Return NIL(char) if OK, error string if error.
 */
char *
rpc_crystal_check_set_flow_args( flow, key)
char *key, *flow;
{

    /* see if flow is in crystal					*/
    if( !cOctNotUniqueFlowName( flow)){
	sprintf( CFbuf, "there is no flow named \"%s\".", flow);
	return( CFbuf);
    }

    /* check the key word						*/
    if( strcmp( key, "in") && strcmp( key, "out")
	&& strcmp( key, "off") && strcmp( key, "ignore")
	&& strcmp( key, "normal")){
	/* keyword isn't one known to crystal				*/
	sprintf(CFbuf, "\"%s\" %s %s",
		key,
		"is not a valid key word.",
		"Try one of [in, out, off, ignore, normal].");
	return( CFbuf);
    }
    return( NIL(char));
}
