/*
 * octBuild.c
 *
 * Routines to build the crystal data structure from a hierachical
 * OCT cell that conforms to OCT Symbolic Design Policy.
 * At the lowest level of the cell's hierachy, instances of FETs,
 * capacitors, resistors, or physical cells that have
 * "crystal" facets (sim2facet (1CAD)) are expected.
 */

#include "crystal.h"
#include "cOct.h"

/* imported from other crystal modules					*/
extern Node *VddNode, *GroundNode, *BuildNode();
extern Flow *BuildFlow();
extern int BuildFETCount;
extern Type TypeTable[];
extern float DiffCPerArea, DiffR, PolyCPerArea, PolyR, MetalCPerArea, MetalR;
extern float Quad();

/* globals used in this module						*/
extern unsigned cOctStatus;
octObject oFacet;
octObject oCrystalFlowBag;
st_table *OctIdTable;
extern st_table *CrNodeNames;
extern int ParseLevel, InstCount, NameCount;
char *cOctGetNetName();

/*** DEUBG */
int cOctExtTermCap = TRUE;

/* oct units to crystal units conversion factor				*/
/* crystal units appear to be half-microns.				*/
float units = 1 / 10.0;

/* MOSFET actual terminal names						*/
#define GATE "GATE"
#define GATE1 "GATE1"
#define SOURCE "SOURCE"
#define DRAIN "DRAIN"

/* property names and values						*/
#define DIRECTION_PROP "DIRECTION"
#define INPUT "INPUT"
#define OUTPUT "OUTPUT"

/* facet that contains simulation/extraction data			*/
#define SIM_FACET "crystal"

enum instType { MOSFET, RESISTOR, CAPACITOR, MODULE, CONTACT };
enum mosType { NENH, NDEP, PENH, PDEP };
typedef struct instRecord {
    enum instType type;	
    octId objectId;
    union {
	struct {
	    int type;
	    octId source;
	    octId drain;
	    octId gate;
	    octId base;
	    float width;
	    float length;
	    int xloc, yloc;
	} mos;
	struct {
	    octId pos;
	    octId neg;
	    double value;
	} res;
	struct {
	    octId node;
	    double value;
	} cap;
    } inst;
} s_instRecord;


/*
 * cOctBuildNet
 *
 * Build the crystal network represented by the top level facet.
 * This is the routine called instead of the normal crystal build routine.
 */
cOctBuildNet( facetName)
char *facetName;
{
    int result;
    octObject oFlowBag, oFTProp;
    octGenerator gBag;
    char cmdbuf[ 256 ];

    if( cOctStatus & COCT_BUILD_DONE){
	printf( "Crystal Oct reader: Warning: build already done\n");
	return ( TRUE );
    }

    octBegin();

    if( facetName == NIL(char)){
	/* RPC crystal							*/
	OH_ASSERT( octGetById( &oFacet));
    } else {
	cOctOpenFacet( facetName );
    }

    /* initialize the tables						*/
    CrNodeNames = st_init_table( st_numcmp, st_numhash );
    OctIdTable = st_init_table( st_numcmp, st_numhash );

    /* get the flow bag if it exists					*/
    if( ohGetByBagName( &oFacet, &oCrystalFlowBag, "CRYSTAL_FLOWS")
	!= OCT_OK){
	oCrystalFlowBag.objectId = oct_null_id;
    }

    result = cOctBuildFacet( &oFacet, NIL(octObject));

    cOctStatus |= COCT_BUILD_DONE;

    if (!octIdIsNull(oCrystalFlowBag.objectId)) {
	OH_ASSERT( octInitGenContents( &oCrystalFlowBag, OCT_BAG_MASK, &gBag));
	while( octGenerate( &gBag, &oFlowBag) == OCT_OK){
	    if( ohGetByPropName( &oFlowBag, &oFTProp, "CRYSTAL_FLOW_TYPE")
		== OCT_OK){
		if( cOctNotUniqueFlowName( oFlowBag.contents.bag.name)){
		    if( oFTProp.contents.prop.type == OCT_STRING){
			printf(": ");
			sprintf( cmdbuf, "flow %s %s\n",
			    oFTProp.contents.prop.value.string,
			    oFlowBag.contents.bag.name);
			printf( cmdbuf );
			CmdDo( cmdbuf);
		    } else {
			/*** bad prop type				*/
		    }
		}
	    }
	}
    }

    return( result );
}

/*
 * cOctOpenFacet
 *
 * Open the given facet.  This routine is only called by normal oct crystal.
 * RPC crystal just uses octGetById().
 */
