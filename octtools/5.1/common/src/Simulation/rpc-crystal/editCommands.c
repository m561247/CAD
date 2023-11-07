/*
 * editCommands.c
 *
 * The functions on the edit menu.
 */

#include "rpc_crystal.h"

extern char RCbuf[];

extern char *cmdName();

extern char *cOctGetNetName();
extern unsigned rpcCrystalStatus;
extern char *rpc_crystal_add_term_to_flow();
extern char *rpc_crystal_get_flow();
extern char *rpc_crystal_check_set_flow_args();
extern char *rpc_crystal_add_formal_dir();
extern char *rpc_crystal_add_actual_dir();

/*
 * rpcHelpCreateFlow
 */
int
rpcHelpCreateFlow( syn, desc)
char **syn, **desc;
{
    *syn = "object(s) \"flow name\"";

    *desc = "Create a flow. Objects are expected to be actual terminals\n\
of CELLTYPE \"MOSFET\" instances. The terminals must be named\n\
\"SOURCE\" or \"DRAIN\". If the name is a unique flow name,\n\
a flow containing the terminals is created, otherwise the terminals\n\
are added to the existing flow. This does not set the direction\n\
of the flow. To save new/changed flows, the facet must be\n\
saved explicitly.";
}

/*
 * rpcCrystalCreateFlow
 *
 * usage - object(s)  "unique name" : crystal-create-flow
 *
 * objects must be actual terms of MOSFET instances.
 * name must be new, unique name niether in crystal or already a flow bag.
 *
 * Get or create the CRYSTAL_FLOWS bag, get or create a flow bag with
 * the given name, then attach all the actual terms of the MOSTETs.
 */
/*ARGSUSED*/
int
rpcCrystalCreateFlow( spot, cmdList, uow )
RPCSpot *spot;          /* window, facet, and mouse location            */
lsList cmdList;         /* VEM arguement list                           */
long uow;               /* for future use                               */
{
    struct RPCArg *rObject, *rText;
    octObject oBag, oTerm;
    octStatus sTerm;
    octGenerator gTerm;
    char *errstr;
    int doCrystal;

    if( (lsLength( cmdList) != 2)
	|| ( lsDelBegin( cmdList, (Pointer *)&rObject ) != LS_OK)
	|| ( rObject->argType != VEM_OBJ_ARG)
	|| ( lsDelBegin( cmdList, (Pointer *)&rText ) != LS_OK)
	|| ( rText->argType != VEM_TEXT_ARG)){

	return( usage( rpcCrystalCreateFlow) );
    }

    if( rpcCrystalStatus & RPC_CRYSTAL_BUILD_DONE){
	doCrystal = TRUE;
    } else {
	doCrystal = FALSE;
    }

    oBag.objectId = rObject->argData.objArg.theBag;
    OH_ASSERT( octGetById( &oBag));

    if( cOctNotUniqueFlowName( rText->argData.string)){
	sprintf( RCbuf, "%s: adding terms to existing flow",
		cmdName( rpcCrystalCreateFlow));
	message( RCbuf);
    } else {
	sprintf( RCbuf, "%s: adding terms to new flow",
		cmdName( rpcCrystalCreateFlow));
	message( RCbuf);
    }

    /* generate each terminal contained by the bag			*/
    OH_ASSERT( octInitGenContents( &oBag, OCT_TERM_MASK, &gTerm));
    while(( sTerm = octGenerate( &gTerm, &oTerm)) == OCT_OK){

	/* make sure its an actual terminal				*/
	if (octIdIsNull(oTerm.contents.term.instanceId)) {
	    sprintf( RCbuf, "%s: %s %s.\n",
		cmdName( rpcCrystalCreateFlow),
		ohFormatName( &oTerm),
		"must be an actual terminal");
	    message( RCbuf);
	} else {

	    /* add this term to the flow				*/
	    errstr = rpc_crystal_add_term_to_flow( &oTerm,
			rText->argData.string, doCrystal);
	    if( errstr != NIL(char)){
		sprintf( RCbuf, "%s: %s",
			cmdName( rpcCrystalCreateFlow), errstr);
		message( RCbuf);
	    }
	}
    }
    OH_ASSERT( sTerm);

    return( RPC_OK);
}

/*
 * rpcHelpDeleteFromFlow
 */
int
rpcHelpDeleteFromFlow( syn, desc)
char **syn, **desc;
{
    *syn = "object(s) \"flow name\"";

    *desc = "Delete objects from a flow. `flow name' must\n\
be the name of an existing flow, and all objects\n\
must be contained by this flow. If there are no\n\
more objects contained by the flow after the delete,\n\
the flow will be deleted. This command only alters\n\
the Oct representation of the flow. A build must be\n\
done to reflect the change in Crystal";
}

