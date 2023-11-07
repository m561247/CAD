/***********************************************************************
 * Musa global variable declarations and typedefs.
 * Filename: musa.h
 * Program: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Design: Russell Segal
 * Company:  University Of California, Berkeley
 *           Electronic Research Laboratory
 *           Berkeley, CA 94720
 * Modifications:
 */

/************************************************************************
 * Include files
 */
#include "copyright.h"
#include "port.h"
#include "errtrap.h"
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "x10-x11.h"
#include "xg.h"
#ifdef RPC_MUSA
#include "rpc.h"
#include "region.h"
#endif
#include "utility.h"
#ifndef NO_OCT
#include "oct.h"
#include "oh.h"
#endif
#include "tr.h"
#include "st.h"
#include "list.h"

#define TRUE	1
#define FALSE	0

#define upper(c)				(islower(c) ? toupper(c) : (c))

/************************************************************************
 * musaInstance is an instance that is an internal node in hierarchy
 * Definitions for structures and macros of "musa"
 *
 * node is a conceptual net that spans the OCT hierarchy
 * It contains information concerning state.
 *
 * An element is a net or terminal in the hierarchy.  It points to a node
 *
 * musaInstance is an instance that is an internal node in hierarchy
 *
 * connect holds data for a connection between nodes and musaInstances
 */

/*** Types Of Connection ***/
#define TERM_OUTPUT	0x1
#define TERM_VSI	0x2			/* voltage sensitive input */
#define TERM_DSI	0x4			/* drive sensitive input */
#define TERM_DLSIO	0x8			/* drive LEVEL sensitive i/output
						 * DLSIO is for transistors only --
						 * it may not be used in user instances */
/*** Drive levels ***/
#define DRAIL	0
#define DRIVE	1
#define WEAK	2
#define CHARGED	3
#define NOTHING	4

/*** Masks for the node data field ***/
#define IMPORTANT_NODE		0x40	/* node should not be removed */
#define INEFF_NODE		0x20	/* node which is inefficient to merge */
#define NO_SERIES		0x60	/* node will not be series merged */

#define COUNT_WRAPPED	0x20	/* count incremented passed 0 */
#define CYCLE_REPORTED	0x10	/* count incremented passed 0 twice */

/*** Composite node levels, 'data' field ***/
#define L_VOLTAGEMASK	        0x03	/* mask for 1, 0, or X */
#define	L_UNKNOWN		0x03
#define	L_HIGH			0x02
#define L_LOW			0x01

#define L_dot			0x0F	/* place holder, not simulatable */
#define L_I			0x0E	/* 1 input */
#define L_O			0x0D	/* 0 input */

#define L_X			0x0B	/* normal X */
#define L_wX			0x07	/* weak X */
#define L_x			0x03	/* charged X */

#define L_1			0x0A	/* normal 1 */
#define L_w1			0x06	/* weak 1 */
#define L_i			0x02	/* charged 1 */

#define L_0			0x09	/* normal 0 */
#define L_w0			0x05	/* weak 0 */
#define L_o			0x01	/* charged 0 */

#define L_ill			0		/* illegal */

/* Masks for the instance data field (initialized to 0) */
/* All but the top 3 of 16 bits may be re-assigned for user devices */
/* for all instances */
/* for any leaf instance */
#define SKIP_SCHED3		0x2000	/* ignore device when it comes up */
#define IS_SCHED3		0x4000  /* Flag that instance is being simulated as sched3. */
/* for mos and pull devices */
#define SUBTYPE			0x3f	/* type of network */
#define IS_ON			0x80	/* device is active */

/*** Flags for data field in 'element_struct' ***/
#define EQUIV_PTR	0x8	/* c is equiv (implied by sim format) */
#define PARENT_PTR	0x4	/* use pointer to to parent */
#define EPARENT_PTR	0x2	/* equiv tree field points to parent */
#define NODE_PTR	0x1	/* equiv tree field points to node */

/* macros for table lookup */
#define LETTER(node) letter_table[(node)->voltage]

#define LEVEL(node) combo_table[ \
	drive_table[0xff & (node)->highStrength] << 3 | \
	drive_table[0xff & (node)->lowStrength]]

#define HIGH(node)    ((node)->voltage & 0x2)
#define LOW(node)     ((node)->voltage & 0x1)
#define UNKNOWN(node) (((node)->voltage & 0x3) == 0x3)

/* #define CNUM(x) (x & 0xff) */	/* safely map a char into an int */


