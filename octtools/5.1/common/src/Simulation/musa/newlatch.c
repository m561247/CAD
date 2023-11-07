/***********************************************************************
 * New latch module.
 * Filename: newlatch.c
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
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define NEWLATCH_MUSA
#include "musa.h"

extern void connInit();
#define ACTIVE_HIGH_TERM        0x002
#define ACTIVE_LOW_TERM         0x004
#define INVERTED_OUTPUT_TERM    0x008
#define SYNCHRONOUS_TERM        0x010
#define ASYNCHRONOUS_TERM       0x020
#define CURRENTLY_ACTIVE_TERM   0x040

#define QX  0
#define QBX 1
#define CX  2
#define SX  3
#define RX  4
#define DX  5
#define CBX 6
#define MAXLATCHTERMS 7

static char* latchTermShortNames[] = { "Q", "QB", "CTRL", "SET", "RST", "D", "CTRLBAR" };
static char* latchTermNames[] = { "OUTPUT", "OUTPUTBAR", "CONTROL", "SET", "RESET", "INPUT", "CONTROLBAR" };
static int   latchTermTypes[] = { TERM_OUTPUT, TERM_OUTPUT, TERM_VSI, TERM_VSI, TERM_VSI, TERM_VSI, TERM_VSI };

typedef struct {
    char  *name;
    int   checkList;
    short numberOfTerms;
    short storedVoltage;
    short masterSlave;
    short terms[MAXLATCHTERMS];
} genericLatch;


/* Terminal types detected */
#define INPUT_TERM	0x01	/* input */
#define SET_TERM	0x02	/* set */
#define RESET_TERM	0x04	/* reset */
#define OUTPUT_TERM	0x08	/* output */
#define OUTPUTBAR_TERM	0x10	/* outputbar */
#define CONTROL_TERM	0x20	/* control */
#define CONTROLBAR_TERM	0x40	/* control */

/*********************************************************************
 * INITLATCH
 */
genericLatch *initLatch( masterSlave, storedVoltage  )
    int masterSlave;
    int storedVoltage;
{
    int i;			/* Counter. */
    genericLatch *latch = ALLOC(genericLatch, 1);

    latch->checkList = 0;
    latch->storedVoltage = storedVoltage;
    latch->numberOfTerms = 0;
    latch->masterSlave = masterSlave;
    for ( i = 0 ; i < MAXLATCHTERMS ; i ++ ) {
	latch->terms[i] = 0;
    }
    return latch;
}		



int readLatchTerm( prop, index, latch, termsArray, termMask, termId )
    octObject* prop;
    int index;
    genericLatch* latch;
    octObject* termsArray;
    short termMask;
    octId termId;
{
    char* name = latchTermNames[index];
    if ( ! StrPropEql(prop, name))  return 0;
    termsArray[index].objectId = termId;

    if (addToCheckList( latch, index  )) {
	errRaise( "musa", 1 , "Multiple %s terminals", name);
    }
    latch->terms[index]  = termMask;
    return 1;
}


void adjustActiveLevel( latch, termsArray, index )
    genericLatch* latch;
    octObject*    termsArray;
    int index;
{
    if ( latch->checkList & ( 1 << index ) ) {
	octObject ActiveLevel;
	if (ohGetByPropName(&termsArray[index], &ActiveLevel, "ACTIVE_LEVEL") == OCT_OK) {
	    if (StrPropEql(&ActiveLevel,"LOW")) {
		latch->terms[index] |= ACTIVE_LOW_TERM;
	    }			
	}			
    }				
}

/********************************************************************
 * LATCH_READ
 * Read a latch from Oct
 */
