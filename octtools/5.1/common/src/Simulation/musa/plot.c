/***********************************************************************
 * Module to handle plotting
 * Filename: plot.c 
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
 *	void NextMacroTime() - Increment the macro time.
 *	static Graph *GetGraph() - Retrieve graph structure associated with name.
 *	void SetPlot() - 'setplot' command.
 *	int rpcMusaSetPlot() - Rpc interface to 'setplot' command.
 *	static int16 AddToPlot() - Add a plot to a graph.
 *	void Plot() - 'plot' command.
 *	int rpcMusaPlot - Rpc interface to 'plot' command.
 *	void SavePlot() - 'saveplot' command.
 *	int rpcMusaSavePlot() - Rpc interface to 'saveplot' command.
 *	void DestroyPlot() - 'destroyplot' command.
 *	int rpcMusaDestoryPlot() - Rpc interface to 'destroyplot' command.
 *	int plot_info() - Display plot probe information.
 *	int sim_plot() - Simulate plot probe.
 *	void UpdatePlot() - Update all plots to current time.
 * Modifications:
 *	Andrea Casotto			Created					1988
 *	Rodney Lai				Ported to X11			1989
 *							RPC interface routines
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define PLOT_MUSA
#include "musa.h"

typedef struct {
	XGraph	xg;
	char	*name;
	int16	curNumberOfPlots;
	char	*plotName[XG_MAXSETS];
	musaInstance	*parent[XG_MAXSETS];
	musaInstance	*inst[XG_MAXSETS];
} Graph;

typedef struct {
	XGraph	xg;
	double	offset;		/* in the graph */
	double	lastMacroTime;
	double	lastMicroTime;
	double	lastPlottedValue;
} graphInstance;

static double macroTime = 0.0;
static double microTime = 0.0;
static double macroTimeStep = 1.0;
static double microTimeStep = 0.01;
static double defaultTimeWindow = 100.0;
static double windowStart = 0.0;
static double windowPadding = 1.0;

/********************************************************************
 * NEXTMACROTIME
 * Increment macro time.
 *
 * Parameters:
 *	None.
 */
void NextMacroTime()
{
	macroTime += macroTimeStep;
	microTime = 0.0;
	if ( macroTime - windowStart > defaultTimeWindow / 2.0) {
		windowStart += defaultTimeWindow / 2.0;
	} /* if... */
} /* NextMacroTime... */

/************************************************************************
 * GETGRAPH
 * Retrieve the 'Graph' structure associated with the specified 'name'.
 *
 * Parameters:
 *	name (char *) - the name of the graph. (Input)
 *
 * Return Value:
 *	The 'Graph' structure for the specified graph. (Graph *)
 */
static Graph *GetGraph(name)
	char *name;
{
	Graph  *graph;
    
	if ((graphList == NIL(st_table)) || 
		!st_lookup(graphList, name, (char **) &graph)) {
		return (NIL(Graph));
	} else {
		return(graph);
	} /* if... */
} /* GetGraph... */

/************************************************************************
 * SETPLOT
 * Open a plot window.
 *
 * Parameters:
 *	argc (int) - number of arguments passed to the command. (Input)
 *	argv (char **) - the arguments passed to the command. (Input)
 */