/*** Masks for return token from musaInteractiveLoop(), 'sim_status' ***/
#define SIM_OK		0x00	/* no errors */
#define SIM_UNKNOWN	0x01	/* Unknown error */
#define SIM_USAGE	0x02	/* Unknown command or syntax error */
#define SIM_CYCLE	0x04	/* A cycle was found in evaluation */
#define SIM_BREAK	0x08	/* A break point was encountered */
#define SIM_INTERRUPT	0x10	/* Evaluation was interrupted */
#define SIM_NOTFOUND	0x20	/* element or vector was not found */
#define SIM_VERIFY	0x40	/* verify failed */
#define SIM_COMMANDBRK	0x80	/* break long listing command -- not
				 * of source or macro */

#define SIM_ANYERROR	0x07	/* a hard error */
#define SIM_ANYBREAK	0x3f	/* a reason to break out of a macro */
#define SIM_CMDINTERR	0x90	/* reason to stop a command */


/*************************************************************************
 * 
 */
#define NMOS 1
#define PMOS 0
/*************************************************************************
 * Maximum number of nested 'source' commands.
 */
#define SOURCE_DEPTH	20

/**************************************************************************
 * The order in this list MUST match the 
 * order of the deviceTable[] array defined in 'device.c'
 */
typedef enum { 
	DEVTYPE_ILLEGAL = -1,		/* -1 */
	DEVTYPE_MODULE,			/* 0 */
	DEVTYPE_NMOS ,			/* 1 */
	DEVTYPE_WEAK_NMOS,
	DEVTYPE_PMOS,			/* 3 */
	DEVTYPE_WEAK_PMOS,	 	/* 4 */
	DEVTYPE_N_PULLDOWN, 		/* 5 */
	DEVTYPE_WEAK_N_PULLDOWN,
	DEVTYPE_P_PULLDOWN,
	DEVTYPE_WEAK_P_PULLDOWN,
	DEVTYPE_N_PULLUP,
	DEVTYPE_WEAK_N_PULLUP,		/* 10 */
	DEVTYPE_P_PULLUP,
	DEVTYPE_WEAK_P_PULLUP,
	DEVTYPE_LOGIC_GATE, 		/* 13 */
	DEVTYPE_LATCH, 	
	DEVTYPE_TRISTATE_BUFFER, 
	DEVTYPE_PASS_GATE,
	DEVTYPE_RAM,			
	DEVTYPE_LED,			
	DEVTYPE_PLOT_PROBE,
	DEVTYPE_7SEGMENT,
	DEVTYPE_END_OF_MODELS
} deviceType;

#define MosInstance(type)			((int) type <= (int) DEVTYPE_WEAK_P_PULLUP)
#define MosTransistor(type)			((int) type <= (int) DEVTYPE_WEAK_PMOS)
#define FindPullDown(type)			(deviceType) ((int) type + 4)
#define FindPullUp(type)			(deviceType) ((int) type + 8)
#define BasicTransistorType(type)	(deviceType) ((int) type & 0x3)

typedef	struct	node_struct		node;
typedef	struct	element_struct		MusaElement;
typedef	struct	musaInstance_struct	musaInstance;
typedef	struct	connect_struct		connect;
typedef	MusaElement			*elemptr;
typedef	musaInstance			*musaInstanceptr;
typedef	node				*nodeptr;




struct node_struct {					/* Node Structure */
        int schedule;
	/* In common with scheduleItem */

	MusaElement *rep;				/* element that points here */
	connect *conns;					/* linked list of connections */
	short numberOfHighDrivers;                      /*  */
	short numberOfLowDrivers;			/* number of drivers */
	unsigned int highStrength : 8;			 /* level in directed graphs */
	unsigned int lowStrength : 8;			 /* level in directed graphs */
	unsigned int is_supply : 1;
	unsigned int is_ground : 1;
	unsigned int voltage : 8; 
	unsigned int count : 8;				/* running count of transistions */
	int data;					/* data about node (see above) */
}; /* node_struct... */

struct element_struct {
    char	*name;
    union {
	/********************************************************* 
	 *	1) Pointer to the next sibling element OR
	 *	2) For the last sibling back to the parent instance
	 */
	MusaElement	*sibling;				/* pointer to next sibling */
	musaInstance	*parentInstance;			/* pointer to parent (data flag = PARENT_PTR) */
    } u;
    union {
	/***********************************************************
	 *	1) Pointer to the next equivalent sibling OR
	 *	2) For the last sibling back to the equiv parent element OR
	 *	3) For the root element to the node structure 
	 */
	MusaElement	*sibling;			/* pointer to next equivalent sibling element */
	MusaElement	*parent;			/* equivalent parent element */
	node	*node;					/* pointer to the node (data flag = NODE_PTR) */
    } equiv;
    union {
	/**********************************************************
	 *	1) For OCT format, pointer to equiv child element OR
	 *	2) For sim format, pointer to equiv element
	 */
	MusaElement	*child;					/* equivalent child element */
	MusaElement	*equiv;					/* list of aliased names (sim format) */
    } c;
    int		data;					/* which fields of union? */
};							/* element_struct... */

