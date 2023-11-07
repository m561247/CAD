/*   STEPS - Schematic Text Entry Package utilizing Schematics
 * 
 *   Denis S. Yip
 *   U.C. Berkeley
 *   1990
 */

#include "port.h"
#include "utility.h"
#include "list.h"
#include "rpc.h"
#include "oh.h"
#include "region.h"
#include "st.h"
#include "errtrap.h"
#include "table.h"
#include "defs.h"

int isRPC = 1;

extern int strset();
extern double atofe();
vemSelSet vem_selected_set;
int highlight_flag;

static char *progName = "STEPS";

char cur_deck[100];
char pro1_str[100];
char pro2_str[100];
char pro3_str[100];
char pro4_str[100];
char pro5_str[100];
char cur_deck_wo[100];
static int anawe();
int dc_ok;
int ac_ok;
int tran_ok;
int disto_ok;
int noise_ok;


FILE *fp;
FILE *fp1;
static	octObject	topFacet, groundNet;

char *gensym();
char *genmod();

st_table *modeltable, *subckttable;
lsList	 modList, subList;
char display_ptr[40];



/*ARGSUSED*/
extern int quit(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
/*
 * Terminates the application.
 */
{
    deHighlight(spotPtr, cmdList, userOptionWord);
    RPCExit(0);
}

/*static int chooseGround(), instanceProps();*/
extern int plot(), quit(), initConds(), spiOptions();
extern int af0(), af1(), af2(), af3(), af4(), af5(), af6(), af7();
extern int changeGlobalPars(), ana2(), ana3(), anan();
extern int printSpiceDeck(), transDeck(), addSource(), partition();
extern int modelValueOn(), modelValueOff();
extern int nodeValueOff(), nodeValueOn(), allValueOff(), allValueOn();
extern int modelNameOff(), modelNameOn();
extern int spiceDevices();
extern int userHelp(),  spiWidth();
extern int spiOp(), spiAc(), spiTf(), spiSens(), spiDisto();
extern int spiPlot();
extern int nodeSet(), bug(), printout(), deHighlight();
extern int modelEdit();



stepsRefresh( spotPtr )
    RPCSpot *spotPtr;
{
    octObject facet;

    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));

    processCell(&facet);
    stepsRpcFinish( spotPtr);
}

stepsRpcFinish( spotPtr)
    RPCSpot *spotPtr;
{
    lsList cmdList = lsCreate();
    if (vemCommand("show-all", spotPtr, (lsList)cmdList, 0) < VEM_OK) {
	stepsEcho( "Can't execute 'show-all'\n");
    }
    vemPrompt();
}



extern int editSpiceOptions(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject facet, bag;

    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));
    ohGetOrCreateBag( &facet, &bag, "STEPS" );

    stepsMultiText( &bag, "SPICE options", optionsTable );

    return RPC_OK;
}


extern int editConnectivity(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject facet, bag;

    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));
    ohGetOrCreateBag( &facet, &bag, "STEPS" );

    stepsMultiText( &bag, "Basic Connectivity Info", connectivityTable );

    return RPC_OK;
}

extern int editSpiceAnalysis(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject facet, bag;

    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));
    ohGetOrCreateBag( &facet, &bag, "STEPS" );

    stepsMultiText( &bag, "SPICE ANALYSES", analysisTable );

    return RPC_OK;
}
extern int editSpicePrint(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject facet, bag;

    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));
    ohGetOrCreateBag( &facet, &bag, "STEPS" );

    stepsMultiText( &bag, "Request for SPICE output", printTable );

    return RPC_OK;
}



extern int editStepsOptions(spotPtr, cmdList, userOptionWord)
RPCSpot *spotPtr;
lsList cmdList;
long userOptionWord;
{
    octObject facet, bag;

    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));
    ohGetOrCreateBag( &facet, &bag, "STEPS" );

    stepsMultiText( &bag, "STEPS options", stepsTable );

    return RPC_OK;
}




extern int spiOptions(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject	facet, bag, bag2;

    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));

    spiceOptions(&facet, &bag);
    spiceOptions2(&facet, &bag2);
    return(RPC_OK);
}



extern int printSpiceDeck(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject	facet, inst, bag, objBag, net;
    struct octBox box;
    regObjGen	gen;
    RPCArg	*arg;
    Pointer	pointer;
    char	*fmt = "format: {nets} {boxes} : spice-deck\n";
    int		idx;


    errPushHandler( stepsErrHandler );
    if ( setjmp( stepsJmp ) ) {
	errPopHandler();
	return RPC_OK;
    }



    dc_ok = 0;
    ac_ok = 0;
    tran_ok = 0;
    disto_ok = 0;
    noise_ok = 0;


    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));
    
    /* changesProcess( &facet ); */


    highlight_flag = 0;
    if (vem_selected_set!=0) {
	(void) vemClearSelSet(vem_selected_set);
    }


    nodeclear(&facet);
    deckInit(&facet, 0 );
    processCell(&facet,0);


    deckFinish(&facet);
    stepsRpcFinish( spotPtr);
    
    errPopHandler();
    return(RPC_OK);
}