cOctOpenFacet( facetName )
char *facetName;
{
    OH_ASSERT( ohUnpackDefaults( &oFacet, "a", "::contents"));
    OH_ASSERT( ohUnpackFacetName( &oFacet, facetName ));

    if( octOpenFacet( &oFacet ) != OCT_OLD_FACET ){
	errRaise( COCT_PKG_NAME, -1,
		"Could not open existing facet %s:%s:%s: %s\n",
		oFacet.contents.facet.cell,
		oFacet.contents.facet.view,
		oFacet.contents.facet.facet,
		octErrorString());
    }
}

/*
 * cOctBuildFacet
 *
 * Parse and build the netlist of this facet.  The facet can be
 * either a Symbolic contents facet or a symbolic extracted netlist
 * facet of a non-symbolic cell.  If a new and unnamed net is enountered
 * here, a unique name will be generated for it; otherwise, new FETs and
 * capacitors are added just as if this where part of a flat description.
 * To make this work, the nets of this facet will be deleted from the
 * global netId -> name hash table before this routine is exited.
 */
cOctBuildFacet( oDefFacet, oThisInst )
octObject *oDefFacet; /* contents or crystal/sim facet			*/
octObject *oThisInst; /* instance of this facet if this isn't top level	*/
{
    /* instance-driven netlist generation variables			*/
    octObject oInstBag;
    octObject oInst, oNextFacet;
    s_instRecord instRecord;

    /* variables for doing on-the-fly capacitance estimation		*/
    octObject oNet, oPath, oLayer;
    struct octBox termBB;
    struct octPoint octPts[ 2 ];
    int32 maxpts = 2;
    int layerBits;
    float value;
    float curcap;

    /* variables used for multiple purposes				*/
    octObject oTerm, oProp;
    octGenerator gObj, gObj2;

    /* recursion variables						*/
    int result = TRUE;
    int saveCount, i;

    /* crystal interface variables					*/
    FET *fet, *cOctBuildFET();
    s_netHash *nh, *pnh;

    if( cOctStatus & COCT_TRACE){
	for( i = 0; i < ParseLevel; ++i){
	    fprintf( stdout, "    ");
	}
	fprintf( stdout, "reading %s:%s:%s\n",
		oDefFacet->contents.facet.cell,
		oDefFacet->contents.facet.view,
		oDefFacet->contents.facet.facet );
    }
    if( oThisInst == NIL(octObject)){
	/* top level of heirarchy					*/
	cOctBuildFormalNodes( oDefFacet);
    } else {
	/* this is some module within the hierarchy			*/
	cOctLinkFormals( oDefFacet, oThisInst);
    }

    /* build graph by processing each instance				*/
    if( ohGetByBagName( oDefFacet, &oInstBag, "INSTANCES") < OCT_OK){
	fprintf(stderr,"%s: %s %s ",
	    COCT_PKG_NAME,
	    "no INSTANCES bag in facet",
	    ohFormatName( oDefFacet));
	if( oThisInst == NIL(octObject)){
	    fprintf(stderr,"%s: Can't build facet\n", COCT_PKG_NAME);
	} else {
	    fprintf(stderr,"%s: Skipping %s\n", COCT_PKG_NAME,
			ohFormatName( oThisInst));
	}

	return( FALSE);
    }

    /* generate each instance						*/
    OH_ASSERT( octInitGenContents( &oInstBag, OCT_INSTANCE_MASK, &gObj ));
    while( octGenerate( &gObj, &oInst) == OCT_OK){
	
	/* calculate somewhat reasonable new name			*/
	++InstCount;
	NameCount = 0;

	/* find the type of this instance				*/
	cOctGetInstType( &oInst, &instRecord );

	switch( instRecord.type){
	    case MOSFET:

		/* extract the FET props and connectivity		*/
		if(( fet = cOctBuildFET( &oInst, &instRecord)) != NIL(FET)){
		    /* all terms of FET instance were connected		*/
		    cOctMergeFET( fet, &instRecord);
		} else {
		    result = FALSE;
		}

		/* if top level, save a mapping from the octId to the FET adr */
		if( oThisInst == NIL(octObject)){
		    if( st_insert( OctIdTable, (char *)fet,
				(char *)(oInst.objectId)) == -1){
			errRaise( COCT_PKG_NAME, -1,
				"No more memory for OctIdTable\n");
		    }
		}

		break;
	    case CAPACITOR:
		/* add the value of capacitance to each ot the nodes	*/
		OH_ASSERT( octInitGenContents( &oInst, OCT_TERM_MASK, &gObj2 ));
		while( octGenerate( &gObj2, &oTerm ) == OCT_OK ){
		    if( octGenFirstContainer( &oTerm, OCT_NET_MASK,
			    &oNet ) != OCT_OK){
			fprintf(stderr, "%s: Warning: %s %s: %s\n",
				COCT_PKG_NAME,
				"capacitor instance",
				 ohFormatName( &oInst ),
				"term without net");
			continue;
		    }

		    (void) cOctGetNetName( &oNet);
		    if( st_lookup( CrNodeNames, (char *)oNet.objectId,
						(char **)&nh) != 1){
			errRaise( COCT_PKG_NAME, -1,
				"Internal st package error");
		    }
		    nh->cap += instRecord.inst.cap.value;

		}

		break;
	    case MODULE:
		++ParseLevel;
		saveCount = NameCount;
		if( ohOpenMaster( &oNextFacet, &oInst, "contents", "r")
			< OCT_OK){
		    fprintf(stderr,"%s: %s %s:%s:%s :%s. %s %s\n",
			COCT_PKG_NAME,
			"Failed to open",
			oInst.contents.instance.master,
			oInst.contents.instance.view,
			oNextFacet.contents.facet.facet,
			octErrorString(),
			"Skipping instance",
			ohFormatName( &oInst));
		    result = FALSE;
		    break;
		}

		if ( ohGetByPropName( &oNextFacet, &oProp, "VIEWTYPE")
			    != OCT_OK){
		    fprintf(stderr,"%s: %s %s from %s. Skipping instance %s\n",
			COCT_PKG_NAME,
			"Couldn't get",
			ohFormatName( &oProp),
			ohFormatName( &oNextFacet),
			ohFormatName( &oInst));
		    result = FALSE;
		}

		if( !strcmp( oProp.contents.prop.value.string, "SYMBOLIC")
		    || !strcmp( oProp.contents.prop.value.string, "symbolic")){

		    /* its a symbolic facet, so use it as def facet of
		     * next level.
		     */
		    result = cOctBuildFacet( &oNextFacet, &oInst);

		} else {

		    /* try to get the sim facet				*/
		    if ( ohOpenMaster( &oNextFacet, &oInst, SIM_FACET, "r")
			< OCT_OK){
			fprintf( stderr,
			    "%s: %s %s:%s:%s. %s %s \"%s\" :%s. %s %s\n",
			    COCT_PKG_NAME,
			    "Failed to open",
			    oNextFacet.contents.facet.cell,
			    oNextFacet.contents.facet.view,
			    oNextFacet.contents.facet.facet,
			    "Necessary because contents facet is",
			    oProp.contents.prop.name,
			    oProp.contents.prop.value.string,
			    octErrorString(),
			    "Skipping instance",
			    ohFormatName( &oInst));
			result = FALSE;
			break;
		    } else {

			/* have crystal facet of instance		*/
			result = cOctBuildFacet( &oNextFacet, &oInst);
		    }
		}
		NameCount = saveCount;
		--ParseLevel;

		break;
	    case RESISTOR:
		fprintf(stderr, "Crystal OCT Reader: Warning: %s\n",
			"resistors unimplemented. Skipping.");
		break;
	    case CONTACT:
		break;
	    default:
		break;
	}
    }

    /* Generate all nets. Look for sim props if its a sim facet,
     * else do a quick capacitance calculation for the net and add the
     * calculation to the Crystal node.
     * Remove nets from the hash table if this isn't the top level of hierarchy.
     */

    OH_ASSERT( octInitGenContents( oDefFacet, OCT_NET_MASK, &gObj ));
    while( octGenerate( &gObj, &oNet) == OCT_OK){

	if( st_lookup( CrNodeNames, (char *)oNet.objectId, (char **)&nh) == 0){
	    /* Some spurious net in the cell that isn't connected
	     * to any instances.
	     */
	    continue;
	}

	if( strcmp( oDefFacet->contents.facet.facet, "contents")){

	    /* Look for all the possible sim properties for nets (this should
	     * be a procedure, but is left in-line for better performance).
	     */
	    if ( ohGetByPropName( &oNet, &oProp, "SIM_RESISTANCE") == OCT_OK){
		nh->res += oProp.contents.prop.value.real;
	    }

	    if ( ohGetByPropName( &oNet, &oProp, "SIM_DIFF_AREA") == OCT_OK){
		/* convert to sqaure microns from square meters		*/
		if(( value = oProp.contents.prop.value.real * (1e+12)) != 0){
		    if ( ohGetByPropName( &oNet, &oProp, "SIM_DIFF_PERIM")
								    == OCT_OK){
			nh->cap += value * DiffCPerArea;
			nh->res += DiffR *  Quad( value,
					oProp.contents.prop.value.real * 1e+6);
		    }
		}
	    }

	    if ( ohGetByPropName( &oNet, &oProp, "SIM_POLY_AREA") == OCT_OK){
		/* convert to sqaure microns from square meters		*/
		if(( value = oProp.contents.prop.value.real * (1e+12)) != 0){
		    if ( ohGetByPropName( &oNet, &oProp, "SIM_POLY_PERIM")
								    == OCT_OK){
			nh->cap += value * PolyCPerArea;
			nh->res += PolyR *  Quad( value,
					oProp.contents.prop.value.real * 1e+6);
		    }
		}
	    }

	    if ( ohGetByPropName( &oNet, &oProp, "SIM_MET_AREA") == OCT_OK){
		/* convert to sqaure microns from square meters		*/
		if(( value = oProp.contents.prop.value.real * (1e+12)) != 0){
		    if ( ohGetByPropName( &oNet, &oProp, "SIM_MET_PERIM")
								    == OCT_OK){
			nh->cap += value * MetalCPerArea;
			nh->res += MetalR *  Quad( value,
					oProp.contents.prop.value.real * 1e+6);
		    }
		}
	    }

	} else { /* it is a contents facet				*/

	    /* This section of code is an attempt to make up for the fact that
	     * at this point, we're reading a contents facet with no
	     * extaction data. The capacitance is added to increase the
	     * acuracy of the delay times for unextracted Oct cells. It will
	     * not produce totally accurate values, but it is consistant for
	     * all nets, so the reletive times remain consistant,
	     * plus the absolute times are more accurate.
	     */

	    curcap = 0.0;
	    /* generate each path attached to this node			*/
	    OH_ASSERT( octInitGenContents( &oNet, OCT_PATH_MASK, &gObj2 ));
	    while( octGenerate( &gObj2, &oPath) == OCT_OK){

		/* get the associated PAIR of points (Symbolic Policy)	*/
		if( octGetPoints( &oPath, &maxpts, octPts) < OCT_OK){
		    fprintf( stderr, "%s %s of %s: %s\n",
			"Crystal Oct reader: Couldn't get points on",
			ohFormatName( &oNet),
			ohFormatName( oDefFacet),
			octErrorString() );
		    result = FALSE;
		    continue;
		}

		/* find the length of the segment			*/
		if( octPts[0].x == octPts[1].x){
		    value = (float) abs( octPts[0].y - octPts[1].y);
		} else if( octPts[0].y == octPts[1].y){
		    value = (float) abs( octPts[0].x - octPts[1].x);
		} else {
		    value = (float) sqrt((double) (((octPts[0].x - octPts[1].x)
					   * (octPts[0].x - octPts[1].x))
					+  ((octPts[0].y - octPts[1].y)
					   * (octPts[0].y - octPts[1].y)) ));
		}
		value *= (float) oPath.contents.path.width;

		/* generate the layer of this segment			*/
		if( octGenFirstContainer( &oPath, OCT_LAYER_MASK, &oLayer)
			!= OCT_OK){
		    fprintf( stderr, "%s %s in %s: %s\n",
			"Crystal Oct reader: No layer containing",
			ohFormatName( &oPath),
			ohFormatName( oDefFacet),
			octErrorString() );
		    result = FALSE;
		    continue;
		}

		/* figure out area capacitance for this segment		*/
		if( !strcmp( oLayer.contents.layer.name, "NDIF")
		    || !strcmp( oLayer.contents.layer.name, "PDIF")){
		    curcap += DiffCPerArea * value;
		} else if( !strcmp( oLayer.contents.layer.name, "MET1")
		    || !strcmp( oLayer.contents.layer.name, "MET2")){
		    curcap += MetalCPerArea * value;
		} else if( !strcmp( oLayer.contents.layer.name, "POLY")){
		    curcap += PolyCPerArea * value;
		}

	    }

	    /* generate each terminal to find its implementing geometry	*/
	    OH_ASSERT( octInitGenContents( &oNet, OCT_TERM_MASK, &gObj2 ));
	    while( octGenerate( &gObj2, &oTerm) == OCT_OK){

		/*** DEBUG */
		if( !cOctExtTermCap) continue;
		/* find a simple BB estimate and the associated layers	*/
		cOctFindBBLayers( &oTerm, &termBB, &layerBits);

		/* find area of terminal implementation (in microns)	*/
		value = (termBB.upperRight.x - termBB.lowerLeft.x)
			* (termBB.upperRight.y - termBB.lowerLeft.y);

		/* add capacitance (area still in Oct Units)		*/
		if( layerBits & COCT_NDIF_MASK){
		    curcap += DiffCPerArea * value;
		}
		if( layerBits & COCT_PDIF_MASK){
		    curcap += DiffCPerArea * value;
		}
		if( layerBits & COCT_POLY_MASK){
		    curcap += PolyCPerArea * value;
		}
		if( layerBits & COCT_MET1_MASK){
		    curcap += MetalCPerArea * value;
		}
		if( layerBits & COCT_MET2_MASK){
		    curcap += MetalCPerArea * value;
		}

	    }

	    /* check cap value and add if big enough (100 fF threshold)	*/
	    /* 178 = ((20 units/lambda) / (1.5 micron / lambda)) ** 2	*/
	    curcap /= 178.0;
	    nh->cap += curcap;
	}

	/* get the net hash of oNet, deleting if not top level		*/
	if( oThisInst != NIL(octObject)){
	    if( st_delete( CrNodeNames, (char **)&oNet.objectId, (char **)&nh)
			!= 1){
		errRaise( COCT_PKG_NAME, -1, "Internal st package error");
	    }
	}

	/* have hash of current net, now deal with this net's capacitance,
	 * pass it up if its got a parent, or assign it to the crystal node
	 * if it doesn't have a parent and its a large enough value.
	 */
	if (!octIdIsNull(nh->parentId)) {
	    if( st_lookup( CrNodeNames, (char *)nh->parentId, (char **)&pnh)
			!= 1){
		fprintf(stderr,"%s line %d of %s\n",
			"Crystal Oct reader: internal st package error:",
			__LINE__, __FILE__);
		result = FALSE;
	    } else {
		pnh->cap += nh->cap;
		pnh->res += nh->res;
	    }
	} else {
	    if( nh->cap >= 0.10){
		BuildNode( cOctGetNetName( &oNet))->n_C += nh->cap;
		/***Is this REALLY cap to ground, or just thought of that way?*/
		GroundNode->n_C += nh->cap;
		if( cOctStatus & COCT_VERBOSE){
		    fprintf( stdout, "Adding %f pF to %s\n",
			    nh->cap, cOctGetNetName( &oNet));
		}
	    } else {
		if( cOctStatus & COCT_VERBOSE){
		    fprintf( stdout, "Ignoring %f pF on %s\n",
			    nh->cap, cOctGetNetName( &oNet));
		}
	    }
	}
	
	/* if this node hash was deleted from the hash table, free it	*/
	if( oThisInst != NIL(octObject)){
	    FREE( nh);
	} else {
	    /* otherwise, make sure this top-level net is the the
	     * OctIdTable, so it can be used in critical paths
	     */
	    if( st_insert( OctIdTable,
			(char *)BuildNode( cOctGetNetName( &oNet)),
			(char *)(oNet.objectId)) == -1){
		errRaise( COCT_PKG_NAME, -1, "No more memory for OctIdTable.");
	    }
	}

    }

    return ( result );
}