struct musaInstance_struct {
    int		        schedule;            /*  */

    /* */
    char		*name;
    deviceType		type;		     /* type of instance */
    int			data;		     /* data about inst (see above) */
    musaInstance*	parent;	     	     /* Parent of instance */
    char		*userData;	     /* pointer to data for user device */
    int			terms;		     /* number of incident nets */
    connect		*connArray;	     /* array of incident nets (in order) */
    musaInstance	*child;		     /* sub-musaInstances of internal (type == 0) */
    st_table*       hashed_children;	     /* hashed sub-musaInstances */
    st_table*      hashed_elements;	     /* hashed sub-elements of internal */
    MusaElement		*elems;		     /* sub-elements of internal */
}; /* musaInstance_struct... */

struct connect_struct {
    node	 *node;					/* the node where this connects */
    musaInstance *device;				/* the device where this connects */
    connect	 *next;					/* next connect on same node */
    char	 ttype;					/* type of connection */
    unsigned char	highStrength;			/* level of high drive */
    unsigned char	lowStrength;			/* level of low drive */
    unsigned		hout : 1;			/* true if high is output */
    unsigned		lout : 1;			/* true if low is output */
}; /* connect_struct... */

/*** Structure to hold data about device types ***/
struct device_struct {
    char	*name;
    int		expandable;				/* connection which may repeat */
    struct term_struct *term;				/*  */
    int		(*sim_routine)();			/* routine to simulate device */
    int16	param, param2, param3;			/* parameters to simulation routines */
    int		(*info_routine)();			/* routine to give info about device */
    int		(*startup_routine)();			/* called before first simulation */
    int		(*restart_routine)();			/* called after data read */
    int		(*save_routine)();			/* routine to save instance data */
    int		(*load_routine)();			/* load back instance data */
}; /* device_struct... */

struct term_struct {
    char	*name;
    char	ttype;
}; /* term_struct... */

/*** Info about transistor networks ***/
struct network_struct {
    char	*name;					/*  */
    long	active;					/* lookup for network activity */
    int		numin;					/* number of inputs */
    int		paradd[3];				/* how to add networks in parallel */
    int		seradd[3];				/* how to add networks in series */
}; /* network_struct... */


/*** Linked list of nodes -- high bit first ***/
typedef struct vect_struct			vect;
struct vect_struct {
    int		is_blank;				/* space holder -- no data */
    MusaElement	*data;
    vect	*next, *prev;
}; /* vect_struct... */

/*** Vector of nodes for user interface ***/
typedef struct vector_struct		vector;
struct vector_struct {
    char	*name;					/* the vector's name (hashed by this) */
    vect	*highbit, *lowbit;		/* the actual nodes */
}; /* vector_struct... */

/*** List of vectors and element names ***/
typedef struct watchl_struct		watchl;
struct watchl_struct {
    char	*name;
    watchl	*next;
    int		mode;					/* 0 => binary, 1 => hex */
}; /* watchl_struct... */

/*** List of vectors and elements for a watch list ***/
typedef struct watchlist_struct		watchlist;
struct watchlist_struct {
    char	*name;
    watchl	*first;
}; /* watchlist_struct... */

#ifdef RPC_MUSA
typedef struct {
	RPCSpot		*spot;
	vemSelSet	*selSet;
} vemInfo;
#endif

/*********************************************************************
 * Externals for getopt
 */
extern int optind;
extern char *optarg;

/*********************************************************************
 * RegEx Routines
 */
extern char	*re_comp();
extern int	re_exec();

#ifndef ALLOC_MUSA
extern	void		InitLists();
extern	void		InitAllocs();
extern	musaInstance		*MusaInstanceAlloc();
extern	void		MusaInstanceFree();
extern	void		ForEachMusaInstance();
extern	node		*NodeAlloc();
extern	void		NodeFree();
extern	void		ForEachNode();
extern	MusaElement	*ElemAlloc();		/*  */
extern	void		ElemFree();
extern	char		*IStringAlloc();
extern	char		*EStringAlloc();
extern	connect		*ConnAlloc();
extern	void		ConnFree();
#endif

#ifndef DEVICE_MUSA
extern	struct device_struct	deviceTable[];
#endif

#ifndef ERRORS_MUSA
extern	void		FatalError();
extern	void		FatalOctError();
extern	void		Warning();
#ifdef RPC_MUSA
extern	void		Format();
#else
extern	void		Usage();
#endif
extern	void		VerboseWarning();
extern	void		InternalError();
extern	void		Message();
extern	void		VerboseMessage();
#endif

