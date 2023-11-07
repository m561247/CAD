/***********************************************************************
 * Handle reading and writing the state of the network.
 * Filename: state.c
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
 *	int write_node_state() - write the state of a node.
 *	void SaveState() - save the state of the network.
 *	int rpcMusaSaveState() - rpc interface to 'savestate' command.
 *	int do_restart() - restart instance.
 *	int read_node_state() - read the state of a node.
 *	void LoadState() - load the state of the network.
 *	int rpcMusaLoadState() - rpc interface to 'loadstate' command.
 *	int InitNode() - initialize node.
 *	void InitState() - initialize state of the network.
 *	int rpcMusaInitState() - rpc interface to 'initstate' command.
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define STATE_MUSA
#include "musa.h"
#include "vov.h"

static FILE *datafile;
#define MUSAHEADER	"musa data file"

/*
 *  Fancy macros courtesy of Peter Moore and Rick Rudell
 *
 *  UNSAFE MACROs to input an integer
 *  fp should be a "register FILE *" for best performance.
 *  Unsafe to pass variables used within
 */

#define OUTPUT_INT(fp, value) {\
	register int byte0, byte1, v;\
	int negative, byte2, byte3, cnt;\
	negative = value < 0;\
	v = negative ? -value : value;\
	byte3 = ((unsigned) v >> 24) & 255;\
	byte2 = ((unsigned) v >> 16) & 255;\
	byte1 = ((unsigned) v >> 8) & 255;\
	byte0 = v & 255;\
	if (byte3 != 0) cnt = 4; else if (byte2 != 0) cnt = 3;\
	else if (byte1 != 0) cnt = 2; else if (byte0 != 0) cnt = 1;\
    else cnt = 0;\
    if (negative) {putc(cnt+5, fp);} else {putc(cnt, fp);}\
    switch(cnt) {\
	case 4: putc(byte3, fp);\
	case 3: putc(byte2, fp);\
	case 2: putc(byte1, fp);\
	case 1: putc(byte0, fp);\
    }\
} /* OUTPUT_INT... */

#define INPUT_INT(fp, value, status) {\
    register int v = 0;\
    status = 1;\
    switch(getc(fp)) {\
	case 9: v = getc(fp);\
	case 8: v = (v << 8) + getc(fp);\
	case 7: v = (v << 8) + getc(fp);\
	case 6: v =  - ((v << 8) + getc(fp));\
	    break;\
	case 4: v = getc(fp);\
	case 3: v = (v << 8) + getc(fp);\
	case 2: v = (v << 8) + getc(fp);\
	case 1: v = (v << 8) + getc(fp);\
	case 0: break;\
	default: status = 0;\
    }\
    value = v;\
} /* INPUT_INT... */

/*************************************************************************
 * WRITE_NODE_STATE
 * Write the state of 'theNode'.
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 */
void write_node_state(theNode)
	node *theNode;
{
	if (putc(theNode->voltage , datafile) == EOF) {
		Warning("Error occured while writing file");
		sim_status |= SIM_UNKNOWN;
	} 
} /* write_node_state... */

/*************************************************************************
 * SAVESTATE
 * Save state of network.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - the arguments. (Input)
 */