/* some globals used with each fet inst to reduce regeneration and
 * excesive paramenter passing.
 */
static octObject oCurGateTerm, oCurSourceTerm, oCurDrainTerm;

/*
 * cOctBuildFET
 *
 * Create and fill the fields of a crystal FET structure.
 */
FET *
cOctBuildFET(oInst, iRec)
octObject *oInst;
s_instRecord *iRec;
{
    octGenerator gTerm;
    octStatus sProp;
    octObject oTerm, oNet, oProp;
    FET *fet;
    float width, length;
    char *nodeName;

    /* create FET struct 						*/
    ++ BuildFETCount;
    fet = ALLOC( FET, 1);
    width = (float) iRec->inst.mos.width;
    length = (float) iRec->inst.mos.length;
    fet->f_area = width * length;
    fet->f_aspect = length / width;
    fet->f_xloc = (float) iRec->inst.mos.xloc * units;
    fet->f_yloc = (float) iRec->inst.mos.yloc * units;
    fet->f_typeIndex = iRec->inst.mos.type;
    fet->f_flags = 0;
    fet->f_fp = NIL(FPointer);
    fet->f_gate = 0;
    fet->f_drain = 0;
    fet->f_source = 0;

    /* clear objectIds to detect if inst doesn't have all FET terms.	*/
    oCurGateTerm.objectId = oCurSourceTerm.objectId =
	oCurDrainTerm.objectId = oct_null_id;

    /* find nets and link nodes						*/
    OH_ASSERT( octInitGenContents( oInst, OCT_TERM_MASK, &gTerm ));
    while( octGenerate( &gTerm, &oTerm ) == OCT_OK ){
	if(  octGenFirstContainer( &oTerm, OCT_NET_MASK,
		    &oNet ) != OCT_OK){
	    /*** this gets to be quite obnoxious			*/
	    if( cOctStatus & COCT_VERBOSE){
		fprintf(stdout, "%s: %s %s %s %s\n",
		    COCT_PKG_NAME,
		    "actual term",
		    oTerm.contents.term.name,
		    "has no net in",
		     ohFormatName( oInst ));
	    }
	    continue;
	}

	nodeName = cOctGetNetName( &oNet);

	sProp = ohGetByPropName( &oTerm, &oProp, DIRECTION_PROP);

	/* figure out which term of the mosfet this is			*/
	if( !strcmp( oTerm.contents.term.name, GATE ) ||
	    !strcmp( oTerm.contents.term.name, GATE1 )){
	    fet->f_gate = BuildNode( nodeName );
	    oCurGateTerm = oTerm;
	} else if( !strcmp( oTerm.contents.term.name, DRAIN)){
	    fet->f_drain = BuildNode( nodeName );
	    oCurDrainTerm = oTerm;
	    if( sProp == OCT_OK){
		if( !strcmp( oProp.contents.prop.value.string, INPUT)){
		    fet->f_flags = FET_NOSOURCEINFO;
		}else if( !strcmp( oProp.contents.prop.value.string, OUTPUT)){
		    fet->f_flags = FET_NODRAININFO;
		}
	    }
	} else if( !strcmp( oTerm.contents.term.name, SOURCE)){
	    fet->f_source = BuildNode( nodeName );
	    oCurSourceTerm = oTerm;
	    if( sProp == OCT_OK){
		if( !strcmp( oProp.contents.prop.value.string, INPUT)){
		    fet->f_flags = FET_NODRAININFO;
		} else if( !strcmp(oProp.contents.prop.value.string, OUTPUT)){
		    fet->f_flags = FET_NOSOURCEINFO;
		}
	    }
	} else {
	    fprintf(stderr,"%s: Bad term name on %s: %s\n",
		    COCT_PKG_NAME,
		    ohFormatName(oInst),
		    oTerm.contents.term.name);
	    fprintf(stderr,"\tExpecting one of %s, %s, %s, %s\n",
		    GATE, GATE1, SOURCE, DRAIN);
#ifdef notdef
	/* XXX don't die if the BULK terminal is around */
	    fprintf(stderr,"Skipping instance.\n");
	    return( NIL(FET));
#endif
	}
    }

    if (fet->f_drain == 0) {
        fprintf(stderr,"%s: no DRAIN connection for %s at (%d,%d)\n",
            COCT_PKG_NAME, ohFormatName(oInst),
            oInst->contents.instance.transform.translation.x,
            oInst->contents.instance.transform.translation.y);
        return(0);
    }
    if (fet->f_source == 0) {
        fprintf(stderr,"%s: no SOURCE connection for %s at (%d,%d)\n",
            COCT_PKG_NAME, ohFormatName(oInst),
            oInst->contents.instance.transform.translation.x,
            oInst->contents.instance.transform.translation.y);
        return(0);
    }
    if (fet->f_gate == 0) {
        fprintf(stderr,"%s: no GATE connection for %s at (%d,%d)\n",
            COCT_PKG_NAME, ohFormatName(oInst),
            oInst->contents.instance.transform.translation.x,
            oInst->contents.instance.transform.translation.y);
        return(0);
    }

    return( fet );
}