/*
 * rpcCrystalDeleteFromFlow
 *
 * usage - object(s)  "unique name" : crystal-create-flow
 *
 * objects must be actual terms of MOSFET instances.
 * name must be new, unique name niether in crystal or already a flow bag.
 *
 * Get or create the CRYSTAL_FLOWS bag, get or create a flow bag with
 * the given name, then attach all the actual terms of the MOSTETs.
 */
/*ARGSUSED*/
int
rpcCrystalDeleteFromFlow( spot, cmdList, uow )
RPCSpot *spot;          /* window, facet, and mouse location            */
lsList cmdList;         /* VEM arguement list                           */
long uow;               /* for future use                               */
{
    struct RPCArg *rObject, *rText;
    octObject oBag, oTerm, oFlow;
    octStatus sTerm;
    octGenerator gTerm;
    char *errstr;

    if( (lsLength( cmdList) != 2)
	|| ( lsDelBegin( cmdList, (Pointer *)&rObject ) != LS_OK)
	|| ( rObject->argType != VEM_OBJ_ARG)
	|| ( lsDelBegin( cmdList, (Pointer *)&rText ) != LS_OK)
	|| ( rText->argType != VEM_TEXT_ARG)){

	return( usage( rpcCrystalDeleteFromFlow) );
    }

    oBag.objectId = rObject->argData.objArg.theBag;
    OH_ASSERT( octGetById( &oBag));

    if( !cOctNotUniqueFlowName( rText->argData.string)){
	sprintf( RCbuf, "%s: flow named `%s' does not exist",
		cmdName( rpcCrystalDeleteFromFlow),
		rText->argData.string);
	message( RCbuf);
	return( RPC_OK);
    }

    if(( errstr = rpc_crystal_get_flow( rText->argData.string, &oFlow, FALSE))
		!= NIL( char)){
	sprintf( RCbuf, "%s: %s", cmdName( rpcCrystalDeleteFromFlow), errstr);
	message( RCbuf);
	return( RPC_OK);
    }
    /* generate each terminal contained by the bag			*/
    OH_ASSERT( octInitGenContents( &oBag, OCT_TERM_MASK, &gTerm));
    while(( sTerm = octGenerate( &gTerm, &oTerm)) == OCT_OK){

	/* make sure its an actual terminal				*/
	if( octIsAttached( &oFlow, &oTerm) != OCT_OK){
	    sprintf( RCbuf, "%s: %s %s.",
		cmdName( rpcCrystalDeleteFromFlow),
		ohFormatName( &oTerm),
		"isn't attached to the flow bag");
	    message( RCbuf);
	} else {

	    /* delete this term from the flow				*/
	    if( octDetach( &oFlow, &oTerm) != OCT_OK){
		sprintf( RCbuf, "%s: %s",
			cmdName( rpcCrystalDeleteFromFlow),
			octErrorString());
		message( RCbuf);
	    }
	}
    }
    OH_ASSERT( sTerm);

    if( octGenFirstContainer( &oFlow, OCT_TERM_MASK, &oTerm) != OCT_OK){
	if( octDelete( &oFlow) != OCT_OK){
	    sprintf( RCbuf, "%s: flow bag empty, attemped to delete it: %s",
		    cmdName( rpcCrystalDeleteFromFlow),
		    octErrorString());
	    message( RCbuf);
	}
    }

    return( RPC_OK);
}

/*
 * rpcHelpSetFlowByName
 */
rpcHelpSetFlowByName( syn, desc)
char **syn, **desc;
{
    *syn = "\"flow name\" \"in|out|off|normal|ignore\"";

    *desc = "Set the direction of a flow by the flow name\n\
(equivalent to the \"flow\" command in crystal).\n\
If a delay has been run, crystal's \"clear\" command\n\
should be run before doing this operation.";
}

/*
 * rpcCrystalSetFlowByName
 *
 * usage - "name" "keyword" : crystal-set-flow-by-name
 *
 * name must be in crystal flow table or an existing named flow bag.
 * keyword must be one of [ in | out | ignore | normal | off ]
 *
 * If the name is in crystal's FlowTable, execute the equivelent "flow" command.
 * Else, if its a flow bag, set the direction of each terminal.
 */