void SaveState(argc, argv)
	int argc;
	char **argv;
{
#ifdef RPC_MUSA
	(void) ignore((char *) &argc);
#else
	if (argc < 1) {
		Usage("SaveState filename\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	if (events_pending()) {
		Warning("Events pending, unable to save state.");
		sim_status |= SIM_UNKNOWN;
		return;
	} 
	VOVoutput( VOV_UNIX_FILE, argv[0] );
	if ((datafile = fopen( util_file_expand(argv[0]), "w")) == NIL(FILE)) {
		(void) sprintf(errmsg, "Can't open \"%s\"", argv[0]);
		Warning(errmsg);
		sim_status |= SIM_UNKNOWN;
		return;
	} 

	(void) fputs(MUSAHEADER,datafile);
	OUTPUT_INT(datafile, number_of_nodes)
	ForEachNode(write_node_state);
	(void) fclose(datafile);
} /* SaveState... */

/**************************************************************************
 * RPCMUSASAVESTATE
 * Rpc interface to 'savestate' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
#ifdef RPC_MUSA
int rpcMusaSaveState (spot,cmdList,cominfo)
    RPCSpot     *spot;      /* window, facet, and mouse location */
    lsList      cmdList;    /* VEM argument list */
    int32       cominfo;
{
	struct	RPCArg		*arg;
	static dmTextItem	item = {"File Name",1,50,"",NULL};
	int					len = lsLength(cmdList);

	(void) ignore((char *) spot);

    if ((len > 1) || 
		((len == 1) &&
		((lsDelBegin(cmdList,(lsGeneric *) &arg) != LS_OK) ||
		(arg->argType != VEM_TEXT_ARG)))) {
		Format("musa-savestate",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
	if (len == 0) {
		if ((dmMultiText("musa-savestate",1,&item) == VEM_OK) &&
			(strlen(item.value) > 0)) {
			SaveState (1,&(item.value));
		} else {
			return (RPC_OK);
		} /* if... */
	} else {
		SaveState (1,&(arg->argData.string));
	} /* if... */			
	sim_status &= !SIM_UNKNOWN;
	return (RPC_OK);
} /* rpcMusaSaveState... */
#endif

/**************************************************************************
 * DO_RESTART
 * Restart each instance
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 */
int do_restart(inst)
	musaInstance *inst;
{
    (*deviceTable[(int) inst->type].restart_routine)(inst);
} /* do_restart... */

/***************************************************************************
 * READ_NODE_STATE
 * Read state of 'theNode'.
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 */
void read_node_state(theNode)
	node *theNode;
{
	int temp;

	if ((temp = getc(datafile)) == EOF) {
		Warning("Error occured while reading file");
		sim_status |= SIM_UNKNOWN;
		return;
	} /* if... */
	theNode->voltage = temp;
	switch (theNode->voltage) {
		case L_I:
			theNode->numberOfHighDrivers = 1;
			theNode->numberOfLowDrivers = 0;
			theNode->highStrength = 0;
			theNode->lowStrength = 255;
			break;
		case L_O:
			theNode->numberOfLowDrivers = 1;
			theNode->numberOfHighDrivers = 0;
			theNode->lowStrength = 0;
			theNode->highStrength = 255;
			break;
		default:
			theNode->numberOfLowDrivers = theNode->numberOfHighDrivers = 0;
			theNode->lowStrength = theNode->highStrength = 255;
			break;
	} /* switch... */
} /* read_node_state... */

/**************************************************************************
 * LOADSTATE
 * Load state of network.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - the arguments. (Input)
 */
void LoadState(argc, argv)
	int		argc;
	char	**argv;
{
	char	buffer[80];
	int		value, status;

#ifdef RPC_MUSA
	(void) ignore((char *) &argc);
#else
	if (argc < 1) {
		Usage("loadstate filename\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	if (events_pending()) {
		dump_events();
	} /* if... */
	VOVinput( VOV_UNIX_FILE, argv[0] );
	if ((datafile = fopen( util_file_expand(argv[0]), "r")) == NIL(FILE)) {
		(void) sprintf(errmsg, "Can't open \"%s\"", argv[0]);
		Warning(errmsg);
		sim_status |= SIM_UNKNOWN;
		return;
	} /* if... */

	(void) fgets(buffer, sizeof(MUSAHEADER), datafile);
	if (strcmp(buffer, MUSAHEADER) != 0) {
		Warning("Can't read file format (data preserved)");
		sim_status |= SIM_UNKNOWN;
		(void) fclose(datafile);
		return;
	} /* if... */

	INPUT_INT(datafile, value, status)
	if (status == 0) {
		Warning("Can't read file format (data preserved)");
		sim_status |= SIM_UNKNOWN;
		(void) fclose(datafile);
		return;
	} /* if... */
	if (value != number_of_nodes) {
		Warning("Data file out of date (data preserved)");
		sim_status |= SIM_UNKNOWN;
		(void) fclose(datafile);
		return;
	} /* if... */

	ForEachNode(read_node_state);
	/* check file length */
	if (feof(datafile) || (getc(datafile) != EOF)) {
		FatalError("Data file is bad : Can't recover!!");
	} /* if... */
	(void) fclose(datafile);
	disable_sched2 = TRUE;
	ForEachMusaInstance(do_restart);
	Evaluate(0, NIL(char *), TRUE);
	disable_sched2 = FALSE;
} /* LoadState... */

/**************************************************************************
 * RPCMUSALOADSTATE
 * Rpc interface to 'loadstate' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
#ifdef RPC_MUSA
int rpcMusaLoadState (spot,cmdList,cominfo)
    RPCSpot     *spot;      /* window, facet, and mouse location */
    lsList      cmdList;    /* VEM argument list */
    int32       cominfo;
{
	struct	RPCArg		*arg;
	static dmTextItem	item = {"File Name",1,50,"",NULL};
	int					len = lsLength(cmdList);

	(void) ignore((char *) spot);

    if ((len > 1) || 
		((len == 1) &&
		((lsDelBegin(cmdList,(lsGeneric *) &arg) != LS_OK) ||
		(arg->argType != VEM_TEXT_ARG)))) {
		Format("musa-loadstate",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
	if (lsLength(cmdList) == 0) {
		if ((dmMultiText("musa-loadstate",1,&item) == VEM_OK) &&
			(strlen(item.value) > 0)) {
			LoadState (1,&(item.value));
		} else {
			return (RPC_OK);
		} /* if... */
	} else {
		LoadState (1,&(arg->argData.string));
	} /* if... */
	sim_status &= !SIM_UNKNOWN;
	rpcMusaUpdateSelSet();
	return (RPC_OK);
} /* rpcMusaLoadState... */
#endif

/**************************************************************************
 * INITNODE
 * Initialize node.
 *
 * Parameters:
 *	theNode (node *) - the node. (Input)
 */
int InitNode (theNode)
    node	*theNode;
{
    printf( "OLD initNode" );
    theNode->data = 0;
    theNode->voltage = L_x;
    theNode->numberOfLowDrivers = 0;
    theNode->numberOfHighDrivers = 0;
    theNode->lowStrength = 255;
    theNode->highStrength = 255;
}				/* InitNode... */

/**************************************************************************
 * INITSTATE
 * Initialize state of network.
 *
 * Parameters:
 *	argc (int) - the argument count. (Input)
 *	argv (char **) - the arguments. (Input)
 */
void InitState (argc, argv)
	int		argc;
	char	**argv;
{
	(void) ignore((char *) argv);

#ifdef RPC_MUSA
	(void) ignore((char *) &argc);
#else
	if (argc > 0) {
		Usage("initstate\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	if (events_pending()) {
		dump_events();
	} /* if... */
	ForEachNode( init_node );
	disable_sched2 = TRUE;
	ForEachMusaInstance(do_restart);
	Evaluate(0, NIL(char *), TRUE);
	disable_sched2 = FALSE;
	init_sched();
} /* InitState... */

/**************************************************************************
 * RPCMUSAINITSTATE
 * Rpc interface to 'initstate' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
#ifdef RPC_MUSA
int rpcMusaInitState (spot,cmdList,cominfo)
    RPCSpot     *spot;      /* window, facet, and mouse location */
    lsList      cmdList;    /* VEM argument list */
    int32       cominfo;
{
	(void) ignore((char *) spot);

    if (lsLength(cmdList) != 0) {
		Format("musa-initstate",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
	InitState (0,NIL(char *));
	sim_status &= !SIM_UNKNOWN;
	rpcMusaUpdateSelSet();
	return (RPC_OK);
} /* rpcMusaInitState... */
#endif
