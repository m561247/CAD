/*
 * simulateCommands.c
 *
 * Commands on the simulation menu.
 */

#include "rpc_crystal.h"

extern char *cmdName();
extern char RCbuf[];

extern vemSelSet RCHiSet;
extern vemStatus idm_which_one();
extern octObject oFacet;
extern char *cOctInsertBackSlash(), *cOctGetNetName();
extern unsigned cOctStatus;
extern unsigned rpcCrystalStatus;
extern int rpc_crystal_dummy_function();
int rpc_crystal_sel_item();

/*
 * rpcHelpDelay
 */
rpcHelpDelay( syn, desc)
char **syn, **desc;
{
    *syn = "object(s) [\"HiTime LoTime\"]";

    *desc = "Runs the delay command on each net specified.\n\
Objects should be nets or terminals.  If the text argument isn't\n\
given, it defaults to \"0 0\".";
}

/*
 * rpcCrystalDelay
 *
 * usage: object(s) ["HiTime LoTime"] : crystal-delay
 *
 * Set the delay for the for the specified net or term.
 *
 * If the text arg isn't given, both HiTime and LoTime default to 0.
 */
/*ARGSUSED*/
int 
rpcCrystalDelay( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    struct RPCArg *rObject, *rText;
    octObject oBag, oObj, oNet;
    octStatus sObj;
    octGenerator gObj;
    char *crName;
    int cmdCount = 0, len;
    char *delayText;

    if( !( rpcCrystalStatus & RPC_CRYSTAL_BUILD_DONE)){
	return( message( "you have to run the build command first"));
    }
    if((( len = lsLength( cmdList)) < 1)
	|| ( len > 2)
	|| ( lsDelBegin( cmdList, (Pointer *)&rObject ) != LS_OK)
	|| ( rObject->argType != VEM_OBJ_ARG)){

	return( usage( rpcCrystalDelay) );
    }
    if( len == 2){
	if(( lsDelBegin( cmdList, (Pointer *)&rText ) != LS_OK)
	   || ( rText->argType != VEM_TEXT_ARG)){
	    return( usage( rpcCrystalDelay) );
	} else {
	    delayText = rText->argData.string;
	}
    } else {
	/* len must equal 1						*/
	delayText = "0 0";
    }

    /* find the net assocated with each object				*/
    oBag.objectId = rObject->argData.objArg.theBag;
    OH_ASSERT( octGetById( &oBag));
    OH_ASSERT( octInitGenContents( &oBag, OCT_TERM_MASK|OCT_NET_MASK, &gObj));
    while(( sObj = octGenerate( &gObj, &oObj)) == OCT_OK){
	if( oObj.type == OCT_TERM ){
	    if( octGenFirstContainer( &oObj, OCT_NET_MASK, &oNet) != OCT_OK){
		/*** error */
		(void) sprintf( RCbuf, "%s: term \"%s\" isn't contained by a net",
			cmdName( rpcCrystalDelay),
			oObj.contents.term.name);
		message( RCbuf);
		continue;
	    }
	} else {
	    oNet = oObj;
	}

	/* construct the text with which to interface to crystal	*/
	crName = cOctInsertBackSlash( cOctGetNetName( &oNet ));
	(void) sprintf( RCbuf, "delay %s %s", crName, delayText);

	/* call crystal routine						*/
	CmdDo( RCbuf );
	cOctStatus |= COCT_PENDING_CRIT_UPDATE;
	++cmdCount;

	/* echo to user so (s)he knows what's going on			*/
	echoCmd( RCbuf);
    }
    OH_ASSERT( sObj);

    if( cmdCount){
	echoCmdEnd();
    } else {
	(void) sprintf( RCbuf, "%s: no term or net object args found",
		cmdName( rpcCrystalDelay));
	message( RCbuf);
    }

    return( RPC_OK);
}

/*
 * rpcHelpCrit
 */
rpcHelpCrit( syn, desc)
char **syn, **desc;
{
    *syn = "[\"path number\"]";

    *desc = "Store and view delay results. First, an OCT representation\n\
of the most recent delay results is stored in the facet,\n\
then the paths can be examined. If \"path number\" is given,\n\
this path is high lighted. If no \"path number\" is given, a\n\
dialog is entered to select a critical path.";
}

static dmWhichItem WhichPath[ 5 ];

/*
 * rpcCrystalCrit
 *
 * usage: "path number" : crit
 *
 * Initially, flush the Calculate the critical path bag.
 * Eventually, do some fancy displaying of critical paths.
 */