/*ARGSUSED*/
int
rpcCrystalSetFlowByName( spot, cmdList, uow )
RPCSpot *spot;          /* window, facet, and mouse location            */
lsList cmdList;         /* VEM arguement list                           */
long uow;               /* for future use                               */
{
    struct RPCArg *rKeyText, *rFlowText;
    char *keyword, *flowname, *errstr;

    if( !( rpcCrystalStatus & RPC_CRYSTAL_BUILD_DONE)){
	return( message( "you have to run the build command first"));
    }
    if( (lsLength( cmdList) != 2)
	|| ( lsDelBegin( cmdList, (Pointer *)&rFlowText ) != LS_OK)
	|| ( rFlowText->argType != VEM_TEXT_ARG)
	|| ( lsDelBegin( cmdList, (Pointer *)&rKeyText ) != LS_OK)
	|| ( rKeyText->argType != VEM_TEXT_ARG)){

	return( usage( rpcCrystalSetFlowByName) );
    }
    flowname = rFlowText->argData.string;
    keyword = rKeyText->argData.string;

    if(( errstr = rpc_crystal_check_set_flow_args( flowname, keyword))
	!= NIL(char)){
	sprintf( RCbuf, "%s: %s", cmdName( rpcCrystalSetFlowByName), errstr);
	return( message( RCbuf));
    } else {
	/* issue the command to crystal					*/
	sprintf( RCbuf, "flow %s %s", keyword, flowname);
	echoCmd( RCbuf);
	CmdDo( RCbuf);
	echoCmdEnd();
    }
    return( RPC_OK);
}

/*
 * rpcHelpSetFlowByBag
 */
rpcHelpSetFlowByBag( syn, desc)
char **syn, **desc;
{
    *syn = "object(s) \"in|out|off|normal|ignore\"";

    *desc = "Set the direction of a flow by the flow bag.\n\
The objects must all be flow bags.\n\
If a delay has been run, crystal's \"clear\" command\n\
should be run before doing this operation.";
}

/*
 * rpcCrystalSetFlowByBag
 *
 * usage - object(s) "keyword" : crystal-set-flow-by-bag
 *
 * objects must be bags contained by CRYSTAL_FLOWS.
 * keyword must be one of [ in | out | ignore | normal | off ]
 *
 * For each bag, set the direction of the associated FETs.
 */
/*ARGSUSED*/
int
rpcCrystalSetFlowByBag( spot, cmdList, uow )
RPCSpot *spot;          /* window, facet, and mouse location            */
lsList cmdList;         /* VEM arguement list                           */
long uow;               /* for future use                               */
{
    struct RPCArg *rKeyText, *rObject;
    char *keyword, *flowname, *errstr;
    octObject oBag, oCmdBag;
    octGenerator gBag;
    octStatus sBag;
    int cmdCount = 0;

    if( !( rpcCrystalStatus & RPC_CRYSTAL_BUILD_DONE)){
	return( message( "you have to run the build command first"));
    }
    if( (lsLength( cmdList) != 2)
	|| ( lsDelBegin( cmdList, (Pointer *)&rObject ) != LS_OK)
	|| ( rObject->argType != VEM_OBJ_ARG)
	|| ( lsDelBegin( cmdList, (Pointer *)&rKeyText ) != LS_OK)
	|| ( rKeyText->argType != VEM_TEXT_ARG)){

	return( usage( rpcCrystalSetFlowByBag) );
    }

    keyword = rKeyText->argData.string;
    oCmdBag.objectId = rObject->argData.objArg.theBag;
    if( octGetById( &oCmdBag) < OCT_OK){
	sprintf( RCbuf, "%s: %s: %s",
		cmdName( rpcCrystalSetFlowByBag),
		"couldn't get RPC object bag",
		octErrorString());
	return( message( RCbuf));
    }

    if( octInitGenContents( &oCmdBag, OCT_BAG_MASK, &gBag) < OCT_OK){
	sprintf( RCbuf, "%s: %s: %s",
		cmdName( rpcCrystalSetFlowByBag),
		"couldn't init generator",
		octErrorString());
	return( message( RCbuf));
    }
    while(( sBag = octGenerate( &gBag, &oBag)) == OCT_OK){
	flowname = oBag.contents.bag.name;
	if(( errstr = rpc_crystal_check_set_flow_args( flowname, keyword))
	    != NIL(char)){
	    sprintf( RCbuf, "%s: %s", cmdName( rpcCrystalSetFlowByBag), errstr);
	    message( RCbuf);
	} else {
	    /* issue the command to crystal				*/
	    sprintf( RCbuf, "flow %s %s", keyword, flowname);
	    echoCmd( RCbuf);
	    CmdDo( RCbuf);
	    ++cmdCount;
	}
    }
    OH_ASSERT( sBag);

    if( cmdCount == 0){
	sprintf( RCbuf, "%s: no flow bags found in object arg",
		cmdName( rpcCrystalSetFlowByBag));
	message( RCbuf);
    } else {
	echoCmdEnd();
    }
    return( RPC_OK);
}