void SetPlot(argc, argv)
	int			argc;
	char		**argv;
{
	Graph		*graph;
	int			width, height, llx , lly;
#ifdef RPC_MUSA
	static int	zero = 0;
	int			readmask;
#endif

	if (!MusaOpenXDisplay()){
		return;
	} /* if... */

#ifndef RPC_MUSA
	if ((argc == 0) || (argc == 2) || (argc == 4) || (argc > 5)) {
		Usage("setplot graph_name [width height] [llx lly]\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif

	if (argc > 1) {
		width = atoi(argv[1]);
		height= atoi(argv[2]);
	} else {
		width = height = -1;
	} /* if... */
	if (argc > 3) {
		llx   = atoi(argv[3]);
		lly   = atoi(argv[4]);
	} else {
		llx = lly = -1;
	} /* if... */

	if (GetGraph(argv[0])) {
		Warning("Graph already exists.");
		return;
	} /* if... */
	if ((graph = ALLOC(Graph,1)) == NIL(Graph)) {
		Warning("Unable to allocate graph structure.");
		return;
	} /* if... */
	graph->name = util_strsav(argv[0]);
	graph->curNumberOfPlots = 0;
	if ((graph->xg = xgNewWindow(theDisp,progName, graph->name , 
		"time" ,"Y" , width, height, llx, lly)) == (XGraph) NULL) {
		Warning("Unable to open plot window.");
		return;
	} /* if... */
	xgClearFlags(graph->xg,XGGridFlag);
	st_add_direct(graphList, graph->name, (char *) graph);
#ifdef RPC_MUSA
	readmask = (1<<ConnectionNumber(theDisp));
	RPCUserIO(ConnectionNumber(theDisp)+1,&readmask,&zero,&zero,
		RPCHandleXEvents);
#endif
} /* SetPlot... */

#ifdef RPC_MUSA
/***********************************************************************
 * RPCMUSASETPLOT
 * Rpc interface routine to 'setplot'.
 *
 * Parameters:
 *	spot (RPCSpot *) - the RPC spot structure. (Input)
 *	cmdList (lsList) - the argument list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaSetPlot(spot,cmdList,cominfo)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
	int32       cominfo;
{
	int					len = lsLength(cmdList);
	struct RPCArg		*name,*width,*height,*x,*y;
	char				*argList[5];
	static dmTextItem	items[5] = {
		{"Graph Name",1,32,"",NULL},
		{"Width",1,8,"",NULL},
		{"Height",1,8,"",NULL},
		{"X-Coordinate",1,8,NULL},
		{"Y-Coordinate",1,8,NULL}};

	(void) ignore((char *) spot);

	if (((len == 2) || (len == 4) || (len > 5)) ||
		((len > 0) &&
		((lsDelBegin(cmdList,(lsGeneric *) &name) != LS_OK) ||
		(name->argType != VEM_TEXT_ARG))) ||
		((len > 2) &&
		((lsDelBegin(cmdList,(lsGeneric *) &width) != LS_OK) ||
		(width->argType != VEM_TEXT_ARG) ||
		(lsDelBegin(cmdList,(lsGeneric *) &height) != LS_OK) ||
		(height->argType != VEM_TEXT_ARG))) ||
		((len > 4) &&
		((lsDelBegin(cmdList,(lsGeneric *) &x) != LS_OK) ||
		(x->argType != VEM_TEXT_ARG) ||
		(lsDelBegin(cmdList,(lsGeneric *) &y) != LS_OK) ||
		(y->argType != VEM_TEXT_ARG)))) {
		Format("musa-setplot",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
	if (len == 0) {
		if (dmMultiText("musa-setplot",5,items) == VEM_OK) {
			if (strlen(items[0].value) == 0) {
				Warning("No graph name specified.");
				return(RPC_OK);
			} /* if... */
			argList[len] = items[len].value;
			len++;
			if ((strlen(items[1].value) > 0) && (strlen(items[2].value) > 0)) {
				argList[len] = items[len].value;
				len++;
				argList[len] = items[len].value;
				len++;
				if ((strlen(items[3].value) > 0) && (strlen(items[4].value) > 0)) {
					argList[len] = items[len].value;
					len++;
					argList[len] = items[len].value;
					len++;
				} /* if... */
			} /* if... */	
		} else {
			return(RPC_OK);
		} /* if... */
	} else {
		argList[0] = name->argData.string;
		if (len > 2) {
			argList[1] = width->argData.string;
			argList[2] = height->argData.string;
		} /* if... */
		if (len > 4) {
			argList[3] = x->argData.string;
			argList[4] = y->argData.string;
		} /* if... */
	} /* if... */
	SetPlot(len,argList);
	return(RPC_OK);
} /* rpcMusaSetPlot... */
#endif

/*************************************************************************
 * ADDTOPLOT
 * Add a data set to the specified graph.
 *
 * Parameters:
 *	name (char *) - the name of the new data set. (Input)
 *	graph (Graph *) - the graph structure. (Input)
 *	dirmusaInstance (musaInstance *) - the instance that contains the element to plot. (Input)
 *
 * Return Value:
 *	TRUE, success.
 *	FALSE, unsuccessful.
 */