#ifndef GN_MUSA
extern	node		*ElementToNode();
extern	MusaElement		*NextEquiv();
extern	void		gnEquivElems();
extern	void		MakeElemPath();
extern	void		MakeConnPath();
extern	void		MakePath();
extern	void		gnPwd();
extern	void		gnSetDir();
extern	void		gnListNets();
extern	void		gnListInsts();
extern	int16		gnGetElem();
extern	int16		gnInstPath();
extern	int16		gnCInst();
extern	int16		LookupVect();
extern	void		ListLogiX();
extern	void		DumpList();
extern	void		ShowValue();
extern	void		Watch();
extern	void		Verify();
extern	void		SetVect();
extern	void		MakeVect();
#ifdef RPC_MUSA
extern	int			rpcMusaListElems();
extern	int			rpcMusaListInsts();
extern	int			rpcMusaListLogiX();
extern	int			rpcMusaEquivElems();
extern	int			rpcMusaPrintInst();
extern	int			rpcMusaMakeVector();
extern	int			rpcMusaSet();
extern	int			rpcMusaShow();
extern	int			rpcMusaWatch();
extern	int			rpcMusaVerify();
extern	int			rpcMusaChangeInst();
#endif
#endif

#ifndef TABLES_MUSA
extern char drive_table[];
extern char letter_table[];
extern char combo_table[];
extern struct network_struct network[];
#endif

#ifndef LOGIC_MUSA
#ifndef NO_OCT
extern int		logic();
extern int		logic_info();
#endif
#endif

#ifndef SEG_MUSA
#ifndef NO_OCT
extern int16	seg_read();
extern int		seg_info();
extern int		sim_seg();
#endif
#endif

#ifndef LATCH_MUSA
#ifndef NO_OCT
extern int16	tristate_read();
extern int		sim_tristate();
extern int		tristate_info();
#endif
#endif

#ifndef NEWLATCH_MUSA
#ifndef NO_OCT
extern int16	latch_read();
extern int		sim_latch();
extern int		latch_info();
#endif
#endif

#ifndef RAM_MUSA
#ifndef NO_OCT
extern int16	ram_read();
extern int		sim_ram();
extern int		ram_info();
#endif
#endif

#ifndef LED_MUSA
#ifndef NO_OCT
extern int16	led_read();
extern int		sim_led();
extern int		led_info();
#endif
#endif

#ifndef MEMORY_MUSA
#ifndef NO_OCT
extern int16	memory_read();
#endif
#endif

#ifndef READOCT_MUSA
#ifndef NO_OCT
extern void		get_oct_elem();
extern void		read_oct();
extern int16	mos_read();
extern int16	logic_read();
#endif
#endif

#ifndef READSIM_MUSA
extern void		init_node();
extern void		make_musaInstance();
extern void		equiv_elem();
extern void		make_conns();
extern void		make_element();
#ifndef RPC_MUSA
extern void		read_sim();
#endif
#endif

#ifndef VERSION_MUSA
extern char		*version();
#endif


enum com_keys {
    /* index around hierarchy */
    COM_cinst,
    COM_listelems,
    COM_equivelems,
    COM_listinsts,
    COM_pwd,
	COM_listlogix,

    /* user vector stuff */
    COM_makevect,
    COM_setvect,
    COM_verivect,
    COM_show,
    COM_watch,

    /* go do simulation type stuff */
    COM_evaluate,
    COM_continue,
    COM_step,
    COM_setbreak,

    /* crawl around the data structure */
    COM_nodeinfo,
    COM_iinfo,
    COM_backtrace,

    /* interactive graphics */
    COM_setplot,
    COM_plot, 
    COM_saveplot,
    COM_destroyplot,

    /* state saving and recall */
    COM_savestate,
    COM_loadstate,
	COM_initstate,

    COM_macro,
    COM_source,

    COM_lopen,
    COM_lclose,
    COM_sleep,			/* Insert delays in simulation. */
    COM_repeat,
    COM_separator,
    COM_help,

    COM_unknown
}; /* com_keys... */

typedef struct {
    char *name;
    char *shortname;
    enum com_keys command;
    char *description;
} COMTABLE;

#ifdef INTER_MUSA