/*
 * rpcHelpSetTermDir
 */
rpcHelpSetTermDir( syn, desc)
char **syn, **desc;
{
    *syn = "object(s) \"in|out\"";

    *desc = "Set the direction of a MOSFET terminal.\n\
Each object should be either a MOSFET instance, a\n\
\"SOURCE\" or \"DRAIN\" terminal of a MOSFET instance,\n\
or a formal terminal of the top-level facet. For MOSFET\n\
instance objects, the direction of the \"SOURCE\" terminal is\n\
is set. Note that this routine only alters the OCT facet.\n\
To relfect the change in crystal, a build command must be run.";
}

/*
 * rpcCrystalSetTermDir
 *
 * usage: object(s) "keyword" : crystal-set-term-dir
 *
 * Put a property on the terminal specifying the direction of data flow
 * through the terminal, and make the call to the crystal routine
 * to update the simualation data structure.
 */
/*ARGSUSED*/
int 
rpcCrystalSetTermDir( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    struct RPCArg *rObject, *rText;
    octObject oBag, oObj, oProp, oTerm, oInst;
    octStatus sObj;
    octGenerator gObj;
    char *keyword, *errstr;

    if( (lsLength( cmdList) != 2)
	|| ( lsDelBegin( cmdList, (Pointer *)&rObject ) != LS_OK)
	|| ( rObject->argType != VEM_OBJ_ARG)
	|| ( lsDelBegin( cmdList, (Pointer *)&rText ) != LS_OK)
	|| ( rText->argType != VEM_TEXT_ARG)){

	return( usage( rpcCrystalSetTermDir) );
    }

    /* create the property to attach to each terminal			*/
    oProp.type = OCT_PROP;
    oProp.contents.prop.type = OCT_STRING;
    oProp.contents.prop.name = "DIRECTION";
    keyword = rText->argData.string;
    if( !strcmp( keyword, "in")){
	oProp.contents.prop.value.string = "INPUT";
    } else if( !strcmp( keyword, "out")){
	oProp.contents.prop.value.string = "OUTPUT";
    } else {
	sprintf( RCbuf, "%s: unknown keyword \"%s\": %s.\n",
		cmdName( rpcCrystalSetTermDir),
		keyword, "Try one of [in, out]");
	return( message( RCbuf));
    }

    oBag.objectId = rObject->argData.objArg.theBag;
    OH_ASSERT( octGetById( &oBag));

    /* generate each terminal contained by the bag			*/
    OH_ASSERT( octInitGenContents( &oBag,
		OCT_TERM_MASK | OCT_INSTANCE_MASK, &gObj));
    while(( sObj = octGenerate( &gObj, &oObj)) == OCT_OK){

	if( oObj.type == OCT_TERM){
	    /* find the instance that contains this terminal		*/
	    oTerm = oObj;
	    if (!octIdIsNull(oInst.objectId = oTerm.contents.term.instanceId)) {
		if( strcmp( oTerm.contents.term.name, "SOURCE")
			&& strcmp( oTerm.contents.term.name, "DRAIN") ){
		    sprintf( RCbuf, "%s: Bad term name \"%s\": %s",
			cmdName(rpcCrystalSetTermDir),
			oTerm.contents.term.name,
			"Must be one of [SOURCE, DRAIN]" );
		    message( RCbuf);
		    continue;
		}
		if( octGetById( &oInst) < OCT_OK){
		    sprintf( RCbuf, "%s: %s %s failed: %s\n",
			cmdName( rpcCrystalSetTermDir),
			"get by id of instance containing aTerm",
			oTerm.contents.term.name,
			octErrorString() );
		    message( RCbuf);
		    continue;
		}
	    } else {
		if(( errstr = rpc_crystal_add_formal_dir( &oTerm,
				spot->facet, &oProp))
			!= NIL(char)){
		    sprintf( RCbuf, "%s: %s",
			cmdName( rpcCrystalSetTermDir), errstr);
		    message( RCbuf);
		}
		continue;
	    }
	} else {
	    /* find the "SOURCE" term of the instance			*/
	    oInst = oObj;
	    if( ohGetByTermName( &oInst, &oTerm, "SOURCE") != OCT_OK){
		sprintf( RCbuf, "%s: %s \"%s\" from %s",
			cmdName(rpcCrystalSetTermDir),
			"Couldn't get term named", "SOURCE",
			ohFormatName( &oInst) );
		message( RCbuf);
		continue;
	    }
	}

	/* find the crystal transistor					*/
	if(( errstr = rpc_crystal_add_actual_dir( &oTerm, &oInst, &oProp))
		!= NIL(char)){
	    sprintf( RCbuf, "%s: %s", cmdName( rpcCrystalSetTermDir), errstr);
	    message( RCbuf);
	    continue;
	}

	/* set the appropriate flags					*/
	/*** This is not implemented now because this kind of direction 
	     property access is not available in tty based crystal.  It is
	     not known how changing these flags mid-session will effect
	     crystal.  For now, the direction is stored in OCT, and the user
	     must rebuild to have this change reflected in crystal.	*/

    }
    OH_ASSERT( sObj);

    return( RPC_OK);
}