static int16 AddToPlot(name,graph,dirmusaInstance)
	char			*name;
	Graph			*graph;
	musaInstance			*dirmusaInstance;
{
	vect			*highbit,*lowbit,*v;
	node			*theNode;
	musaInstance			*newmusaInstance;
	char			instname[80];
	graphInstance	*graphInst;

	if (!LookupVect(name,&highbit,&lowbit,dirmusaInstance)) {
		(void) sprintf (errmsg,"No node or vector with name, \"%s\".",name);
		Warning (errmsg);
		return(FALSE);
	} /* if... */
	for (v = highbit;v != NIL(vect);v = v->next) {
		(void) sprintf(instname,"%s_PLOT_PROBE_%s",v->data->name,graph->name);
		if (InstLookup(dirmusaInstance,instname,&newmusaInstance)) {
			(void) sprintf(errmsg,
				"Node(\"%s\") already being plotted.",v->data->name);
			Warning(errmsg);
			return(FALSE);
		} else {
			if ((graphInst = ALLOC(graphInstance,1)) == NIL(graphInstance)) {
				Warning("Unable to allocate graph instance.");
				return(FALSE);
			} /* if... */

			theNode = ElementToNode(v->data);
			graph->plotName[graph->curNumberOfPlots] = util_strsav(v->data->name);
			graph->parent[graph->curNumberOfPlots] = parent_inst = dirmusaInstance;
			make_leaf_inst(&newmusaInstance,DEVTYPE_PLOT_PROBE,instname,1);
			graph->inst[graph->curNumberOfPlots] = newmusaInstance;

			graphInst->xg = graph->xg;
			graphInst->offset = 2.0 * graph->curNumberOfPlots++;
			graphInst->lastMacroTime = macroTime;
			graphInst->lastMicroTime = 0.0;
			graphInst->lastPlottedValue = 0.0;
			newmusaInstance->userData = (char *) graphInst;

			/* connect the node */
			newmusaInstance->connArray[0].node = theNode;
			newmusaInstance->connArray[0].next = theNode->conns;
			theNode->conns = &(newmusaInstance->connArray[0]);

			sim_plot(newmusaInstance);
		} /* if... */
	} /* for... */
	return (TRUE);
} /* AddToPlot... */

/***********************************************************************
 * PLOT
 * Add specified nodes to a graph
 *
 * Parameters:
 *	argc (int) - the number of arguments. (Input)
 *	argv (char **) - the arguments. (Input)
 *	cwd (musaInstance *) - the current working directory. (Input)
 */
void Plot(argc, argv, cwd)
	int		argc;
	char	**argv;
	musaInstance	*cwd;
{
	musaInstance			*dirmusaInstance;
	int16			i;
	Graph			*graph;
	char			*path,*name;
	int16			ok;
	watchlist		*wl;
	watchl			*w;

#ifndef RPC_MUSA
	if (argc < 2) {
		Usage("plot graph_name node_name ...\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	if ((graph = GetGraph(argv[0])) == NIL(Graph)) {
		(void) sprintf (errmsg,
			"The specified graph(\"%s\") does not exist.",argv[0]);
		Warning (errmsg);
		return;
	} /* if... */

	for (i = 1; i < argc; i++) {
		if (graph->curNumberOfPlots >= XG_MAXSETS) {
			Warning("The maximum number of plots has been reached for this graph.");
			return;
		} /* if... */

		path = util_strsav(argv[i]);
		if (name = strrchr(path,separator)) {
			*(name++) = '\0';
			(void) gnInstPath(path,&dirmusaInstance,TRUE,&ok);
		} else {
			dirmusaInstance = cwd;
			name = path;
		} /* if... */
		if (st_lookup(watchlists, name, (char **) &wl)) {
			for (w = wl->first; w != NIL(watchl); w = w->next) {
				if (!AddToPlot(w->name,graph,dirmusaInstance)) {
					return;
				} /* if... */
			} /* for... */
		} else {
			if (!AddToPlot(name,graph,dirmusaInstance)) {
				return;
			} /* if... */
		} /* if... */
		FREE(path);
	} /* for... */
	gnSetDir(cwd);
} /* Plot... */

#ifdef RPC_MUSA
/***************************************************************************
 * RPCMUSAPLOT
 * Rpc interface to the 'plot' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - the argument list. (Input)
 *	cominfo (int32) - information about command. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaPlot (spot,cmdList,cominfo)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
	int32       cominfo;
{
	struct RPCArg	*name;
	char			*argList[256];
	int				len = lsLength(cmdList);

	if ((len == 0) ||
		(lsDelEnd(cmdList,(lsGeneric *) &name) != LS_OK) ||
		(name->argType != VEM_TEXT_ARG)) {
		Format("musa-plot",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
    (void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	argList[0] = name->argData.string;
	if (len == 1) {
		(void) rpcMusaUnderSpot(spot,2,argList,1,TRUE,FALSE,Plot);
	} else { 
		rpcMusaRunRoutine (spot,cmdList,2,argList,1,TRUE,FALSE,Plot);
	} /* if... */
	return (RPC_OK);
} /* rpcMusaPlot... */
#endif