/*ARGSUSED*/
int 
rpcCrystalCrit( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    RPCArg *rText;
    octObject oPathBag, oStageBag, oCritPathBag;
    octGenerator gBag;
    octStatus sBag;
    int pathnum, len, selpath;
    vemStatus ret;

    len = lsLength( cmdList);
    if( len > 1){
	return( usage( rpcCrystalCrit) );
    } else if( len == 1){
	if(( lsDelBegin( cmdList, (Pointer *)&rText ) != LS_OK)
	    || ( rText->argType != VEM_TEXT_ARG)){
	    return( usage( rpcCrystalCrit) );
	}
	selpath = atoi(rText->argData.string);
    } else {
	selpath = 0;
    }

    /* store most recent results in OCT					*/
    if(( rpcCrystalStatus & RPC_CRYSTAL_BUILD_DONE) 
	    && ( cOctStatus & COCT_PENDING_CRIT_UPDATE)){
	cOctStoreCrit( FALSE);
	cOctStatus &= ~COCT_PENDING_CRIT_UPDATE;
    }

    /* Show critical path stuff						*/
    oCritPathBag.type = OCT_BAG;
    oCritPathBag.contents.bag.name = "CRYSTAL_CRITICAL_PATHS";
    if( octGetByName( &oFacet, &oCritPathBag) != OCT_OK){
	/* some kind of error ( cOctStoreCrit() failed)			*/
	(void) sprintf( RCbuf, "%s: %s",
		    cmdName( rpcCrystalCrit),
		    "No critical paths bag");
	return( message( RCbuf));
    }

    if( selpath == 0){
	/* count path bags						*/
	pathnum = 0;
	if( octInitGenContents( &oCritPathBag, OCT_BAG_MASK, &gBag) != OCT_OK){
	    /* error */
	    (void) sprintf( RCbuf, "%s: %s: %s",
			cmdName( rpcCrystalCrit),
			"Couldn't init generator",
			octErrorString() );
	    return( message( RCbuf));
	}
	while(( sBag = octGenerate( &gBag, &oPathBag)) == OCT_OK){
	    if( octGenFirstContent( &oPathBag, OCT_BAG_MASK, &oStageBag)
			== OCT_OK){
		(void) sprintf( RCbuf, "Path %d (%s)",
				pathnum + 1, oStageBag.contents.bag.name);
		WhichPath[pathnum].itemName = util_strsav( RCbuf);
		WhichPath[pathnum].flag = 0;
		++pathnum;
	    }
	}

	if( pathnum == 0){
	    (void) sprintf( RCbuf, "%s: %s",
			cmdName( rpcCrystalCrit),
			"There are no critical paths");
	    return( message( RCbuf));
	}
	ret = dmWhichOne( "Critical Paths",
		    pathnum,
		    WhichPath,
		    &selpath,
		    rpc_crystal_dummy_function,
		    "Select a critical path\nto be highlighted");

	for( len = 0; len < pathnum; ++len){
	    FREE( WhichPath[ len ].itemName);
	}
	if(( ret == VEM_NOSELECT) || (selpath == -1)){
	    (void) sprintf( RCbuf, "%s: No critical path selected",
		    cmdName( rpcCrystalCrit));
	    return( message( RCbuf));
	}
    }

    /* now show the user the selpath 					*/
    pathnum = 0;
    if( octInitGenContents( &oCritPathBag, OCT_BAG_MASK, &gBag) != OCT_OK){
	/* error */
	(void) sprintf( RCbuf, "%s: %s: %s",
		    cmdName( rpcCrystalCrit),
		    "Couldn't init generator",
		    octErrorString() );
	return( message( RCbuf));
    }
    while(( sBag = octGenerate( &gBag, &oPathBag)) == OCT_OK){
	if( octGenFirstContent( &oPathBag, OCT_BAG_MASK, &oStageBag)
		    == OCT_OK){
	    if( pathnum == selpath) break;
	    ++pathnum;
	}
    }
    if( sBag == OCT_OK){
	if( octFreeGenerator( &gBag) != OCT_OK){
	    (void) sprintf( RCbuf, "%s: %s: %s",
		cmdName( rpcCrystalCrit),
		"Warning: Couldn't free generator",
		octErrorString() );
	    message( RCbuf);
	}
    } else {
	/* didn't find path number selpath				*/
	(void) sprintf( RCbuf, "%s: %s %d",
		    cmdName( rpcCrystalCrit),
		    "No path",
		    selpath);
	return( message( RCbuf));
    }

    if( RCHiSet == (vemSelSet) 0){
	(void) sprintf( RCbuf, "%s: %s",
		    cmdName( rpcCrystalCrit),
		    "Couln't get a select set from vem");
	return( message( RCbuf));
    } else {
	if(( vemClearSelSet( RCHiSet) != VEM_OK) ||
		(vemAddSelSet( RCHiSet, oPathBag.objectId) != VEM_OK)){
	    (void) sprintf( RCbuf, "%s: %s",
			cmdName( rpcCrystalCrit),
			"Couln't add the path bag to the select set");
	    return( message( RCbuf));
	}
    }

    ret = dmConfirm( "See Stages?",
		"Would you like to see the individual stages?",
		"Yes", "No" );
    if( ret == VEM_FALSE){
	/* give some printed data to go along with hilighted path	*/
	if( octGenFirstContent( &oPathBag, OCT_BAG_MASK, &oStageBag) == OCT_OK){
	    (void) sprintf( RCbuf,"%s: %s %d (%s) with stage times:",
		    cmdName( rpcCrystalCrit),
		    "Selected critical path",
		    selpath + 1, oStageBag.contents.bag.name);
	    echoCmd( RCbuf);
	}

	if( octInitGenContents( &oPathBag, OCT_BAG_MASK, &gBag) != OCT_OK){
	    /* error */
	    (void) sprintf( RCbuf, "%s: %s: %s",
			cmdName( rpcCrystalCrit),
			"Couldn't init generator",
			octErrorString() );
	    return( message( RCbuf));
	}
	while(( sBag = octGenerate( &gBag, &oStageBag)) == OCT_OK){
	    (void) sprintf( RCbuf, "%s", oStageBag.contents.bag.name);
	    echoCmd( RCbuf);
	}
	(void) sprintf( RCbuf, "Note: the decimal point may appear as a blank space");
	echoCmd( RCbuf);
	echoCmdEnd();
    } else {
	rpc_crystal_stage_dialog( &oPathBag);
    }

    return( RPC_OK);
}