/*
 * rpcHelpShowTermDir
 */
rpcHelpShowTermDir( syn, desc)
char **syn, **desc;
{
    *syn = "[object(s)]";

    *desc = "Show how crystal will interpret the direction properties\n\
on the objects. Objects should be terms or instances. If no\n\
objects are given, the directions on the formal terminals\n\
of the facet are shown";
}

/*
 * rpcCrystalShowTermDir
 *
 * usage: object(s) : crystal-show-term-dir
 *
 * Give the user a text-command shell.
 */
/*ARGSUSED*/
int 
rpcCrystalShowTermDir( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    struct RPCArg *rObject;
    octObject oBag, oObj;
    octGenerator gObj;
    int len;

    len = lsLength( cmdList);
    if(( len > 1)
	|| (( len == 1) &&
	    (( lsDelBegin( cmdList, (Pointer *)&rObject ) != LS_OK)
	    || ( rObject->argType != VEM_OBJ_ARG)))){

	return( usage( rpcCrystalShowTermDir) );
    }

    if( len == 0){
	rpc_crystal_show_formal_dir( spot->facet);
	return( RPC_OK);
    }

    oBag.objectId = rObject->argData.objArg.theBag;
    OH_ASSERT( octGetById( &oBag));

    /* generate each terminal contained by the bag			*/
    OH_ASSERT( octInitGenContents( &oBag, OCT_TERM_MASK | OCT_INSTANCE_MASK,
	&gObj));
    while( octGenerate( &gObj, &oObj) == OCT_OK){
	if( oObj.type == OCT_TERM){
	    rpc_crystal_show_terminal_dir( &oObj);
	} else {
	    rpc_crystal_show_instance_dir( &oObj);
	}
    }
    echoCmdEnd();

    return( RPC_OK);
}

/*
 * rpcHelpIdent
 */
rpcHelpIdent( syn, desc)
char **syn, **desc;
{
    *syn = "object(s)";

    *desc = "Give crystal's internal node name for a net.\n\
Objects should be nets or terms.";
}

/*
 * rpcCrystalIdent
 *
 * usage: objects : crystal-ident
 *
 * Tell the user crystal's name for the selected net or the net
 * containing the selected term.
 */
/*ARGSUSED*/
int 
rpcCrystalIdent( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    struct RPCArg *rObject;
    octObject oBag, oObj, oNet;
    char *crName;

    if( !( rpcCrystalStatus & RPC_CRYSTAL_BUILD_DONE)){
	return( message( "you have to run the build command first"));
    }
    if( (lsLength( cmdList) != 1)
	|| ( lsDelBegin( cmdList, (Pointer *)&rObject ) != LS_OK)
	|| ( rObject->argType != VEM_OBJ_ARG)){

	return( usage( rpcCrystalIdent));
    }

    /* find the net assocated with each object				*/
    oBag.objectId = rObject->argData.objArg.theBag;
    OH_ASSERT( octGetById( &oBag));
    if( octGenFirstContent( &oBag, OCT_TERM_MASK|OCT_NET_MASK, &oObj)
	!= OCT_OK){
	message("crystal-ident: object isn't net or term");
    } else {
	if( oObj.type == OCT_TERM ){
	    if( octGenFirstContainer( &oObj, OCT_NET_MASK, &oNet) != OCT_OK){
		/*** error */
		sprintf( RCbuf, "%s: %s",
			cmdName( rpcCrystalIdent),
			"term isn't contained by a net");
		message( RCbuf);
		return( RPC_OK);
	    }
	} else {
	    oNet = oObj;
	}

	/* construct the text with which to interface to crystal	*/
	crName = cOctGetNetName( &oNet );
	sprintf( RCbuf, "User net \"%s\" is called \"%s\" in crystal",
		oNet.contents.net.name, crName);

	/* give the user crystal's name for the net			*/
	message( RCbuf );
    }

    return( RPC_OK);
}