int16 latch_read(ifacet, inst, celltype)
    octObject	*ifacet, *inst;
    char		*celltype;
{
    musaInstance	*newmusaInstance;
    octObject           latchTerms[MAXLATCHTERMS];

    octObject		term, prop;

    int masterSlave = FALSE;
    octGenerator	termgen;
    genericLatch	*latch, *initlatch();

    (void) ignore(celltype);
    /* identify latch type */
    if (ohGetByPropName(ifacet, &prop, "SYNCHMODEL") != OCT_OK) {
	return 0;
    }			
    if (StrPropEql(&prop,"TRANSMISSION-GATE")||
	StrPropEql(&prop,"TRISTATE-DRIVER")||
	StrPropEql(&prop,"TRISTATE")) {
	return 0;
    }			
    if (StrPropEql(&prop,"TRANSPARENT-LATCH")) {
	masterSlave = FALSE;
    } else if (StrPropEql(&prop, "MASTER-SLAVE-LATCH")) {
	masterSlave = TRUE;
    } else {
	(void) sprintf(errmsg,"\n\
\tUnknown SYNCHMODEL (%s) for %s:%s\n\
\tLegal values are: TRANSPARENT-LATCH MASTER-SLAVE-LATCH TRANSMISSION-GATE TRISTATE-DRIVER",
		       prop.contents.prop.value.string,ifacet->contents.facet.cell,
		       ifacet->contents.facet.view);
	FatalError(errmsg);
    }				/* if... */

    {
	int storedVoltage = L_UNKNOWN;
	if ( ohGetByPropName( inst, &prop, "INITIAL_VALUE" ) == OCT_OK ) {
	    storedVoltage = prop.contents.prop.value.integer ? L_HIGH : L_LOW;
	    if ( verbose ) printf( "Initial value for %s is %d\n", ohFormatName(inst), storedVoltage );
	}
	latch = initLatch( masterSlave, storedVoltage );
    }

    /* Parse all terms. */
    OH_ASSERT(octInitGenContents(ifacet, OCT_TERM_MASK, &termgen));
    while (octGenerate(&termgen, &term) == OCT_OK) {
	int termMask = 0;
	octId id = term.objectId;
	prop.objectId = OCT_NULL_ID;
	if (ohGetByPropName(&term, &prop, "SYNCHTERM") == OCT_OK) {
	    termMask =  SYNCHRONOUS_TERM | ACTIVE_HIGH_TERM;
	    if ( readLatchTerm( &prop, DX, latch, latchTerms, termMask, id ) ) continue;
	    if ( readLatchTerm( &prop, QX, latch, latchTerms, termMask, id ) ) continue;
	    if ( readLatchTerm( &prop, SX, latch, latchTerms, termMask, id ) ) continue;
	    if ( readLatchTerm( &prop, RX, latch, latchTerms, termMask, id ) ) continue;
	    if ( readLatchTerm( &prop, CX, latch, latchTerms, termMask, id ) ) continue;

	    termMask = SYNCHRONOUS_TERM | INVERTED_OUTPUT_TERM;
	    if ( readLatchTerm( &prop, QBX, latch, latchTerms, termMask, id ) ) continue;

	    termMask = SYNCHRONOUS_TERM | ACTIVE_LOW_TERM;
	    if ( readLatchTerm( &prop, CBX, latch, latchTerms, termMask, id ) ) continue;

	} else  if (ohGetByPropName(&term, &prop, "ASYNCHTERM") == OCT_OK) {
	    termMask = ASYNCHRONOUS_TERM;
	    if ( readLatchTerm( &prop, SX, latch, latchTerms, termMask, id ) ) continue;
	    if ( readLatchTerm( &prop, RX, latch, latchTerms, termMask, id ) ) continue;
	}

	if ( ! octIdIsNull( prop.objectId ) ) {
	    (void) sprintf(errmsg,"\n\tIllegal value \"%s\": %s\n\t%s of %s",
			   prop.contents.prop.value.string,
			   ohFormatName( &prop ), ohFormatName( &term ), ohFormatName( ifacet ) );
	    Warning(errmsg);
	} else {
	    /* The terminal does not have SYNCHTERM, Can we ignore this fact ?*/
	    if (ohGetByPropName(&term, &prop, "TERMTYPE") != OCT_OK || 
		(!(StrPropEql(&prop,"SUPPLY") || StrPropEql(&prop,"GROUND")))) {
		errRaise( "musa", 1, "\n\
\t%s in %s has neither SYNCHTERM nor ASYNCHTERM prop\n\
\tnor is it a SUPPLY or a GROUND terminal.",
			 ohFormatName( &term ), ohFormatName( ifacet ) );
	    }
	}	
    }		

    adjustActiveLevel( latch, latchTerms, SX );
    adjustActiveLevel( latch, latchTerms, RX );
    adjustActiveLevel( latch, latchTerms, CX );

    make_leaf_inst(&newmusaInstance, DEVTYPE_LATCH, inst->contents.instance.name, latch->numberOfTerms);
    newmusaInstance->userData = (char *) latch;

    {
	int index = 0;
	int count = 0;
	int connType = TERM_OUTPUT;
	  for ( index = 0; index < MAXLATCHTERMS; index++ ) {
	      if ( latch->checkList & ( 1 << index ) ) {
		  octGetById( &latchTerms[index] );
		  /* Now switch from master(interface) terminals to instance terminals */
		  OH_ASSERT(ohGetByTermName(inst,&latchTerms[index], latchTerms[index].contents.term.name));
		  connType = latchTermTypes[index];
		  connInit( &newmusaInstance->connArray[count], connType, newmusaInstance );
		  connect_term( newmusaInstance, &latchTerms[index], count++ );
	      }
	  }
    }
    return 1;
}	

/************************************************************************
 * ADDTOCHECKLIST
 * If the bit is already set return 1, else return 0
 */
int addToCheckList( latch, index)
    genericLatch* latch;
    int index;
{
    int bit = ( 1 << index );
    if (latch->checkList & bit) {
	return 1;
    } else {
	latch->numberOfTerms++;
	latch->checkList |= bit;
	return 0;
    }				
}
/*********************************************************************
 * LATCH_INFO
 */

