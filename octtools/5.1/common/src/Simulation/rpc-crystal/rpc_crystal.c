/*
 * rpc_crystal.c
 *
 * UserMain and other functions for rpc-crystal.
 */

#include "rpc_crystal.h"

extern octObject oFacet;
extern octObject oCrystalFlowBag;
extern char *cOctGetNetName(), *cOctInsertBackSlash();

/* globals necessary for compilation					*/
int RepeatLimit = 1;
char *program_name = "rpc-crystal";

/* status varialble for oct crystal					*/
extern unsigned cOctStatus;

/* status variable for rpc-crystal					*/
unsigned rpcCrystalStatus;

/* general purpose buffer used by many command functions		*/
char RCbuf[ 1024 ];

/* Hilite set 								*/
vemSelSet RCHiSet;

/* rpc-crystal functions						*/
/* simulation control							*/
extern int rpcCrystalDelay(), rpcHelpDelay();
extern int rpcCrystalCrit(), rpcHelpCrit();

/* editing								*/
extern int rpcCrystalCreateFlow(), rpcHelpCreateFlow();
extern int rpcCrystalSetFlowByName(), rpcHelpSetFlowByName();
extern int rpcCrystalSetFlowByBag(), rpcHelpSetFlowByBag();
extern int rpcCrystalDeleteFromFlow(), rpcHelpDeleteFromFlow();
extern int rpcCrystalSetTermDir(), rpcHelpSetTermDir();
extern int rpcCrystalShowTermDir(), rpcHelpShowTermDir();
extern int rpcCrystalIdent(), rpcHelpIdent();

/* random/general functions						*/
extern int rpcCrystalBuild(), rpcHelpBuild();
extern int rpcCrystalCommand(), rpcHelpCommand();
extern int rpcCrystalClearSet(), rpcHelpClearSet();
extern int rpcCrystalFlushText(), rpcHelpFlushText();
extern int rpcCrystalHelp(), rpcHelpHelp();
extern int rpcCrystalExit(), rpcHelpExit();

RPCFunction CommandArray[] = {
    { rpcCrystalBuild, 		"general", 	"crystal-build", 	"", 0},
    { rpcCrystalCommand,	"general", 	"crystal-command", 	"", 0},
    { rpcCrystalClearSet,	"general", 	"crystal-clear-sel-set", "", 0},
    { rpcCrystalFlushText, 	"general", 	"crystal-flush-text", 	"", 0},
    { rpcCrystalHelp, 		"general", 	"crystal-help", 	"", 0},
    { rpcCrystalExit, 		"general", 	"crystal-exit", 	"", 0},

    { rpcCrystalCreateFlow,	"edit", 	"crystal-create-flow", 	"", 0},
    { rpcCrystalSetFlowByName,	"edit",        "crystal-set-flow-by-name","",0},
    { rpcCrystalSetFlowByBag,	"edit", 	"crystal-set-flow-by-bag","",0},
    { rpcCrystalDeleteFromFlow,	"edit",        "crystal-delete-from-flow","",0},
    { rpcCrystalSetTermDir,	"edit", 	"crystal-set-term-dir",	"", 0},
    { rpcCrystalShowTermDir,	"edit", 	"crystal-show-term-dir","", 0},
    { rpcCrystalIdent, 		"edit", 	"crystal-ident", 	"", 0},

    { rpcCrystalDelay, 		"timing", 	"crystal-delay", 	"", 0},
    { rpcCrystalCrit, 		"timing", 	"crystal-critical", 	"", 0},
};

/* Usage array, must be in same order as CommandArray			*/
s_helper UsageArray[] = {
    rpcHelpBuild, 
    rpcHelpCommand,
    rpcHelpClearSet,
    rpcHelpFlushText,
    rpcHelpHelp,
    rpcHelpExit,

    rpcHelpCreateFlow,
    rpcHelpSetFlowByName,
    rpcHelpSetFlowByBag,
    rpcHelpDeleteFromFlow,
    rpcHelpSetTermDir,
    rpcHelpShowTermDir,
    rpcHelpIdent, 

    rpcHelpDelay,
    rpcHelpCrit,
};

/*
 * UserMain 
 *
 * Initializes the application.
 */
/*ARGSUSED*/
long
UserMain(display, spot, argList, uow, array )
char *display;		/* name of X display				*/
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList argList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
RPCFunction **array;	/* the CommandArray[]				*/
{
    char *version();
    *array = CommandArray;

    /* initialize crystal						*/
    initCrystal();
    rpc_crystal_init_command_dialog();
    rpc_crystal_init_rpc_help( sizeof( CommandArray ) / sizeof( RPCFunction ));

    /* initialize the OCT layer						*/
    cOctStatus = COCT_ON;
    rpcCrystalStatus = 0;
    oFacet.objectId = spot->facet;

    /* must be the same as in cOctBuildNet() (octBuild.c)		*/
    oCrystalFlowBag.objectId = oct_null_id;
    oCrystalFlowBag.type = OCT_BAG;
    oCrystalFlowBag.contents.bag.name = "CRYSTAL_FLOWS";

    /* get the fancy hilite set						*/
    RCHiSet = vemNewSelSet( oFacet.objectId,
		    (unsigned short)( 60000),			/*RGB	*/
		    (unsigned short)( 60000),
		    (unsigned short)(  6000),
		    1, 1,				/* line style	*/
		    6, 6,				/* fill pat	*/
		    "100000 010000 001000 000100 000010 000001");

    /* give the user some indication that things are ready to go	*/
    (void)vemMessage( version(), MSG_DISP );
    vemPrompt();
    return( sizeof( CommandArray ) / sizeof( RPCFunction ));
}

static char usageBuffer[512];
/*
 * usage
 * 
 * Format and send a usage error message.
 */
int
usage( function)
int (*function)();
{
    int index;
    char *synopsis, *description;

    for( index = sizeof(CommandArray) / sizeof(RPCFunction) - 1;
	 index >= 0; index-- ){
	if( function == CommandArray[index].function) break;
    } 
    if( index < 0){
	message( "Internal error - usage function corrupted");
    } else {
	UsageArray[index].usageHelpFunct( &synopsis, &description);
	sprintf( usageBuffer, "usage - %s : %s",
		 synopsis, CommandArray[index].menuString);
	message( usageBuffer);
    }
    return(RPC_OK);
}

static char messageBuffer[512];
/*
 * message
 *
 * Sends messageString to the vem console.
 */
int
message( messageString )
char *messageString;
{
    sprintf( messageBuffer, "\n0rpc-crystal:2 %s0\n", messageString);
    (void)vemMessage( messageBuffer, MSG_DISP );
    vemPrompt();
    return( RPC_OK );
}

/*
 * echoCmd
 *
 * Same as message, but doesn't append the "rpc-crystal:" string.
 */
echoCmd( messageString)
char *messageString;
{
    sprintf( messageBuffer, "\n2%s0", messageString);
    (void)vemMessage( messageBuffer, MSG_DISP );
}

/*
 * echoCmdEnd
 *
 * terminate echoing a series of commands.
 */
echoCmdEnd()
{
    (void)vemMessage( "\n", MSG_DISP);
    vemPrompt();
}

/*
 * cmdName
 * 
 * return the command name binding of the given function.
 */
char *
cmdName( function)
int (*function)();
{
    int index;

    for( index = sizeof(CommandArray) / sizeof(RPCFunction) - 1;
	 index >= 0; index-- ){
	if( function == CommandArray[index].function) break;
    } 
    if( index < 0){
	message( "cmdName: Internal error - function not found");
	return( "unknown function");
    } else {
	return( CommandArray[index].menuString);
    }
}