/*
 * rpc_crystal_stage_dialog
 *
 * Simulate interactive bag selection of stage bags until done.
 */
rpc_crystal_stage_dialog( oPathBag)
octObject *oPathBag;
{
    int count = 0, select;
    octObject oStageBag;
    octGenerator gBag;
    dmWhichItem *stageArray;

    if( octInitGenContents( oPathBag, OCT_BAG_MASK, &gBag) != OCT_OK){
	(void) sprintf( RCbuf, "%s: %s: %s",
		    "rpc_crystal_stage_dialog",
		    "Couldn't init generator",
		    octErrorString() );
	message( RCbuf);
	return;
    }
    while( octGenerate( &gBag, &oStageBag) == OCT_OK){
	++count;
    }
    if( count == 0){
	(void) sprintf( RCbuf, "%s: %s",
		    "rpc_crystal_stage_dialog",
		    "No stages in this path");
	message( RCbuf);
	return;
    }

    stageArray = ALLOC( dmWhichItem, count);

    if( octInitGenContents( oPathBag, OCT_BAG_MASK, &gBag) != OCT_OK){
	(void) sprintf( RCbuf, "%s: %s: %s",
		    "rpc_crystal_stage_dialog",
		    "Couldn't init generator",
		    octErrorString() );
	message( RCbuf);
	return;
    }
    count = 0;
    while( octGenerate( &gBag, &oStageBag) == OCT_OK){
	(void) sprintf( RCbuf, "Stage at %s",
			oStageBag.contents.bag.name);
	stageArray[count].itemName = util_strsav( RCbuf);
	stageArray[count].flag = 0;
	stageArray[count].userData = (Pointer) oStageBag.objectId;
	++count;
    }

    /* call my interactive dm emulator					*/
    (void)idm_which_one( "Select a Stage",
		count,
		stageArray,
		&select,
		rpc_crystal_sel_item,
		"Select one stage. The net that gets high-lighted\n\
has the indicated delay time.");

    /* free memory							*/
    while( count > 0){
	count--;
	FREE(stageArray[count].itemName);
    }
    FREE( stageArray);
}

/*
 * rpc_crystal_sel_item
 *
 * select one item
 */
rpc_crystal_sel_item( item)
dmWhichItem *item;
{
    if( vemClearSelSet( RCHiSet) == VEM_OK){
	if( vemAddSelSet( RCHiSet, (octId) item->userData) == VEM_OK){
	    return;
	}
    }
    (void) sprintf( RCbuf, "%s: %s",
		"rpc_crystal_sel_item",
		"Couldn't add item");
    message( RCbuf);
}

/*
 * rpcHelpClearSet
 */
rpcHelpClearSet( syn, desc)
char **syn, **desc;
{
    *syn = "";

    *desc = "Clear all objects from the critical path select set";
}

/*
 * rpcCrystalClearSet
 *
 * Clear the RCHiSet.
 */
/*ARGSUSED*/
int 
rpcCrystalClearSet( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    if( RCHiSet != (vemSelSet) 0){
	if( vemClearSelSet( RCHiSet) != VEM_OK){
	    (void) sprintf( RCbuf, "%s: %s",
		    cmdName( rpcCrystalClearSet),
		    "Couln't clear the select set");
	    return( message( RCbuf));
	}
    }
    return( RPC_OK);
}