/*
 * cOctMergeFET
 *
 * This code is essentially from build.c of the crystal distribution.
 * It performs transistor merging where possible and also adds to node
 * capacitances.  The major difference is that where the original code
 * parses attributes, the cOct code checks for containment of the instance
 * terminals by a flow bag.
 */
cOctMergeFET( fet, iRec)
FET *fet; /* the new transistor						*/
s_instRecord *iRec; /* needed if the transistor is merged		*/
{
    Pointer *p;
    float cap;
    Type *typeEntry;
    octObject oFlowBag;

    /* See if there is already a transistor of the same type
     * between the same three nodes.  If so, just lump the
     * two transistors into a single bigger transistor (this
     * handles pads with multiple driving transistors in
     * parallel).  If not, then create pointers to the fet
     * from each of the nodes it connects to.  Note:  it doesn't
     * matter which terminal's list we examine, but pick one
     * other than Vdd or Ground, or this will take a LONG time.
     */
		
    if ((fet->f_source != 0)
        && (fet->f_source != VddNode)
	&& (fet->f_source != GroundNode))
	p = fet->f_source->n_pointer;
    else if ((fet->f_drain != 0)
        && (fet->f_drain != VddNode)
	&& (fet->f_drain != GroundNode))
	p = fet->f_drain->n_pointer;
    else if ((fet->f_gate != 0)
        && (fet->f_gate != VddNode)
	&& (fet->f_gate != GroundNode))
	p = fet->f_gate->n_pointer;
    else p = NIL(Pointer);

    while (p != NIL(Pointer))
    {
	FET *f2;

	f2 = p->p_fet;
	if ((f2->f_gate == fet->f_gate)
	    && (f2->f_source == fet->f_source)
	    && (f2->f_drain == fet->f_drain)
	    && (f2->f_typeIndex == fet->f_typeIndex))
	{
	    f2->f_area += fet->f_area;
	    f2->f_aspect = 1.0/(1.0/fet->f_aspect
		+ 1.0/f2->f_aspect);
	    free((char *) fet);
	    fet = f2;
	    break;
	}
	p = p->p_next;
    }
		    
    if (p == NIL(Pointer))
    {
	BuildPointer(fet->f_gate, fet);
	if (fet->f_drain != fet->f_gate)
	    BuildPointer(fet->f_drain, fet);
	if ((fet->f_source != fet->f_gate)
	    && (fet->f_source != fet->f_drain))
	    BuildPointer(fet->f_source, fet);
    }

    /* Do the equivalent of handling FET attributes.  The properties have
     * already been handled, so here we deal only with flows.
     * THIS IS OCT-VERSION-ONLY CODE.
     */
    /* if term contained by a bag, and if the bag contained by CFB	*/
    if (!octIdIsNull(oCrystalFlowBag.objectId)) {
	if (!octIdIsNull(oCurSourceTerm.objectId)) {
	    if( octGenFirstContainer( &oCurSourceTerm, OCT_BAG_MASK, &oFlowBag)
		== OCT_OK){
		if( octIsAttached( &oCrystalFlowBag, &oFlowBag)){
		    /* the term is part of a crystal flow		*/
		    cOctSetTermFlow( fet, &oCurSourceTerm,
			oFlowBag.contents.bag.name);
		}
	    }
	}

	if (!octIdIsNull(oCurDrainTerm.objectId)) {
	    if( octGenFirstContainer( &oCurDrainTerm, OCT_BAG_MASK, &oFlowBag)
		== OCT_OK){
		if( octIsAttached( &oCrystalFlowBag, &oFlowBag)){
		    /* the term is part of a crystal flow		*/
		    cOctSetTermFlow( fet, &oCurDrainTerm,
			oFlowBag.contents.bag.name);
		}
	    }
	}
    }
    /* Back to crystal distribution code.				*/

    /* Figure out which transistors are loads, and mark them
     * as such.
     */

    if (fet->f_typeIndex == FET_NDEP)
    {
	if (fet->f_source == VddNode)
	{
	    fet->f_typeIndex = FET_NLOAD;
	    if (fet->f_gate != fet->f_drain)
		fet->f_typeIndex = FET_NSUPER;
	}
	else if (fet->f_drain == VddNode)
	{
	    fet->f_typeIndex = FET_NLOAD;
	    if (fet->f_gate != fet->f_source)
		fet->f_typeIndex = FET_NSUPER;
	}
    }
    typeEntry = &(TypeTable[fet->f_typeIndex]);
    fet->f_flags |= typeEntry->t_flags;

    /* If the gate doesn't connect to either source or drain,
     * then add its gate capacitance onto the gate node.
     * Also add in source and drain overlap capacitances,
     * if the relevant terminal doesn't connect to the gate.
     * Be careful to recompute the fet area, because we
     * may have merged two transistors.
     */
		
    if ((fet->f_gate != fet->f_source)
	&& (fet->f_gate != fet->f_drain))
	fet->f_gate->n_C += iRec->inst.mos.length*
		    iRec->inst.mos.width*typeEntry->t_cPerArea;
    cap = iRec->inst.mos.width*typeEntry->t_cPerWidth;
    if (fet->f_drain != fet->f_gate)
    {
	fet->f_drain->n_C += cap;
	fet->f_gate->n_C += cap;
    }
    if (fet->f_source != fet->f_gate)
    {
	fet->f_source->n_C += cap;
	fet->f_gate->n_C += cap;
    }

}

