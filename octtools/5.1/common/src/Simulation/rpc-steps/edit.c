#include "port.h"
#include "list.h"
#include "utility.h"
#include "rpc.h"
#include "st.h"
#include "oh.h"
#include "th.h"
#include "errtrap.h"
#include "table.h"
#include "defs.h"


extern int modelEdit(spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
    /* 
     * Invoked over an instance, modelEdit opens a dialog
     * on the master's model properties.
     * Can also be used to copy the model of another cell.
     */
{
    octObject	spotFacet, inst, master, prop, copyFacet, srctype, prop2,
    facet,
    mBag, copymBag, sBag, copysBag, property, proper, optBag;
    octGenerator gen;
    RPCArg	*pArg;
    char	buffer[1024], temp_mod[30], temp_mod2[30];
 

    errPushHandler( stepsErrHandler );
    if ( setjmp( stepsJmp ) ) {
	errPopHandler();
	return RPC_OK;
    }


    facet.objectId = spotPtr->facet;
    OH_ASSERT(octGetById(&facet));


    if (!findInst(spotPtr, &spotFacet, &inst)) {
	return RPC_OK;		/* instance not found. */
    }
    OH_ASSERT( ohOpenMaster( &master, &inst, "interface", "r" ));
    if ( ohGetByPropName( &master, &prop, "CELLTYPE" ) != OCT_OK ) {
	errRaise( "STEPS", 1, "%s is missing CELLTYPE", ohFormatName( &master ) );
    }

    if ( PROPEQ(&prop, "CONTACT" )) {
	octObject aterm, fterm;
	OH_ASSERT(octGenFirstContent( &inst, OCT_TERM_MASK, &aterm ));
	if ( octGenFirstContainer( &aterm, OCT_TERM_MASK, &fterm ) == OCT_OK ) {
	    octObject direction, termtype;
	    if ( ohGetByPropName( &fterm, &direction, "DIRECTION" ) != OCT_OK ) {
		errRaise( "STEPS", 1, "Missing DIRECTION prop" );
	    }
	    if ( ohGetByPropName( &fterm, &termtype, "TERMTYPE" ) != OCT_OK ) {
		errRaise( "STEPS", 1, "Missing TERMTYPE prop" );
	    }
	    
	    if ( PROPEQ(&direction,"INPUT" ) ) {
		if ( PROPEQ( &termtype, "SUPPLY" ) ) {
		    stepsMultiText( &fterm,  "Formal term as an input source", dcSourceTable );
		} else if ( PROPEQ( &termtype, "GROUND" ) ) {
		    stepsEcho( "That is already a GROUND terminal.\n" );
		} else {
		    stepsMultiText( &fterm,  "Formal term as an input source", sourceTable );

		}
	    } else if ( PROPEQ(&direction,"OUTPUT") ) {
		stepsMultiText( &fterm,  "Describe load on output term", outputTable );
	    }
	} else {
	    stepsEcho( "The contact does not implement a formal terminal.\n" );
	}

    } else if ( PROPEQ(&prop, "MOSFET")) {
	octObject viewType;
	
	if ( ohGetByPropName( &facet, &viewType, "VIEWTYPE" ) == OCT_OK  && PROPEQ(&viewType,"SYMBOLIC") ) {
	    thTechnology( &inst ); /* Make sure th knows what technology we are talking about. */
	    recomputeMosfetParams( &inst, &master );
	}
	stepsMultiText( &inst,  "MOSFET parameters to be modified", mosfetTable );

    } else if ( PROPEQ(&prop, "JFET")) {
	    
	stepsMultiText( &inst,  "JFET parameters to be modified", jfetTable );
	    
    } else if ( PROPEQ(&prop, "BJT")) {
	 
	stepsMultiText( &inst,  "BJT parameters ", bjtTable );

    } else if ( PROPEQ(&prop, "DIODE")) {

	stepsMultiText( &inst,  "DIODE parameters ", diodeTable );

    } else  if ( PROPEQ(&prop, "RESISTOR")) {

	stepsMultiText( &inst,  "resistor  parameters ", resistorTable );

    } else  if ( PROPEQ(&prop, "CAPACITOR") || PROPEQ(&prop, "CAPACITORGRD") ) {
		
	stepsMultiText( &inst,  "capacitor  parameters ", capacitorTable );

    } else if ( PROPEQ(&prop, "INDUCTOR")) {	  

	stepsMultiText( &inst,  "inductor  parameters ", inductorTable );

    } else if ( PROPEQ(&prop, "CCCS")) {

	stepsMultiText(&inst, "CCCS parameter", depSourceTable);

    } else if ( PROPEQ(&prop, "CCVS")) {	  

	stepsMultiText(&inst, "CCVS parameters", depSourceTable );

    } else if ( PROPEQ(&prop, "VCCS")) {	  

	stepsMultiText(&inst, "VCCS parameters", depSourceTable );
	  
    } else  if ( PROPEQ(&prop, "VCVS")) {	  

	stepsMultiText(&inst, "VCVS parameters", depSourceTable );

    } else if ( PROPEQ(&prop, "TRANSFORMER")) {	  

	stepsMultiText(&inst, "Transformer Values to be modified", 0 );

    } else if ( PROPEQ(&prop, "SOURCE")) {	
	octObject srctype;
	ohGetByPropName( &master, &srctype, "SOURCETYPE" );
	if ( PROPEQ(&srctype,"VSUPPLY") ) {
	    stepsMultiText(&inst, "Source DC Value to be modified", dcSourceTable );
	} else {
	    stepsMultiText(&inst, "Source DC Value to be modified", sourceTable );
	}

    } else {
	stepsEcho( "Instance of unknown type\n" );
    } 
    OH_ASSERT(octFreeFacet(&master));


    {
	struct octBox bb;
	struct octPoint c;	/* Center of instance bb */
	octCoord w,h;		/* width and height */
	int fact = 2;		/* Expansion factor for the box  */
	
	octBB( &inst, &bb );
	ohBoxCenter( &bb, &c );
	w = ohBoxWidth(&bb);
	h = ohBoxHeight(&bb);
	
	bb.lowerLeft.x = c.x - w * fact;
	bb.lowerLeft.y = c.y - h * fact;
	bb.upperRight.x = c.x + w * fact;
	bb.upperRight.y = c.y + h * fact;

	
	octModify( &inst );	/* Force redraw */
	vemWnQRegion( &facet, &bb ); /* this does not seem to work some times. */
	
    }

    errPopHandler();
    return RPC_OK;
}



int clearStepsAnnotation( inst )
    octObject *inst;
{
    octObject  master, prop, aterm, termtype, direction, fterm, srctype;
    octGenerator gen;
 

    OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r" ));
    if ( ohGetByPropName( &master, &prop, "CELLTYPE" ) != OCT_OK ) {
	errRaise( "STEPS", 1, "%s is missing CELLTYPE", ohFormatName( &master ) );
    }

    if ( PROPEQ(&prop, "CONTACT" )) {
	octObject aterm, fterm;
	OH_ASSERT(octGenFirstContent( inst, OCT_TERM_MASK, &aterm ));
	if ( octGenFirstContainer( &aterm, OCT_TERM_MASK, &fterm ) == OCT_OK ) {
	    octObject direction, termtype;
	    if ( ohGetByPropName( &fterm, &direction, "DIRECTION" ) != OCT_OK ) {
		errRaise( "STEPS", 1, "Missing DIRECTION prop" );
	    }
	    if ( ohGetByPropName( &fterm, &termtype, "TERMTYPE" ) != OCT_OK ) {
		errRaise( "STEPS", 1, "Missing TERMTYPE prop" );
	    }
	    
	    if ( PROPEQ(&direction,"INPUT" ) ) {
		if ( PROPEQ( &termtype, "SUPPLY" ) ) {
		    delTable( &fterm, dcSourceTable );
		} else {
		    delTable( &fterm, sourceTable );
		}
	    } else if ( PROPEQ(&direction,"OUTPUT") ) {
		delTable( &fterm, outputTable );
	    }
	} 

    } else if ( PROPEQ(&prop, "MOSFET")) {
	thTechnology( inst );	/* Make sure th knows what technology we are talking about. */
	delTable( inst, mosfetTable );
	recomputeMosfetParams( inst, &master );

    } else if ( PROPEQ(&prop, "JFET")) {
	    
	delTable( inst,  jfetTable );
	    
    } else if ( PROPEQ(&prop, "BJT")) {
	 
	delTable( inst,  bjtTable );

    } else if ( PROPEQ(&prop, "DIODE")) {

	delTable( inst,  diodeTable );

    } else  if ( PROPEQ(&prop, "RESISTOR")) {

	delTable( inst,  resistorTable );

    } else  if ( PROPEQ(&prop, "CAPACITOR") || PROPEQ(&prop, "CAPACITORGRD") ) {
		
	delTable( inst,  capacitorTable );

    } else if ( PROPEQ(&prop, "INDUCTOR")) {	  

	delTable( inst,  inductorTable );

    } else if ( PROPEQ(&prop, "CCCS")) {

	delTable(inst, depSourceTable);

    } else if ( PROPEQ(&prop, "CCVS")) {	  

	delTable(inst, depSourceTable );

    } else if ( PROPEQ(&prop, "VCCS")) {	  

	delTable(inst, depSourceTable );
	  
    } else  if ( PROPEQ(&prop, "VCVS")) {	  

	delTable(inst, depSourceTable );

    } else if ( PROPEQ(&prop, "TRANSFORMER")) {	  

	/* delTable(inst, 0 ); */

    } else if ( PROPEQ(&prop, "SOURCE")) {	
	octObject srctype;
	ohGetByPropName( &master, &srctype, "SOURCETYPE" );
	if ( PROPEQ(&srctype,"VSUPPLY") ) {
	    delTable(inst, dcSourceTable );
	} else {
	    delTable(inst, sourceTable );
	}

    }
    OH_ASSERT(octFreeFacet(&master));


}