COMTABLE com_table[] = {
    "backtrace","bt",COM_backtrace,"List many levels of fanin to a node",
    "changeinst","ci",COM_cinst,"Change the working instance",
    "continue","co",COM_continue,"Continue processing",
    "destroyplot","dpl",COM_destroyplot,"Destroy a graph",
    "equivalent","eq",COM_equivelems,"List the elements that are equivalent",
    "evaluate","ev",COM_evaluate,"Propagate all changes through the network",
    "help","?",COM_help,"Print out this help message",
         "initstate","is",COM_initstate,"Reinitialize all nodes",
    "instinfo", "ii", COM_iinfo, "Give detailed information about instances",
    "lclose", "lc", COM_lclose, "Close the current log file", 
    "listelems","le",COM_listelems,"List the elements contained by an instance",
    "listinsts","li",COM_listinsts,"List the instances contained by an instance",
	"listlogix","lx",COM_listlogix,"List the elements with logic level = X or x",
    "loadstate", "ls", COM_loadstate, "Retrieve a past state of the simulation",
    "lopen", "lo", COM_lopen, "Open a log file", 
    "macro", "ma", COM_macro, "Define a macro",
    "makevector","mv",COM_makevect,"Create a new vector",
    "nodeinfo", "ni", COM_nodeinfo, "Give detailed information about nodes",
    "plot","pl",COM_plot,"Plot a node on a graph",
    "printinst","pi",COM_pwd,"Print the name of the current working instance",
    "repeat","rp",COM_repeat,"Repeat a command a fixed number of times",
    "saveplot","svpl",COM_saveplot,"Save a graph",
    "savestate","ss",COM_savestate,"Save the current state of the simulation",
    "separator","sp",COM_separator,"Set the value of the separator",
    "set","se",COM_setvect,"Set the value of a vector, element, or constant",

    "setbreak","sb",COM_setbreak,"Set a breakpoint in the evaluation",
    "setplot","spl",COM_setplot,"Set up a graph for plotting",
    
    "show", "sh", COM_show, "Show the value of a vector, element, or constant",
    "sleep","sl",COM_sleep,"Sleep a fixed number of milliseconds",
    "source","sr",COM_source,"Execute musa commands from a file",
    "step","st",COM_step,"Continue one step in source file",
    "verify","ve",COM_verivect,"Verify the value of a vector or single element",
    "watch","wa",COM_watch,"Add vector or element to a watched list",
     0,0,COM_unknown,"unknown command"
};

#else

extern	void			InterInit();
extern	void			DisplaySourceStatus();
extern	void			my_print();
extern	COMTABLE		com_table[];
#ifdef RPC_MUSA
extern	int				rpcMusaSource();
extern	int				rpcMusaLOpen();
extern	int				rpcMusaLClose();
#endif

#endif

#ifndef INFO_MUSA
extern  void 			singleConnectInfo();
extern	void			NodeInfo();
extern	void			InstInfo();
extern	void			Backtrace();
extern	void			ConnectInfo();
extern	int				mos_info();
#ifdef RPC_MUSA
extern	int				rpcMusaNodeInfo();
extern	int				rpcMusaInstInfo();
extern	int				rpcMusaBacktrace();
#endif
#endif

#ifndef STATE_MUSA
extern	void		SaveState();
extern	void		LoadState();
extern	void		InitState();
#ifdef RPC_MUSA
extern	int			rpcMusaInitState();
extern	int			rpcMusaSaveState();
extern	int			rpcMusaLoadState();
#endif
#endif

#ifndef MY_MUSA
extern	int16		StrPropEql();
#endif

#ifndef PANEL_MUSA
extern 	int16		StartPanel();
extern	int16		MusaOpenXDisplay();
extern	void		RPCHandleXEvents();
extern	void		HandleXEvents();
extern	PIXEL		GetColor();
#endif

#ifndef PLOT_MUSA
extern	void		NextMacroTime();
extern	void		plot();
extern	int			sim_plot();
extern	int			plot_info();
extern	void		UpdatePlot();
extern	void		SetPlot();
extern	void		Plot();
extern	void		SavePlot();
extern	void		DestroyPlot();
#ifdef RPC_MUSA
extern	int			rpcMusaDestroyPlot();
extern	int			rpcMusaSavePlot();
extern	int			rpcMusaSetPlot();
extern	int			rpcMusaPlot();
#endif
#endif

#ifndef VDD_MUSA
extern	int16		RecognizePowerNode();
extern	void		DeletePowerNode();
extern	void		InitPowerNodes();
extern	void		init_all_power_nodes();
extern	int16		IsGround();
extern	int16		IsSupply();
extern	void		ConnectPowerNodes();
#endif

#ifndef TRANS_MUSA
extern	int			mos();
extern	int			wmos();
extern	int			pull();
extern	int			mos_restart();
extern	int			pull_restart();
#endif

#ifndef MERGE_MUSA
extern	void		merge_trans();
extern	void		remove_connect();
#endif

#ifndef PROP_MUSA
extern	void		sched_start();
extern	void		init_sched();
extern	void		inst_driveSensitiveSchedule();
extern	int			inst_normalSchedule();
extern	void		musaDriveHigh();
extern	void		musaUndriveHigh();
extern	void		musaDriveLow();
extern	void		musaUndriveLow();
extern	void		musaRailDrive();
extern	void		musaRailUndrive();
extern	int16		events_pending();
extern	void		dump_events();
extern	void		Evaluate();
extern	void		SetBreak();
#ifdef RPC_MUSA
extern	int			rpcMusaEvaluate();
#endif
#endif