int findInst(spotPtr, spotFacet, inst)
    RPCSpot		*spotPtr;
    octObject	*spotFacet, *inst;
{
    spotFacet->objectId = spotPtr->facet;
    OH_ASSERT(octGetById(spotFacet));

    switch (vuFindSpot(spotPtr, inst, OCT_INSTANCE_MASK)) {
    case VEM_CANTFIND:
	stepsEcho("Please point at an instance or at a formal terminal.\n");
	vemPrompt();
	return (0);
    case VEM_NOSELECT:
	return (0);
    case VEM_OK:
	break;
    }
    return(1);
}


int changeGlobalPars(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject	facet, bag, inst, prop;
    octGenerator gen;
    RPCArg* arg;
    char *modelname = 0;
    int count = 0;

    OH_ASSERT(ohGetById(&facet, spotPtr->facet));

    if ( lsLength(cmdList) == 0 ) {
	ohGetOrCreateBag(&facet, &bag, "MODELS");
	stepsMultiText( &bag, "Enter the default models", modelTable );
    } else if ( lsLength(cmdList) == 1 ) {
	lsFirstItem( cmdList, (Pointer*)&arg, (lsHandle*)0 );
	if ( arg->argType != VEM_TEXT_ARG ) {
	    VEMMSG( "Illegal argument (should be text)" );
	    return RPC_OK;
	}
	modelname = arg->argData.string;
	octInitGenContentsSpecial( &facet, OCT_INSTANCE_MASK, &gen );
	while ( octGenerate( &gen, &inst ) == OCT_OK ){
	    if ( ohGetByPropName( &inst,&prop,"model" ) == OCT_OK ) {
		if ( PROPEQ( &prop, modelname ) ) {
		    count++;
		    highlight( &inst );
		}
	    }
	}
	if ( count ) {
	    tableEntry oneModelTable[2];
	    ohGetOrCreateBag(&facet, &bag, "MODELS");

	    fillTableEntry( oneModelTable, 0, modelname, S, "", modelname, 60, 0, 0 );
	    fillTableEntry( oneModelTable, 1, 0, S, 0, 0, 0, 0, 0 );

	    stepsMultiText( &bag, "Edit the model", oneModelTable );
	    vemClearSelSet(vem_selected_set);
	} else {
	    VEMMSG( "No device with that model\n" );
	}

    } else {
	VEMMSG( "This function wants at most one text argument\n" );
    }
    return RPC_OK;
}

int listModels(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject	facet, bag, prop;
    octGenerator gen;

    OH_ASSERT(ohGetById(&facet, spotPtr->facet));

    VEMMSG( "MODELS:" );
    if ( ohGetOrCreateBag(&facet, &bag, "MODELS")== OCT_OK ) {
	octInitGenContents(&bag, OCT_PROP_MASK, &gen );
	while ( octGenerate( &gen, &prop ) == OCT_OK ) {
	    VEMMSG( prop.contents.prop.name );
	    VEMMSG( " " );
	}
    }
    VEMMSG("\n");
    vemPrompt();
    

    return RPC_OK;
}


 




#ifdef UNUSEDCODE
static int select(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    octObject facet, item;
    RPCArg *arg;
    vemSelSet seto = 0;

    facet.objectId = spotPtr->facet;

    if (octGetById(&facet) < OCT_OK) {
	stepsEcho( "facet");
	return(RPC_OK);
    }
  
    if (vuFindSpot(spotPtr, &item, OCT_INSTANCE_MASK) == VEM_OK) {
	seto = vemNewSelSet(facet.objectId,35000, 0, 0, 1, 1, 4, 4,
			    "0001 0010 0100 1000");
	(void) vemAddSelSet(seto, item.objectId);
    }
    return(RPC_OK);
}
#endif


extern int modelValueOff(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    vemWnTurnOffLayer(spotPtr->theWin, "RED");
    stepsRpcFinish( spotPtr);
    return(RPC_OK);
}

extern int modelValueOn(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    vemWnTurnOnLayer(spotPtr->theWin, "RED");
    stepsRpcFinish( spotPtr);
    return(RPC_OK);
}

extern int modelNameOff(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    vemWnTurnOffLayer(spotPtr->theWin, "GREEN");
    stepsRpcFinish( spotPtr);
    return(RPC_OK);
}

extern int modelNameOn(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    vemWnTurnOnLayer(spotPtr->theWin, "GREEN");
    stepsRpcFinish( spotPtr);
    return(RPC_OK);
}

extern int nodeValueOff(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    vemWnTurnOffLayer(spotPtr->theWin, "BLACK");
    stepsRpcFinish( spotPtr);
    return(RPC_OK);
}

extern int nodeValueOn(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    vemWnTurnOnLayer(spotPtr->theWin, "BLACK");
    stepsRpcFinish( spotPtr);
    return(RPC_OK);
}

extern int allValueOff(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    vemWnTurnOffLayer(spotPtr->theWin, "BLACK");
    vemWnTurnOffLayer(spotPtr->theWin, "GREEN");
    vemWnTurnOffLayer(spotPtr->theWin, "RED");
    stepsRpcFinish( spotPtr);
    return(RPC_OK);
}