/****************************************************************************
 * SAVEPLOT
 * Save graph to a file, used for 'saveplot' command.
 *
 * Parameters:
 *	argc (int) - the number of arguments. (Input)
 *	argv (char **) - the arguments. (Input)
 */
void SavePlot(argc,argv) 
	int		argc;
	char	**argv;
{
	XGraph	xg;
	Graph	*graph;

#ifndef RPC_MUSA
	if (argc != 1 && argc != 2) {
		Usage("saveplot graph_name <filename>\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	graph = GetGraph(argv[0]);
	if (graph == NIL(Graph)) {
		(void) sprintf (errmsg,
			"The specified graph(\"%s\") does not exist.",argv[0]);
		Warning (errmsg);
		return;
	} /* if... */
	xg = graph->xg;
	if (argc==2) {
		(void) xgSaveGraph(xg,argv[1]);
	} else {
		(void) xgSaveGraph(xg,"");
	} /* if... */
} /* SavePlot... */

#ifdef RPC_MUSA
/***************************************************************************
 * RPCMUSASAVEPLOT
 * Rpc interface to 'saveplot' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - the argument list. (Input)
 *	cominfo (int32) - information about command. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaSavePlot (spot,cmdList,cominfo)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
	int32       cominfo;
{
	struct RPCArg	*arg1,*arg2;
	int				len = lsLength(cmdList);
	char			*argList[2];
	static dmTextItem	items[2] = {
		{"Graph Name",1,32,"",NULL},
		{"File Name",1,45,"",NULL}};

	(void) ignore((char *) spot);

	if ((len > 2) || 
		((len > 0) &&
		((lsDelBegin(cmdList,(lsGeneric *) &arg1) != LS_OK) ||
		(arg1->argType != VEM_TEXT_ARG))) ||
		((len == 2) &&
		((lsDelBegin(cmdList,(lsGeneric *) &arg2) != LS_OK) ||
		(arg2->argType != VEM_TEXT_ARG)))) {
		Format("musa-saveplot",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
	if (len == 0) {
		if (st_count(graphList) == 0) {
			Warning("There are no plots.");
			return(RPC_OK);
		} /* if... */
		if (dmMultiText("musa-saveplot",2,items) == VEM_OK) {
			if (strlen(items[0].value) == 0) {
				Warning("No graph name specified.");
				return(RPC_OK);
			} /* if... */
			argList[len] = items[len].value;
			len++;
			if (strlen(items[0].value) > 0) {
				argList[len] = items[len].value;
				len++;
			} /* if... */
		} else {
			return(RPC_OK);
		} /* if... */
	} else {
		argList[0] = arg1->argData.string;
		if (len == 2) {
			argList[1] = arg2->argData.string;
		} /* if... */
	} /* if... */
	SavePlot (len,argList);
	return(RPC_OK);
} /* rpcMusaSavePlot... */
#endif

/***********************************************************************
 * DESTROYPLOT
 * Destroy plot command.
 *
 * Parameters:
 *	argc (int) - the number of arguments. (Input)
 *	argv (char **) - the arguments. (Input)
 */
void DestroyPlot(argc,argv)
	int		argc;
	char	**argv;
{
	Graph	*graph;
	int16	i;
	musaInstance	*inst;
	char	name[80];

#ifdef RPC_MUSA
	(void) ignore((char *) &argc);
#else
	if (argc != 1 ) {
		Usage("destroyplot graph_name\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	if ((graphList == NIL(st_table)) || 
		!st_delete(graphList, &argv[0], (char **) &graph)) {
		Warning("The specified graph does not exist.");
		return;
	} /* if... */
	(void) xgDestroyWindow(graph->xg);
	for (i=0;i<graph->curNumberOfPlots;i++) {
		(void) sprintf (name,"%s_PLOT_PROBE_%s",graph->plotName[i],
			graph->name);
		if (InstLookup(graph->parent[i],name,&inst)) {
			FREE(inst->userData);
			remove_connect(&(inst->connArray[0]));
			InstUnlink(inst);
			ConnFree(inst->connArray,1);
			MusaInstanceFree(inst);
		} else {
			char	message[80];

			(void) sprintf(message,"'%s' plot probe does not exist.",
				graph->plotName[i]);
			Warning(message);
		} /* if... */
		FREE(graph->plotName[i]);
	} /* for... */
	FREE(graph->name);
	FREE(graph);
} /* DestroyPlot... */

#ifdef RPC_MUSA
/***************************************************************************
 * RPCMUSADESTROYPLOT
 * Rpc interface to destroy plot.
 *
 * Parameters:
 *	spot (RPCSpot *) - the spot structure. (Input)
 *	cmdList (lsList) - the argument list. (Input)
 *	cominfo (int32) - information about command. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaDestroyPlot (spot,cmdList,cominfo)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
	int32       cominfo;
{
	int				len = lsLength(cmdList);
	struct RPCArg	*arg;
	char			*select;
	lsList			list;
	char			*name;
	Graph			*graph;
	st_generator	*gen;

	(void) ignore((char *) spot);

	if ((len > 1) ||
		((len == 1) &&
		((lsDelBegin(cmdList,(lsGeneric *) &arg) != LS_OK) ||
		(arg->argType != VEM_TEXT_ARG)))) {
		Format("musa-destoryplot",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
	if (len == 0) {
		if (st_count(graphList) == 0) {
			Warning("There are no plots.");
			return(RPC_OK);
		} /* if... */
		list = lsCreate();
		gen = st_init_gen(graphList);
		while (st_gen(gen,&name,(char **) &graph) != 0) {
			lsNewBegin (list,(lsGeneric) name,LS_NH);
		} /* while... */
		st_free_gen(gen);	
		DumpList("musa-destroyplot",NIL(char),list,&select);
		if (select == NIL(char)) {
			return(RPC_OK);
		} else {
			DestroyPlot (1,&select);
		} /* if... */
	} else {
		DestroyPlot (1,&(arg->argData.string));
	} /* if... */
	return(RPC_OK);
} /* rpcMusaDestroyPlot... */
#endif