#ifndef RPC_MUSA
#ifndef YYLEX_MUSA
extern	char		*yylex();
#endif
#endif

extern musaInstance*   getParentInstance();
extern MusaElement*   getMainSiblingElement();

extern musaInstance*  getParentInstance( );
#ifndef HASH_MUSA
extern	int16		InstLookup();
extern	void		InstInsert();
extern	void		InstUnlink();
extern	int16		ElemLookup();
extern	void		ElemInsert();
extern	void		ElemUnlink();
#endif

#ifndef IROUTINES_MUSA
extern	void		drive_output();
extern	void		weakdrive_output();
extern	void		undrive_output();
extern	void		make_leaf_inst();
extern	void		connect_term();
#endif

#ifdef RPC_MUSA
extern void			rpcMusaUpdateSelSet();
extern void			rpcMusaUpdateFacetSelSet();
extern void			rpcMusaRunRoutine();
extern void			rpcMusaPrint();
extern int16		rpcMusaUnderSpot();
extern vemSelSet	*rpcMusaSetSelSet();
extern int			rpcMusaTogDM();
extern int			rpcMusaHelp();
extern int			rpcMusaExit();
extern int			rpcMusaShowLevels();
extern int			rpcMusaUnShowLevels();
#endif

typedef struct {
	char	*description;
	char	*format;
} COMINFO;

#ifdef MAIN_MUSA
/* GLOBAL VARIABLES */
char		*progName;
char		errmsg[1024];			/* error message buffer */
char		msgbuf[1024];			/* message buffer */
Display		*theDisp = NIL(Display);/* x-window display */
char		*displayName;
int			tcount = 0;				/* transistor count */
#ifdef RPC_MUSA
XContext	dirTable;
st_table	*winTable;
char		*dmMessage;				/* message buffer for vem dialog manager */
int16		selSetFlag;
int16		dmFlag;
COMINFO		rpcMusaBacktraceHelp =
	{ "List many levels of fanin to node.",
		"[object(s)][text] [\"levels\"]" };
COMINFO		rpcMusaChangeInstHelp =
	{ "Change the working instance.",
		"" };
COMINFO		rpcMusaDestroyPlotHelp =
	{ "Destroy a graph.",
		"[\"graph name\"]" };
COMINFO		rpcMusaEquivElemsHelp =
	{ "List the elements that are equivalent.",
		"[object(s)][text]" };
COMINFO		rpcMusaEvaluateHelp =
	{ "Propagate all changes through the network.",
		"" };
COMINFO		rpcMusaExitHelp =
	{ "Exit program.",
		"" };
COMINFO		rpcMusaHelpHelp =
	{ "Print out this help message.",
		"" };
COMINFO		rpcMusaInitStateHelp =
	{ "Reinitialize all nodes.",
		"" };
COMINFO		rpcMusaInstInfoHelp = 
	{ "Give detailed information about instances.",
		"[object(s)][text]" };
COMINFO		rpcMusaLCloseHelp =
	{ "Close the current log file.",
		"" };
COMINFO		rpcMusaListElemsHelp =
	{ "List the elements contained by an instance.",
		"[text]" };
COMINFO		rpcMusaListInstsHelp =
	{ "List the instances contained by an instance.",
		"[text]" };
COMINFO		rpcMusaListLogiXHelp =
	{ "List the elements with logic level = X or x.",
		"" };
COMINFO		rpcMusaLoadStateHelp =
	{ "Retrieve a past state of the simulation.",
		"[\"file name\"]" };
COMINFO		rpcMusaLOpenHelp =
	{ "Open a log file.",
		"[\"file name\"]" };
COMINFO		rpcMusaMakeVectorHelp =
	{ "Create a new vector.",
		"[object(s)][text] \"vector name\"" };
COMINFO		rpcMusaNodeInfoHelp =
	{ "Give detailed information about nodes.",
		"[object(s)][text]" };
COMINFO		rpcMusaPlotHelp =
	{ "Plot an element or vector on a graph.",
		"[object(s)][text] \"graph name\"" };
COMINFO		rpcMusaPrintInstHelp =
	{ "Print the name of the current working instance.",
		"" };
COMINFO		rpcMusaSavePlotHelp =
	{ "Save a graph",
		"[\"graph name\" [\"file name\"]]" };
COMINFO		rpcMusaSaveStateHelp =
	{ "Save the current state of the simulation.",
		"[\"file name\"]" };
COMINFO		rpcMusaSetHelp =
	{ "Set the value of a vector, element, or constant.",
		"[object(s)][text] \"value\"" };
