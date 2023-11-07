/*
 * helpCommands.c
 *
 * Give on-line help for rpc and crystal commands.
 */

#include "rpc_crystal.h"

extern char RCbuf[];
extern char *cmdName();
extern RPCFunction CommandArray[];
extern s_helper UsageArray[];

/* imported from crystal's commands.c					*/
extern char *(cmds[]);

static int CrystalHelpCount;
static dmWhichItem *CrystalHelpArray;

static int RpcHelpCount;
static dmWhichItem *RpcHelpArray;

int rpc_crystal_dummy_function();

/*
 * rpc_crystal_command_dialog
 *
 * Give help with the crystal-command rpc command.  This uses crystal's
 * help array to guide the user in formualting a legal command.
 */
char *
rpc_crystal_command_dialog( cmdbuf)
char cmdbuf[];
{
    vemStatus ret;
    int selected;
    dmTextItem textItem;

    static char *ccHelp = "Select one of the text-based crystal commands.\n\
If arguments are needed for the command, another\n\
dialog box will be posted to gather them. The final\n\
command will then be passed to the standard crystal\n\
textual command interpretter.";

    /* post the help dialog window					*/
    ret = dmWhichOne( "Crystal Commands",
		CrystalHelpCount,
		CrystalHelpArray,
		&selected,
		rpc_crystal_dummy_function,
		ccHelp);

    /* get the args to the command					*/
    if(( ret == VEM_NOSELECT) || (selected == -1)){
	return( NIL(char));
    }

    /* select worked, now get the rest of the command			*/
    if( CrystalHelpArray[ selected ].userData == (Pointer) NIL(char)){
	strcpy( cmdbuf, (char *) CrystalHelpArray[ selected ].itemName);
    } else {
	sprintf( RCbuf, "Complete the crystal command: `%s'",
	    CrystalHelpArray[ selected ].itemName);
	textItem.itemPrompt = "command";
	textItem.userData = (Pointer) 0;
	textItem.rows = 1;
	textItem.cols = 50;
	sprintf( cmdbuf, "%s", (char *) CrystalHelpArray[ selected ].userData);
	textItem.value = cmdbuf;

	ret = dmMultiText( RCbuf, 1, &textItem);
	if( ret == VEM_NOSELECT){
	    return( NIL(char));
	} else {
	    strcpy( cmdbuf, textItem.value);
	}
    }

    return( cmdbuf);
}

/*
 * rpc_crystal_init_command_dialog
 *
 * This routine is called by UserMain to initialize the structures
 * used by rpcCrystalCommand().
 */
rpc_crystal_init_command_dialog()
{
    int i, n;

    /* count the crystal commands					*/
    for( i = 0; cmds[i] != NIL(char); ++i) ;

    CrystalHelpCount = i;
    CrystalHelpArray = ALLOC( dmWhichItem, CrystalHelpCount);

    for( i = 0; i < CrystalHelpCount; ++i){
	CrystalHelpArray[i].itemName = cmds[i];
	CrystalHelpArray[i].flag = 0;
	/* strip off the first word (the command name)			*/
	for( n = 0; (cmds[i][n] != ' ') && (cmds[i][n] != '\0'); ++n) ;
	if( cmds[i][n] == '\0'){
	    CrystalHelpArray[i].userData = (Pointer) NIL(char);
	} else {
	    CrystalHelpArray[i].userData = (Pointer) ALLOC( char, n + 1);
	    strncpy( (char *) CrystalHelpArray[i].userData, cmds[i], n);
	    CrystalHelpArray[i].userData[n] = '\0';
	}
    }
}

/*
 * rpc_crystal_dummy_fuction
 *
 * do nothing.
 */
rpc_crystal_dummy_function(){}

/*
 * rpcHelpHelp
 */
rpcHelpHelp( syn, desc)
char **syn, **desc;
{
    *syn = "[\"command name\"]";

    *desc = "Show command synopses and descriptions.\n\
If a command name is given, the command is searched\n\
for by menu binding, otherwise a dialog is started\n\
to get the command.";
}

/*
 * rpcCrystalHelp
 *
 * Give help for an rpc-crystal command.
 */
/*ARGSUSED*/
int
rpcCrystalHelp( spot, cmdList, uow )
RPCSpot *spot;          /* window, facet, and mouse location            */
lsList cmdList;         /* VEM arguement list                           */
long uow;               /* for future use                               */
{
    int cmdlen, index;
    char *synopsis, *description;
    RPCArg *rText;

    cmdlen = lsLength( cmdList);
    if( cmdlen > 1) {
	return( usage( rpcCrystalHelp));
    } else if( cmdlen == 1){
	if(( lsDelBegin( cmdList, (Pointer *)&rText) != LS_OK)
		|| (rText->argType != VEM_TEXT_ARG)){
	    return( usage( rpcCrystalHelp));
	} else {
	    /* get the index by string					*/
	    index = rpc_crystal_name_func_index( rText->argData.string);
	}
    } else {
	index = rpc_crystal_dialog_func_index();
    }

    if( index >= 0){
	UsageArray[index].usageHelpFunct( &synopsis, &description);
	sprintf( RCbuf, "%s : %s", synopsis, CommandArray[index].menuString);
	(void)dmConfirm( RCbuf, description, "Continue", NIL(char));
    } else if( cmdlen == 1){
	/* looked for name and couldn't find it				*/
	sprintf( RCbuf, "%s: no menu binding for \"%s\"",
		cmdName( rpcCrystalHelp), rText->argData.string);
	message( RCbuf);
    }

    return( RPC_OK);
}

/*
 * rpc_crystal_dialog_func_index
 *
 * search for a command with the specified menu binding.
 * Return the index if found, or -1 if not found.
 */
rpc_crystal_dialog_func_index()
{
    int index;
    vemStatus ret;

    /* post the help dialog window					*/
    ret = dmWhichOne( "Help for which RPC Crystal command?",
		RpcHelpCount,
		RpcHelpArray,
		&index,
		rpc_crystal_dummy_function,
		"Pick the command you want help for");

    /* get the args to the command					*/
    if(( ret == VEM_NOSELECT) || (index == -1)){
	return( -1 );
    }
    return( index);
}

/*
 * rpc_crystal_name_func_index
 *
 * Search for a command with the specified menu binding.
 * Return the index if found, or -1 if not found.
 */
rpc_crystal_name_func_index( name)
char *name;
{
    int i;

    for( i = 0; i < RpcHelpCount; ++i){
	if( !strcmp( CommandArray[i].menuString, name)) break;
    }
    if( i == RpcHelpCount){
	return( -1 );
    }
    return( i );
}

/*
 * rpc_crystal_init_rpc_help
 *
 * Set up the data structures for help for RPC commands.
 */
rpc_crystal_init_rpc_help( cmdcount)
int cmdcount;
{
    int i;

    RpcHelpCount = cmdcount;
    RpcHelpArray = ALLOC( dmWhichItem, RpcHelpCount);

    for( i = 0; i < RpcHelpCount; ++i){
	RpcHelpArray[i].itemName = CommandArray[i].menuString;
	RpcHelpArray[i].flag = 0;
	RpcHelpArray[i].userData = (Pointer) NIL(char);
    }
}