/*
 * cOctSetTermFlow
 *
 * Get or create the flow, then make the appropriate
 * attachments and set the appropriate flags.
 * This is meant to be used both during a batch build and an interactive
 * RPC session.
 */
cOctSetTermFlow( fet, oTerm, flowName)
FET *fet;
octObject *oTerm;
char *flowName;
{
    int aloc;
    Flow *flow;
    FPointer *fp;

    if( !strcmp(oTerm->contents.term.name, SOURCE)){
	aloc = FP_SOURCE;
    } else if( !strcmp(oTerm->contents.term.name, DRAIN)){
	aloc = FP_DRAIN;
    } else {
	/* message about bad term name already issued.			*/
	return;
    }

    flow = BuildFlow( flowName);
    fp = ALLOC( FPointer, 1);
    fp->fp_flow = flow;
    fp->fp_flags = aloc;
    fp->fp_next = fet->f_fp;
    fet->f_fp = fp;
}

/*
 * cOctGetInstType
 *
 * gets the CELLTYPE property and any celltype specific properties
 */
cOctGetInstType( oInst, instRecord )
octObject *oInst;
s_instRecord *instRecord;
{
    octObject oCellType;
    octObject oProp;

    instRecord->objectId = oInst->objectId;
    oCellType.contents.prop.name = "CELLTYPE";
    if( !cOctGetInheritedProp( oInst, &oCellType, OCT_STRING )){
	fprintf(stderr, "Crystal Oct reader: Assuming cell type MODULE.\n");
	instRecord->type = MODULE;

	return;
    }
    if( !strcmp( oCellType.contents.prop.value.string, "MOSFET" )){

	instRecord->type = MOSFET;
	oProp.contents.prop.name = "MOSFETTYPE";
	if( !cOctGetInheritedProp( oInst, &oProp, OCT_STRING )){
	    errRaise( COCT_PKG_NAME, -1,
		"MOSFET instance didn't have `MOSFETTYPE' property");
	}
	if( strcmp( oProp.contents.prop.value.string, "NDEP" ) == 0){
	    instRecord->inst.mos.type = FET_NCHAN;
	} else if( strcmp( oProp.contents.prop.value.string, "NENH" ) == 0){
	    instRecord->inst.mos.type = FET_NCHAN;
	} else if( strcmp( oProp.contents.prop.value.string, "PDEP" ) == 0){
	    instRecord->inst.mos.type = FET_PCHAN;
	} else if( strcmp( oProp.contents.prop.value.string, "PENH" ) == 0){
	    instRecord->inst.mos.type = FET_PCHAN;
	} else {
	    errRaise( COCT_PKG_NAME, -1, "Unknown %s value on cell %s: %s\n",
		    oCellType.contents.prop.name,
		    oInst->contents.instance.name,
		    oCellType.contents.prop.value.string);
	}
	oProp.contents.prop.name = "MOSFETWIDTH";
	if( !cOctGetInheritedProp( oInst, &oProp, OCT_REAL )){
	    errRaise( COCT_PKG_NAME, -1,
		"MOSFET %s didn't have `MOSFETWIDTH' property",
                ohFormatName(oInst));
	}
	if (oProp.contents.prop.value.real <= 0.0) {
	    errRaise( COCT_PKG_NAME, -1,
		"MOSFET %s has a non-positive `MOSFETWIDTH' value",
                ohFormatName(oInst));
	}
	instRecord->inst.mos.width = oProp.contents.prop.value.real * 1e6;

	oProp.contents.prop.name = "MOSFETLENGTH";
	if( !cOctGetInheritedProp( oInst, &oProp, OCT_REAL )){
	    errRaise( COCT_PKG_NAME, -1,
		"MOSFET %s didn't have `MOSFETLENGTH' property",
                ohFormatName(oInst));
	}
	if (oProp.contents.prop.value.real <= 0.0) {
	    errRaise( COCT_PKG_NAME, -1,
		"MOSFET %s has a non-positive `MOSFETLENGTH' value",
                ohFormatName(oInst));
	}
	instRecord->inst.mos.length = oProp.contents.prop.value.real * 1e6;

	instRecord->inst.mos.xloc = (int)
			oInst->contents.instance.transform.translation.x;
	instRecord->inst.mos.yloc = (int)
			oInst->contents.instance.transform.translation.y;

    } else if( !strcmp( oCellType.contents.prop.value.string, "RESISTOR" )){

	instRecord->type = RESISTOR;
	oProp.contents.prop.name = "RESISTANCE";
	if( !cOctGetInheritedProp( oInst, &oProp, OCT_REAL )){
	    errRaise( COCT_PKG_NAME, -1,
		"RESISTOR instance didn't have `RESISTANCE' property");
	}
	instRecord->inst.res.value = oProp.contents.prop.value.real;

    } else if( !strcmp( oCellType.contents.prop.value.string, "CAPACITOR" )){

	instRecord->type = CAPACITOR;
	oProp.contents.prop.name = "CAPACITANCE";
	if( !cOctGetInheritedProp( oInst, &oProp, OCT_REAL )){
	    errRaise( COCT_PKG_NAME, -1,
		"CAPACITOR instance didn't have `CAPACITANCE' property");
	}
	instRecord->inst.cap.value = oProp.contents.prop.value.real * 1e12;

    } else if( !strcmp( oCellType.contents.prop.value.string, "CONTACT" )){

	instRecord->type = CONTACT;

    } else if(( !strcmp( oCellType.contents.prop.value.string, "COMBINATIONAL"))
	|| ( !strcmp( oCellType.contents.prop.value.string, "MODULE"))
	|| ( !strcmp( oCellType.contents.prop.value.string, "MEMORY"))){

	instRecord->type = MODULE;

    } else {

	fprintf( stderr, "%s: Unknown value on cell %s: %s - %s\n",
			COCT_PKG_NAME,
			oCellType.contents.prop.name,
			ohFormatName( oInst),
			oCellType.contents.prop.value.string,
			"default to type MODULE");

	instRecord->type = MODULE;

    }
}