COMINFO		rpcMusaSetPlotHelp =
	{ "Set up a graph for plotting.",
		"[\"graph name\" [\"width\" \"height\" [\"x\" \"y\"]]]" };
COMINFO		rpcMusaShowHelp =
	{ "Show the value of a vector, element, or constant",
		"[object(s)][text] [\"format string\"]" };
COMINFO		rpcMusaShowLevelsHelp =
	{ "Color nets in vem window to correspond with\ncurrent driven level of element.",
		"" };
COMINFO		rpcMusaSourceHelp =
	{ "Execute musa commands from a file",
		"[\"file name\"]" };
COMINFO		rpcMusaTogDMHelp =
	{ "Toggle whether information is put in\nvem console window or dialog boxes.",
		"" };
COMINFO		rpcMusaUnShowLevelsHelp =
	{ "Turn off coloring of nets.",
		"" };
COMINFO		rpcMusaVerifyHelp =
	{ "Verify the value of a vector or single element.",	
		"[object(s)][text] \"value\"" };
COMINFO		rpcMusaWatchHelp =
	{ "Add vector or element to a watched list.",
		"[object(s)][text] \"watch list name\"" };

/************************************************************************
 * COMMANDARRAY (RPCFUNCTION)
 */
RPCFunction CommandArray[] = {
	{ rpcMusaInitState,		"General",	"musa-initstate",		"Q",	
		(long) &rpcMusaInitStateHelp},
	{ rpcMusaSaveState,		"General",	"musa-savestate",		"{",
		(long) &rpcMusaSaveStateHelp},
	{ rpcMusaLoadState,		"General",	"musa-loadstate",		"}",
		(long) &rpcMusaLoadStateHelp},
	{ rpcMusaSource,		"General",	"musa-source",			"a",
		(long) &rpcMusaSourceHelp},
	{ rpcMusaLOpen,			"General",	"musa-lopen",			"[",
		(long) &rpcMusaLOpenHelp},
	{ rpcMusaLClose,		"General",	"musa-lclose",			"]",
		(long) &rpcMusaLCloseHelp},
	{ rpcMusaTogDM,			"General",	"musa-togdm",			"v",
		(long) &rpcMusaTogDMHelp},
	{ rpcMusaHelp,			"General",	"musa-help",			"h",
		(long) &rpcMusaHelpHelp},
	{ rpcMusaExit,			"General",	"musa-exit",			"X",
		(long) &rpcMusaExitHelp},
	{ rpcMusaChangeInst,	"Examine",	"musa-changeinst",		"C",
		(long) &rpcMusaChangeInstHelp},
	{ rpcMusaListElems,		"Examine",	"musa-listelems",		"J",
		(long) &rpcMusaListElemsHelp},
	{ rpcMusaListInsts,		"Examine",	"musa-listinsts",		"K",
		(long) &rpcMusaListInstsHelp},
	{ rpcMusaEquivElems,	"Examine",	"musa-equivalent",		"E",
		(long) &rpcMusaEquivElemsHelp},
	{ rpcMusaPrintInst,		"Examine",	"musa-printinst",		"",
		(long) &rpcMusaPrintInstHelp},
	{ rpcMusaShowLevels,	"Show",		"musa-showlevels",		"Y",
		(long) &rpcMusaShowLevelsHelp},
	{ rpcMusaUnShowLevels,	"Show",		"musa-unshowlevels",	"U",
		(long) &rpcMusaUnShowLevelsHelp},
	{ rpcMusaSetPlot,		"Plot",		"musa-setplot",			"G",
		(long) &rpcMusaSetPlotHelp},
	{ rpcMusaPlot,			"Plot",		"musa-plot",			"g",
		(long) &rpcMusaPlotHelp},
	{ rpcMusaSavePlot,		"Plot",		"musa-saveplot",		"",
		(long) &rpcMusaSavePlotHelp},
	{ rpcMusaDestroyPlot,	"Plot",		"musa-destroyplot",		"",
		(long) &rpcMusaDestroyPlotHelp},
	{ rpcMusaNodeInfo,		"Info",		"musa-nodeinfo",		"j",
		(long) &rpcMusaNodeInfoHelp},
	{ rpcMusaInstInfo,		"Info",		"musa-instinfo",		"k",
		(long) &rpcMusaInstInfoHelp},
	{ rpcMusaBacktrace,		"Info",		"musa-backtrace",		"H",
		(long) &rpcMusaBacktraceHelp},
	{ rpcMusaListLogiX,		"Info",		"musa-listlogix",		"q",
		(long) &rpcMusaListLogiXHelp},
	{ rpcMusaMakeVector,	"Vector",	"musa-makevector",		"",
		(long) &rpcMusaMakeVectorHelp},
	{ rpcMusaSet,			"Vector",	"musa-set",				"",
		(long) &rpcMusaSetHelp},
	{ rpcMusaVerify,		"Vector",	"musa-verify",			"V",
		(long) &rpcMusaVerifyHelp},
	{ rpcMusaShow,			"Vector",	"musa-show",			"",
		(long) &rpcMusaShowHelp},
	{ rpcMusaWatch,			"Vector",	"musa-watch",			"",
		(long) &rpcMusaWatchHelp},
	{ rpcMusaEvaluate,		"Simulate",	"musa-evaluate",		"",
		(long) &rpcMusaEvaluateHelp},
};