void latchSetResetInfo( latch, index )
    genericLatch* latch;
    int index;
{

    if ( latch->checkList & ( 1 << index ) ) {
	int asyn = (latch->terms[index] & SYNCHRONOUS_TERM);
	int al   = (latch->terms[index] & ACTIVE_LOW_TERM);
	sprintf( msgbuf, "%s is %ssynchronous active %s\n",latchTermShortNames[index],
		asyn ? "a":"", al ? "low":"high" );
	MUSAPRINT( msgbuf );
    }				
}

latch_info(inst)
	musaInstance *inst;
{
    genericLatch *latch = (genericLatch *) inst->userData;

    if (latch->masterSlave) {
	(void) sprintf(msgbuf," : MASTER/SLAVE : Stored voltage %c\n", letter_table[latch->storedVoltage]);
	MUSAPRINT(msgbuf);
    } else {
	MUSAPRINT(" : TRANSPARENT\n");
    }

    
    latchSetResetInfo( latch, CX );
    latchSetResetInfo( latch, SX );
    latchSetResetInfo( latch, RX );

    if (latch->checkList & ( 1 << QX )) {
	if (latch->terms[QX] & INVERTED_OUTPUT_TERM) {
	    MUSAPRINT(" OUTPUT1  is inverted\n");
	}
    }		
    if (latch->checkList & ( 1 << QBX )) {
	if (latch->terms[QBX] & INVERTED_OUTPUT_TERM) {
	    MUSAPRINT(" OUTPUT2  is inverted\n");
	}
    }
	
    { 
	int count = 0;
	int i = 0;
	for ( i = 0 ; i < MAXLATCHTERMS; i++ ) {
	    if ( latch->checkList & ( 1 << i ) ) {
		char* name = latchTermShortNames[i];
		connect* conn = &(inst->connArray[count]);
		singleConnectInfo( conn, name );
		count++;
	    }
	}
    }
}

static int invertedValues[] = { L_ill, L_i, L_o, L_x};

/**************************************************************************
 * SIM_LATCH
 */
int sim_latch(inst)
    musaInstance *inst;
{
    int resetActive = 0;
    int setActive = 0;
    int controlActive = 0;
    int inputIndex = 0;
    int outputIndex = 0;
    int inputVoltage = L_UNKNOWN;
    int outputVoltage = L_UNKNOWN;
    node* theNode;

    genericLatch *latch = (genericLatch *) inst->userData;

    int index = 0;
    int i;
    for ( i = 0 ; i < MAXLATCHTERMS; i++ ) {
	if ( latch->checkList & ( 1<<i ) ) {
	    theNode  = inst->connArray[index].node;
	    if ( ( latch->terms[i] & ACTIVE_LOW_TERM ) ? LOW( theNode ) : HIGH( theNode ) ) {
		latch->terms[i] |= CURRENTLY_ACTIVE_TERM; /* Set flag. */
	    } else {
		latch->terms[i] &= ~CURRENTLY_ACTIVE_TERM; /* Clear flag. */
	    }
	    if ( i == DX ) {
		inputIndex = index;
	    }
	    index++;
	}
    }


    controlActive = (latch->terms[CX] & CURRENTLY_ACTIVE_TERM) || (latch->terms[CBX] & CURRENTLY_ACTIVE_TERM );
    resetActive = latch->terms[RX] & CURRENTLY_ACTIVE_TERM;
    setActive = latch->terms[SX] & CURRENTLY_ACTIVE_TERM;
    if ( resetActive && latch->terms[RX] & SYNCHRONOUS_TERM ) {
	resetActive = controlActive;
    }
    if ( setActive && latch->terms[SX] & SYNCHRONOUS_TERM ) {
	setActive = controlActive;
    }

    if (resetActive && setActive) {
	outputVoltage = L_UNKNOWN; /* Undefined */
	if (latch->masterSlave) {
	    latch->storedVoltage = L_UNKNOWN;
	}	
    } else if (resetActive) {
	outputVoltage = L_LOW;	/* 0 */
	if (latch->masterSlave) {
	    latch->storedVoltage = L_LOW;
	}	
    } else if (setActive) {
	outputVoltage = L_HIGH;	/* 1 */
	if (latch->masterSlave) {
	    latch->storedVoltage = L_HIGH;
	}	
    } else {
	inputVoltage = inst->connArray[inputIndex].node->voltage & L_VOLTAGEMASK;
	if (latch->masterSlave) {
	    if (!controlActive ) {
		latch->storedVoltage = inputVoltage;
		return 0;
	    } else {
		outputVoltage = latch->storedVoltage;
	    }	
	} else {
	    if (controlActive) {
		outputVoltage = inputVoltage;
	    } else {
		return 0;
	    }		
	}		
    }			
    
    /* drive the outputs */
    outputIndex = 0;
    for ( i = QX ; i <= QBX ; i++ )  {
	if (latch->checkList & ( 1 << i ) ) {
	    if (latch->terms[i] & INVERTED_OUTPUT_TERM) {
		drive_output(&(inst->connArray[outputIndex]), invertedValues[outputVoltage]);
	    } else {
		drive_output(&(inst->connArray[outputIndex]), outputVoltage);
	    }		
	    outputIndex++;
	}		
    }
    return 0;
}			