extern int allValueOn(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    vemWnTurnOnLayer(spotPtr->theWin, "BLACK");
    vemWnTurnOnLayer(spotPtr->theWin, "RED");
    vemWnTurnOnLayer(spotPtr->theWin, "GREEN");
    stepsRpcFinish( spotPtr);
    return(RPC_OK);
}





extern int deHighlight(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    if (vem_selected_set!=0) {
	(void) vemClearSelSet(vem_selected_set);
    }
    return(RPC_OK);
}



highlight(inst)
    octObject *inst;
{
    highlight_flag = 1;
    if ( vem_selected_set == 0 ) {
	octObject facet;
	octGetFacet( inst, &facet );
	vem_selected_set = vemNewSelSet(facet.objectId, 0, 0, 0, 1, 1,
					4, 4, "0001 0010 0100 1000");
    }

    (void) vemAddSelSet(vem_selected_set, inst->objectId);
}
extern long labelFormalSupply();
extern long labelFormalGround();
extern long labelFormalInput();
extern long labelFormalOutput();

RPCFunction CommandArray[] = {
    { modelEdit,	"Edit",		"element", "", 0		},
    { changeGlobalPars,	"Edit", 	"models", "", 0	},
    { listModels,	"Edit", 	"list-models", "", 0	},
    { labelFormalSupply, "Terminal Props", "supply-term", "", 0 },
    { labelFormalGround, "Terminal Props", "ground-term", "", 0 },
    { labelFormalInput, "Terminal Props", "input-term", "", 0 },
    { labelFormalOutput, "Terminal Props", "output-term", "", 0 },
    { editConnectivity,  "Options",     "substrate-and-ground", 0, 0 },
    { editSpiceAnalysis,"Options",      "choose-analysis", 0, 0 },
    { editSpicePrint,   "Options",      "print-and-plot", 0, 0 },
    { editStepsOptions, "Options",      "steps-options", 0, 0 },
    { editSpiceOptions, "Options",      "common-spice-options", 0, 0 },
    { spiOptions,	"Options",	"obscure-spice-defaults" , "", 0 	        }, 
    { deHighlight,	"Display",	"highlight-off"  , "", 0	},
    { modelNameOff,	"Display",	"name-off", "", 0	},
    { modelNameOn,	"Display",	"name-on" , "", 0	},
    { modelValueOff,	"Display",	"value-off", "", 0	},
    { modelValueOn,	"Display",	"value-on", "", 0	},
    { nodeValueOff,	"Display",	"node-value-off", "", 0	},
    { nodeValueOn,	"Display",	"node-value-on" , "", 0	},
    { allValueOff,	"Display",	"all-value-off"	      , "", 0  },
    { allValueOn,	"Display",	"all-value-on" , "", 0	        },
    { printSpiceDeck,	"Main",		"prepare-spice-deck", "", 0		},
    { quit,		"Main", 	"exit-steps", "", 0  		},
};

jmp_buf stepsJmp;

void stepsErrHandler( pkgname, code, message )
    char* pkgname;
    int code;
    char* message;
{
    char buf[1024];
    
    if ( !strcmp( pkgname, "STEPS" ) ) {
	sprintf( buf, "Error detected by %s:%s\n", pkgname, message );
	VEMMSG( buf );
	longjmp( stepsJmp, 1 );
    } else {
	errPass( pkgname, code, message );
    }
}


/*ARGSUSED*/
long
UserMain(display, spotPtr, cmdList, userOptionWord, array)
    char *display;
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
    RPCFunction **array;
{
    int nCommand;
    octObject layer,facet,prop;

    strcpy(cur_deck,"unknown");
    strcpy(cur_deck_wo,"unknown");

    strcpy(display_ptr, display);

    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));

    stepsDefaults( &facet );


    stepsEcho("Version 2.2 operational\n");
    vemPrompt();

    if ( ohGetByPropName(&facet,&prop,"VIEWTYPE" ) != OCT_OK ) {
	errRaise( "STEPS", 1, "Cell does not have VIEWTYPE" );
    }
    if ( PROPEQ( &prop, "SCHEMATIC" ) ) {
	/* Make sure these layers are displayed. */
	ohGetOrCreateLayer( &facet, &layer, "RED" );    
	ohGetOrCreateLayer( &facet, &layer, "GREEN" );
	ohGetOrCreateLayer( &facet, &layer, "BLACK" );
    }

    changesStart( &facet );

    *array = CommandArray;
    return(sizeof(CommandArray)/sizeof(RPCFunction));
}

stepsEcho( s )
    char* s;
{
    char buf[1024];
    sprintf( buf, "STEPS: %s\n", s );
    VEMMSG( buf );
}

FILE* stepsFopen( name, mode )
    char* name;
    char* mode;
{
    if ( mode[0] == 'r' ) {
	return fopen( name, "r" ); /* Because RPC does not support this mode. */
    } else {
	return RPCfopen( name, mode );
    }
}
