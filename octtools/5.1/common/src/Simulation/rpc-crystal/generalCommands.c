/*
 * generalCommands.c
 *
 * Commands on the general menu.
 */

#include "rpc_crystal.h"

extern char *cmdName();
extern char *rpc_crystal_command_dialog();
extern char RCbuf[];
extern vemSelSet RCHiSet;

extern unsigned cOctStatus;
extern unsigned rpcCrystalStatus;
extern octObject oFacet;

/*
 * rpcHelpCommand
 */
rpcHelpCommand( syn, desc)
char **syn, **desc;
{
    *syn = "[\"text\"]";

    *desc = "Issue a textual command to the regular text-based crystal\n\
command interpretter. If the text argument is ommitted, a dialog is\n\
started to get the command and its arguments.";
}

/*
 * rpcCrystalCommand
 *
 * usage - ["text of command"] : crystal-command
 *
 * The definitive general command, this routine simply fires the user's
 * string to the crystal command interpretter.
 *
 * Planned: if there is no text string, do a dmWhichOne of all crystal
 * commands, get the correct command, then put up a dmSomething to
 * gather another text arg.
 */
/*ARGSUSED*/
int 
rpcCrystalCommand( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    int clen;
    struct RPCArg *rText;
    char *cmd, cmdbuf[ 512 ];

    clen = lsLength( cmdList);
    if( clen > 1){
	return( usage( rpcCrystalCommand));
    } else if( clen == 1){
	if(( lsDelBegin( cmdList, (Pointer *)&rText) != LS_OK)
	    || (rText->argType != VEM_TEXT_ARG)){
	    return( usage( rpcCrystalCommand));
	} else {
	    cmd = rText->argData.string;
	}
    } else {
	/*
	sprintf( RCbuf, "%s: %s",
	    cmdName( rpcCrystalCommand),
	    "fancy command gatherer not implemented");
	message( RCbuf);
	*/
	cmd = rpc_crystal_command_dialog( cmdbuf);
    }
    CmdDo( cmd);
    echoCmd( cmd);
    echoCmdEnd();

    /* the user is probably doing a command that prints some data
     * from crystal's data structure, so this will usually be appropriate
     */
    fflush( stderr);
    fflush( stdout);

    return( RPC_OK);
}

/*
 * rpcHelpFlushText
 */
rpcHelpFlushText( syn, desc)
char **syn, **desc;
{
    *syn = "";

    *desc = "Flush standard output and standard error files to\n\
the terminal that is running the vem process.\n\
This is useful to synchronize text-based crystal's output\n\
(which doesn't go through RPC).";
}

/*
 * rpcCrystalFlushText
 *
 * usage - : crystal-flush-text
 *
 * Hopefully a temporary command, this is a synchronizing function
 * that flushes stderr and stdout to the xterm where vem is running.
 */
/*ARGSUSED*/
int 
rpcCrystalFlushText( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    fflush( stderr);
    fflush( stdout);
    return( RPC_OK);
}

/*
 * rpcHelpBuild
 */
rpcHelpBuild( syn, desc)
char **syn, **desc;
{
    *syn = "";

    *desc = "Read the facet in the window running rpc-crystal.";
}

/*
 * rpcCrystalBuild
 *
 * usage: no arguments.
 *
 * Build the crystal data structure for the facet of the window in which
 * rpc-crystal was started.
 */
/*ARGSUSED*/
int 
rpcCrystalBuild( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    if( cOctStatus & COCT_BUILD_DONE){
	sprintf( RCbuf, "already built %s:%s:%s",
		    oFacet.contents.facet.cell,
		    oFacet.contents.facet.view,
		    oFacet.contents.facet.facet);
	message( RCbuf);
    } else {
	cOctBuildNet( NIL(char));
	rpcCrystalStatus |= RPC_CRYSTAL_BUILD_DONE;
	sprintf( RCbuf, "done building %s:%s:%s",
		    oFacet.contents.facet.cell,
		    oFacet.contents.facet.view,
		    oFacet.contents.facet.facet);
	message( RCbuf);
    }

    return( RPC_OK);
}

/*
 * rpcHelpExit
 */
rpcHelpExit( syn, desc)
char **syn, **desc;
{
    *syn = "";

    *desc = "Exit the RPC application.";
}

/*
 * rpcCrystalExit
 *
 * usage: No arguments.
 *
 * exit RPC crystal
 */
/*ARGSUSED*/
int 
rpcCrystalExit( spot, cmdList, uow )
RPCSpot *spot;		/* window, facet, and mouse location		*/
lsList cmdList;		/* VEM arguement list				*/
long uow;		/* for future use				*/
{
    if( lsLength( cmdList) != 0){
	usage( rpcCrystalExit);
    } else {
	if( RCHiSet != (vemSelSet) 0){
	    (void)vemClearSelSet( RCHiSet);
	}
	RPCExit( RPC_OK);
    }
    return( RPC_OK);
}