#endif
int16		verbose = FALSE;		/* verbose flag */
int16		number_of_nodes;

double		weak_thresh = 1.0;		/* W/L where transistors are weak */
int16		no_pulls = TRUE;		/* no pulldown and pullup */
int16		no_merge = TRUE;		/* don't merge transistors */
int16		update = TRUE;			/* give readin progress */
#ifndef RPC_MUSA
FILE		*input_file = stdin;
#endif
FILE		*output_file = NIL(FILE);
char		separator = '/';		/* character used for hierarchy */
musaInstance		*topmusaInstance;				/* musaInstance at root of hierarchy */
musaInstance		*cwd;					/* current working instance */

#ifdef NO_OCT
int16		sim_format = TRUE;			/* input is .sim */
#else
int16		sim_format = FALSE;			/* input is not .sim */
#endif
int			sim_status;

/* inter.c */
int16		single_step;    /* take a single step in source */

/* panel.c */
Window 		panelWindow;
PIXEL  		bgPanelPixel;
PIXEL		fgPanelPixel;
PIXEL		bdPanelPixel;
lsList		ledList;
lsList		segList;

GC			gc;
Pixmap		tile;

/* readoct.c */
musaInstance		*parent_inst;
octObject	*inter_facet;

/* gn.c */
st_table	*vectors,*watchlists,*constants;

/* declaraion of routines to read from Oct (in order) */
int16 (*read_routine[])() = {
    mos_read, tristate_read, latch_read ,logic_read, led_read, ram_read,
    seg_read
      /****************************************
       *  Add new Oct reading routines here   *
       ****************************************/
  };
int16 number_of_reads = sizeof(read_routine) / sizeof(int16 (*)());

/* plot.c */
st_table	*graphList;

/* prop.c */
int16		disable_sched2 = FALSE;

#else

extern char			*progName;
extern char			errmsg[];		/* buffer for errors and messages */
extern char			msgbuf[];		/* message buffer */
extern Display			*theDisp;		/* x-window display */
extern char			*displayName;
extern int			tcount;			/* transistor count */
#ifdef RPC_MUSA

extern XContext		dirTable;
extern RPCFunction	CommandArray[];

extern st_table		*winTable;
extern char		*dmMessage;		/* message buffer for vem dialog manager */
extern int16		selSetFlag;
extern int16		dmFlag;
#endif
extern int16		verbose;		/* verbose flag */
extern int16		number_of_nodes;

extern double	weak_thresh;	/* W/L where transistors are weak */
extern int16	no_pulls;		/* no pulldown and pullup */
extern int16	no_merge;		/* don't merge transistors */
extern int16	update;			/* give readin progress */
#ifndef RPC_MUSA
extern FILE		*input_file;
#endif
extern FILE		*output_file;
extern char		separator;	/* character used for hierarchy */

extern musaInstance	*topmusaInstance;			/* musaInstance at root of hierarchy */
extern musaInstance	*cwd;					/* current working instance */

extern int16		sim_format;
extern int		sim_status;
/* inter.c */
extern int16		single_step;    /* take a single step in source */

/* panel.c */
extern Window	panelWindow;
extern PIXEL	bgPanelPixel;
extern PIXEL	fgPanelPixel;
extern PIXEL	bdPanelPixel;
extern lsList	ledList;
extern lsList	segList;

extern	GC		gc;
extern	Pixmap		tile;

/* readoct.c */
extern	musaInstance	*parent_inst;
extern	octObject	*inter_facet;

extern st_table	*vectors, *watchlists, *constants;

/* declaraion of routines to read from Oct (in order) */
extern int16	(*read_routine[])();
extern int16	number_of_reads;

extern st_table	*graphList;

extern int16	disable_sched2;
#endif

/*************************************************************************
 * MACROS
 */
#ifdef RPC_MUSA
#define MUSAPRINTINIT(buf)		dmMessage = util_strsav(buf)
#define MUSAPRINT(buf)			dmMessage = \
	strcat(REALLOC(char,dmMessage,strlen(dmMessage)+strlen(buf)+1),buf) 
#else
#define MUSAPRINTINIT(buf)		my_print(buf);
#define MUSAPRINT(buf)			my_print(buf);
#endif