/***************************************************************************
 * PLOT_INFO
 * Display information, if instance is plot probe.
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 */
int plot_info(inst)
	musaInstance	*inst;
{
	if (inst->type == DEVTYPE_PLOT_PROBE) {
		MUSAPRINT("\n");
		ConnectInfo(inst,1);
	} /* if... */
} /* plot_info... */

/**************************************************************************
 * SIM_PLOT
 * Simulate, if instance is plot.
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 */
int sim_plot(inst)
	musaInstance			*inst;
{
    int16	voltage = inst->connArray[0].node->voltage;
    graphInstance *gi = (graphInstance *) inst->userData;
    double		x , y;
    double		v;
    double		llx, lly, urx, ury;
    char		*name,*pos;

    if (inst->type != DEVTYPE_PLOT_PROBE) {
	return;
    }				/* if... */
    MakePath (inst,&name);
    /*** Remove '_PLOT_PROBE_' extension from name ***/
    pos = name;
    while ((pos = strchr(pos,'_')) && (strncmp(pos++,"_PLOT_PROBE_",12))) {}
    *(pos-1) = '\0';
    switch (voltage) {
    case L_I: case L_1:
	v = 1.0;
	break;
    case L_i: case L_w1:
	v = 0.97;
	break;
    case L_O: case L_0:
	v = 0.0;
	break;
    case L_o: case L_w0:
	v = 0.03;
	break;
    case L_X: case L_wX : case L_x:
	v = 0.5;
	break;
    default:
	FatalError("Illegal voltage to be plotted");
    }				/* switch... */
    y = gi->offset + v;
    x = macroTime + microTime;
    
    /* Reduce flickering by controlling the range displayed in the graph */
    (void) xgGetRange(gi->xg, &llx , &urx , &lly , &ury);
    if (urx < x) {
	(void) xgSetRange(gi->xg, windowStart - windowPadding , 
			  windowStart + defaultTimeWindow + windowPadding, 
			  XG_NO_OPT, XG_NO_OPT);
    }				/* if... */
    if (gi->lastMacroTime != macroTime && gi->lastPlottedValue != y) {
	(void) xgSendPoint(gi->xg, x ,gi->lastPlottedValue, name);
    }				/* if... */
    (void) xgSendPoint(gi->xg, x, y, name);
    gi->lastPlottedValue = y;
    gi->lastMacroTime = macroTime;
    gi->lastMicroTime = microTime;
    microTime += microTimeStep;
    FREE(name);
}				/* sim_plot... */

/***************************************************************************
 * UPDATEPLOT
 * Bring all plots up to the current macro/micro time.
 *
 * Parameters:
 *	None.
 */
void UpdatePlot ()
{
	st_generator	*gen;
	char			*name;
	Graph			*graph;
	int16			i;

	gen = st_init_gen(graphList);
	while (st_gen(gen,&name,(char **) &graph) != 0) {
		for (i=0;i<graph->curNumberOfPlots;i++) {
			sim_plot(graph->inst[i]);
		} /* for... */
	} /* while... */
	st_free_gen(gen);
} /* UpdatePlot... */
