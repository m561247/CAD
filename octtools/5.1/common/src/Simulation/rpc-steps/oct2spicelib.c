/*   STEPS - Schematic Text Entry Package utilizing Schematics
 * 
 *   Denis S. Yip
 *   U.C. Berkeley
 *   1990
 */

#include "port.h"
#include "utility.h"
#include "list.h"
#include "oct.h"
#include "oh.h"
#include "st.h"
#include "th.h"
#include "da.h"
#include "table.h"
#include "defs.h"
#include "pep.h"
#include "errtrap.h"
#include <ctype.h>



extern void addModel();
extern void allModels2spice();
extern double atofe();
extern int highlight_flag;
extern int dc_ok;
extern int ac_ok;
extern int tran_ok;
extern int noise_ok;
extern int disto_ok;
extern char *index();
extern int strcmp();
char *strsave();
char *gensym();
char *genmod();
FILE *fp;
extern int sourcename;
extern float sourcevalue;
extern int sourceiname;
extern float sourceivalue;
int addsource_int;
int addisource_int;
extern int addsource_flag;
extern int addisource_flag;
int vdd_done;


char *buffer_nod();

st_table *modeltable = 0;
st_table *subckttable = 0;

lsList	 subList;

typedef struct mElem {
	octId	facetId;
	char	*modelBag;
} modElem;

static	octObject	*topFacetPtr = 0;

#define	STREQ(a,b)	(strcmp((a), (b)) == 0)



char* getStepsOption( facet, name )
    octObject *facet;
    char *name;
{
    octObject bag, prop;
    ohGetOrCreateBag( facet, &bag, "STEPS" );
    if ( ohGetByPropName( &bag, &prop, name ) == OCT_OK ) {
	return getPropValue( &prop );
    }
    return 0;
}

double getStepsOptionReal( facet, name )
    octObject *facet;
    char *name;
{
    octObject bag, prop;
    ohGetOrCreateBag( facet, &bag, "STEPS" );
    if ( ohGetByPropName( &bag, &prop, name ) == OCT_OK ) {
	return prop.contents.prop.value.real;
    }
    return 0.0;
}

int setStepsOption( facet, name, value, type)
    octObject *facet;
    char *name;
    char* value;
    int type;
{
    octObject bag, prop;
    ohGetOrCreateBag( facet, &bag, "STEPS" );
    switch (type ){
    case OCT_STRING: ohGetOrCreatePropStr( &bag, &prop, name, value ); break;
    case OCT_REAL: ohGetOrCreatePropReal( &bag, &prop, name, atofe(value) ); break;
    case OCT_INTEGER: ohGetOrCreatePropInt( &bag, &prop, name, atoi(value) ); break;
    default:
	errRaise("STEPS", 1, "Wrong type in setSetpsOptions" );
    }
}

char* getSubstrate( nmos_flag )
    int nmos_flag;
{
    
    char *s=  (nmos_flag) ? 
      getStepsOption( topFacetPtr, "Psubstrate node" ) :
	getStepsOption( topFacetPtr, "Nsubstrate node" ) ;

    if ( s == 0 || s[0] == '\0' ) {
	errRaise( "STEPS", 1, "%s substrate node has not been specified.\n\tUse the 'substrate-and-ground' command to fix this.", 
		 nmos_flag ? "P":"N"); /* nmos device needs Psubstrate bulk. */
    }


    return s;
}

char* getGround(  )
{
    static char* prev_name = 0;
	

    char *g = getStepsOption( topFacetPtr, "Ground node" ); 
    
    if ( g == 0 ||  g[0] == '\0' ) {
	return "0";
    } else {
	if ( prev_name == 0 || strcmp( g, prev_name ) ) {
	    octObject net;
	    if ( ohGetByNetName( topFacetPtr, &net, g ) != OCT_OK ) {
		errRaise("STEPS", 1, "No ground net called %s", g );
	    }
	    if ( prev_name ) FREE( prev_name );
	    prev_name = util_strsav( g );
	}
	return "0";
    }
    /* NOTREACHED */
    /* return  g; */
}




char* subcktName( master )
    octObject *master;
{
    static count = 0;
    octObject prop;
    
    while ( ohGetByPropName( master, &prop, "model" ) != OCT_OK ) {
	char buf[128];
	char* name = strrchr(master->contents.facet.cell, '/' );

	if ( name )  {
	    name++;
	} else {
	    name = master->contents.facet.cell;
	}
	sprintf( buf, "%s%d", name, count++ );
	
	ohCreateOrModifyPropStr( master, &prop, "model", buf );
    }
    
    return prop.contents.prop.value.string;
    
}



subcktHeader( fp, facet )
    FILE* fp;
    octObject *facet;
{
    octObject term, net;
    octGenerator gen;

    fprintf(fp, ".SUBCKT %s", subcktName( facet ) );
    octInitGenContents( facet, OCT_TERM_MASK, &gen );
    while ( octGenerate( &gen, &term ) == OCT_OK ) {
	if ( ohTerminalNet( &term, &net ) == OCT_OK ) {
	    fprintf( fp, " %s", nodeNumber( &net ) );
	} else {
	    fprintf( fp, "<term %s without net> ", term.contents.term.name );
	}
    }
    fprintf(fp, "\n", facet->contents.facet.cell);
}


processParasitics( facet, level )
    octObject *facet;
    int level;
{
    octObject net, net1, net2, prop;
    octGenerator gen;
    dary allNets;
    int i,j;
    double crossCap, mincap;
    extern int isRPC;

    ohGetByPropName( facet, &prop, "VIEWTYPE" );
    if ( !PROPEQ(&prop,"SYMBOLIC") ) return 0; /* No parasitics for anything other than SYMBOLIC. */


    stepsEcho( "Computing parasitics for " );
    stepsEcho( ohFormatName( facet ) );
    stepsEcho( "..." );
    allNets = daAlloc( octId, 10 );
    OH_ASSERT(octInitGenContentsSpecial(facet, OCT_NET_MASK, &gen));
    while (octGenerate(&gen, &net) == OCT_OK) {
	if ( processNet(facet, &net, level) ) {
	    *daSetLast( octId, allNets) = net.objectId;
	}
    }
    

    if ( isRPC ) {
	stepsEcho( "Cross capacitance is expensive though RPC: skipped\n" );
    } else {

	mincap = getStepsOptionReal( facet, "CapThreshMin" ) ;

	for (i = 0; i < daLen(allNets); i++ ) {
	    OH_ASSERT(ohGetById( &net1, *daGet( octId, allNets, i ))) ;
	    for (j = i+1; j < daLen(allNets); j++ ) {
		OH_ASSERT(ohGetById( &net2, *daGet( octId, allNets, j ))) ;
	    
		crossCap = pepCrossCapacitance( &net1, &net2 );
	    
		if ( crossCap >= mincap ) {
		    fprintf( fp, "CCC%s_%s %s %s %s\n",
			    nodeNumber( &net1 ), nodeNumber( &net2 ),
			    nodeNumber( &net1 ), nodeNumber( &net2 ),
			    PPP( crossCap ) );
		} else {
		    /* XXX temporary code */
		    fprintf( fp, "*** cross cap %s %s %s --- small (<%s)\n",
			    nodeNumber( &net1 ), nodeNumber( &net2 ),
			    PPP( crossCap ), PPP(mincap) );
		}
			
	    }
	}
    }
    daFree( allNets, 0 );
    
    stepsEcho( "done.\n" );

    
}


processCell(facet, level)
    octObject *facet;
    int level;
{
    octObject	ibag, inst, term, net, savFacet;
    octObject   master, celltype, top, cellclass, modelName, value; 
    octGenerator gen, tgen;
    octId	*subPtr;
    modElem	*modPtr;
    char	*dummy, tem[5];
    int		cnt;


    if ( !topFacetPtr ) {
	topFacetPtr = facet;
    }

    if (!modeltable ) {
	modeltable = st_init_table(strcmp, st_strhash);
	subckttable = st_init_table(strcmp, st_strhash);
	subList = lsCreate();
    }



    renumberNodes( facet );

    if (level) {
	subcktHeader( fp, facet );
    }

    OH_ASSERT(ohGetByBagName(facet, &ibag, "INSTANCES"));

    OH_ASSERT(octInitGenContentsSpecial(&ibag, OCT_INSTANCE_MASK, &gen));
    while (octGenerate(&gen, &inst) == OCT_OK) {
	processInst(facet, &inst, level);
    }

    processParasitics( facet, level );

    if ( level == 0 ) {
	OH_ASSERT(octInitGenContentsSpecial(facet, OCT_TERM_MASK, &gen));
	while (octGenerate(&gen, &term) == OCT_OK) {
	    ERR_IGNORE(processTerm(facet, &term, level));
	}
    }

    /*
     * Process saved subckts, excluding those saved by recursion.
     */
    while ( lsDelBegin( subList, (lsGeneric *) &subPtr) != LS_NOMORE ) {
	octObject savFacet;

	savFacet.objectId = *subPtr;
	OH_ASSERT(octGetById(&savFacet));
	FREE(subPtr);

	fprintf( fp, "\n** Subcircuit declaration for %s\n", ohFormatName( &savFacet ) );
	processCell(&savFacet, level + 1);
    }

    /* 
     * Process saved models.
     */
    if ( level == 0 )  {
	allModels2spice( fp );
    }

    if ( level ) {
	fprintf( fp, ".ENDS %s\n\n\n", subcktName( facet ) );
    }
    return 1;
}



char* processInstTerm( inst, termname, net, propname )
    octObject *inst;
    char *termname;
    octObject *net;
    char* propname;
{
    octObject term, prop;

    if (ohGetByTermName(inst, &term, termname) != OCT_OK ) return 0;

    if (octGenFirstContainer(&term, OCT_NET_MASK, net) != OCT_OK) {
	char buf[1024];
	sprintf( buf, "%s not fully connected", ohFormatName( inst ));
	stepsEcho(  buf );
	highlight(inst);	
	return "?";
    } else {
	if ( propname ) {
	    OH_ASSERT(ohCreateOrModifyPropStr(inst, &prop, propname, nodeNumber( net )));
	}
	return nodeNumber( net );
    }
}

char* masterModelName( master )
    octObject* master;
{
    char buf[256];
    sprintf( buf, "%s", master->contents.facet.cell );
    return util_strsav( buf );
}


char *getModelName( inst )
    octObject *inst;
{
    octObject prop;
    char *modelname;
    if ( ohGetByPropName( inst, &prop, "model" ) != OCT_OK ) {
	octObject master, facet;
	OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r" ));
	octGetFacet( inst, &facet );
	getModelFromMaster( &master, &prop );
	modelname = prop.contents.prop.name ;
	ohGetOrCreatePropStr( inst, &prop, "model", modelname );
	octCloseFacet( &master );
	ohGetByPropName( inst, &prop, "model" );
    }	
    return prop.contents.prop.value.string;
}
    

char *getLegalDeviceName( inst, root )
    octObject *inst;
    char *root;
{
    octObject modelname;
    octObject prop, facet, optBag;

    if ( root == 0 || strlen( root ) != 1 ) {
      errRaise( "STEPS", 1, "BUG: getLegalDeviceName(). Illegal parameter root" );
    }
    if ( ohGetByPropName( inst, &prop, "device" ) == OCT_OK ) {
      /* Compare the first letters of each string (case insensitive) */
      if ( tolower(root[0]) != tolower(prop.contents.prop.value.string[0]) ) {
	setPropValue( &prop, genmod( root ));
	OH_ASSERT( octModify( &prop ));
      }
      return prop.contents.prop.value.string;
    } else {
      char buf[200];
      char *name = inst->contents.instance.name;
	
      if ( tolower(root[0]) != tolower( name[0]) ) {
	sprintf( buf, "%s%s", root, name );
	inst->contents.instance.name = buf;
	OH_ASSERT( octModify( inst ));
      }
	
      ohCreatePropStr( inst, &prop, "device", inst->contents.instance.name );

    }
    return util_strsav( prop.contents.prop.name );
}


char* deviceName( obj )
    octObject *obj;
{
    octObject prop;
    char buf[256];

    if ( ohGetByPropName( obj, &prop, "device" ) == OCT_OK ) {
	if ( prop.contents.prop.value.string && prop.contents.prop.value.string[0] != '\0' ) {
	    return prop.contents.prop.value.string;
	}
    }
 
    sprintf( buf,"%s: device name is missing\n", ohFormatName(obj) ); 
    stepsEcho( buf );
    return "?no_device_name?";
}


octStatus getInheritedProp( obj, prop )
    octObject *obj;
    octObject *prop;
{
    octStatus rtn = OCT_NOT_FOUND;

    if ( ( rtn = octGetByName( obj, prop )) != OCT_OK ) {
	if( obj->type == OCT_INSTANCE ) {
	    octObject master;
	    ohOpenMaster(&master,obj,"interface","r" );
	    if (octGetByName( &master,prop )==OCT_OK) {
		rtn = OCT_OK;
		octCreate( obj, prop );
	    }
	    octFreeFacet(&master);
	}
    } 
    return rtn;
}

table2spice( fp, obj, table )
    FILE* fp;
    octObject *obj;
    tableEntry table[];
{
    int i;
    int something_printed = 0;
    octObject *props;
    int size = tableSize( table );

    props = ALLOC( octObject, size );
    for ( i = 0 ; i < size ; i++) {
	props[i].type = OCT_PROP;
	props[i].objectId = oct_null_id;
	props[i].contents.prop.name = table[i].propName;
	props[i].contents.prop.type = table[i].type;
	setPropValue( &props[i], table[i].defaultValue );
	getInheritedProp( obj, &props[i] );
    }


    for ( i = 0 ; i < size ; i ++ ) {
	if ( octIdIsNull( props[i].objectId )) continue;
	if ( table[i].spiceFormat == 0 ) continue;
	
	
	switch ( props[i].contents.prop.type ) {
	case OCT_INTEGER:
	    if ( props[i].contents.prop.value.integer >= 0 ) {
		something_printed = 1;
		fprintf( fp, table[i].spiceFormat, getPropValue( &props[i] ));
	    }
	    break;
	case OCT_REAL:
	    if ( props[i].contents.prop.value.real >= 0.0 ) {
		something_printed = 1;
		fprintf( fp, table[i].spiceFormat, getPropValue( &props[i] ));
	    }
	    break;
	case OCT_STRING:
	    if ( props[i].contents.prop.value.string[0] != '\0' ) {
		something_printed = 1;
		fprintf( fp, table[i].spiceFormat, getPropValue( &props[i] ));
	    }
	    break;
	}
	if ( table[i].table && doSubtable( &props[i] ) ) {
	    table2spice( fp, obj, table[i].table );
	}
    }
    
    FREE(props);
    
    if ( something_printed ) fprintf( fp, "\n" );

}



char* localModel2spice( inst, type, bagName, level )
    octObject *inst;
    char* type;
    char* bagName;
    int level;
{
    octObject localmodel;
    char *modelname  = 0;
    if ( ohGetByPropName(inst, &localmodel, "LOCALMODEL") == OCT_OK ) {
	return localmodel.contents.prop.value.string;
    } else {
	octObject master, facet;
	OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r" ));
	octGetFacet( inst, &facet );
	modelname = masterModelName( &master );
	model2spice(&facet, &master, bagName, level);
	octCloseFacet( &master );
    }
    return modelname;

}

bjt2spice( fp, inst, level )
    FILE *fp;
    octObject *inst;
    int level;
{
    octObject length, width, local;
    octObject localtype, localmodel, facet;
    octObject optBag, prop, term, emitter, base, collector, master;
    char *modelname;
    int size, i;
    int npn_flag;
    char tem[128];

    OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r" ));

    modelname  = getLegalDeviceName( inst, "q" );


    OH_ASSERT(ohGetByPropName(&master, &localtype, "BJTTYPE" ));
    npn_flag = PROPEQ( &localtype, "NPN" );


    modelname = localModel2spice( inst,
				 npn_flag ? "npn": "pnp",
				 "BJTMODEL",
				 level );

    
    {
	octObject substrate;
	
	ohGetOrCreatePropStr( inst, &substrate, "NS", getSubstrate(npn_flag) );
	
	fprintf(fp, "%s %s %s %s %s m%s ",
		deviceName( inst ),
		processInstTerm( inst, "COLLECTOR", &collector, "collector" ),
		processInstTerm( inst, "BASE", &base, "base" ),
		processInstTerm( inst, "EMITTER", &emitter, "emitter" ),
		getPropValue(&substrate),
		modelname );
    }

    table2spice( fp,  inst, bjtTable );
      
}

diode2spice( fp, inst, level )
    FILE *fp;
    octObject *inst;
    int level;
{
    octObject localtype, localmodel, facet, net;
    char *modelname;

    getLegalDeviceName( inst, "d" );
    modelname = localModel2spice( inst,
				 "d",
				 "DIODEMODEL",
				 level );

    fprintf(fp, "%s %s %s %s m%s ",
	    deviceName( inst ),
	    processInstTerm( inst, "ANODE", &net, "top" ),
	    processInstTerm( inst, "CATHODE", &net, "bottom" ),
	    modelname );

    table2spice( fp,  inst, diodeTable );
    
}



recomputeMosfetParams( inst, master )
    octObject *inst;
    octObject *master;
{
    octObject prop, term, path;
    int i; 
    int count = tableSize( mosfetTable );
    for ( i = 0; i < count; i++ ) {
	if ( ohGetByPropName( master, &prop, mosfetTable[i].propName ) == OCT_OK ) {
	    prop.objectId = oct_null_id;
	    OH_ASSERT(octCreateOrModify( inst, &prop ));
	}
    }
    
    getLegalDeviceName( inst, "m" );
    getModelName( inst );
    /* Correct some values now */

    ohGetByTermName( inst, &term, "DRAIN" );
    if ( octGenFirstContainer( &term, OCT_PATH_MASK, &path ) == OCT_OK ) {
	/* The path should be in the layer of the terminal, no need to check */
	ohGetByPropName( inst, &prop, "MOSFETDRAINPERIM" );
	prop.contents.prop.value.real -= path.contents.path.width * thCoordSize();
	octModify( &prop );
    }
    ohGetByTermName( inst, &term, "SOURCE" );
    if ( octGenFirstContainer( &term, OCT_PATH_MASK, &path ) == OCT_OK ) {
	/* The path should be in the layer of the terminal, no need to check */
	ohGetByPropName( inst, &prop, "MOSFETSOURCEPERIM" );
	prop.contents.prop.value.real -= path.contents.path.width * thCoordSize();
	octModify( &prop );
    }

}

mosfet2spice( fp, inst, level )
    FILE *fp;
    octObject *inst;
    int level;
{
    octObject length, width, local;
    octObject localtype, localmodel, facet;
    octObject optBag, prop, term, drain, source, gate, substrate, master;
    char *modelname;
    int size, i;
    int nmos_flag;
    char* pseudoSubstrate = 0;
    char tem[128];

    OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r" ));



    getLegalDeviceName( inst, "m" );




    OH_ASSERT( ohGetByPropName( &master, &localtype, "MOSFETTYPE" ));
    nmos_flag = PROPEQ(&localtype, "NENH");

    modelname = getModelName( inst );

    /* new_model2spice( modelname ); */

    if ( ohGetByTermName( inst, &term, "BULK" ) == OCT_OK ) {
	OH_ASSERT(octGenFirstContainer(&term, OCT_NET_MASK, &substrate));
    } else {
	pseudoSubstrate = getSubstrate(nmos_flag);
    }


    fprintf(fp, "%s %s %s %s %s %s ",
	    deviceName( inst ),
	    processInstTerm( inst, "DRAIN", &drain, "drain" ),
	    processInstTerm( inst, "GATE", &gate, "gate" ),
	    processInstTerm( inst, "SOURCE", &source, "source" ),
	    pseudoSubstrate ? pseudoSubstrate : nodeNumber( &substrate ),
	    modelname );

    table2spice( fp, inst, mosfetTable );
}


jfet2spice( fp, inst, level ) 
    FILE *fp;
    octObject *inst;
    int level;
{
    octObject length, width, local;
    octObject localtype, localmodel, facet, model;
    octObject optBag, prop, term, drain, source, gate, substrate, master;
    octObject *props;
    char *modelname;
    int nmos_flag;
    char tem[128];

    OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r" ));

    modelname  = getLegalDeviceName( inst, "j" );

    
    substrate.contents.net.name = "";
    if ( processInstTerm( inst, "Nbulk", &substrate, 0 ) == 0 ) {
	if ( processInstTerm( inst, "Pbulk", &substrate, 0 ) == 0 ) {
	    processInstTerm( inst, "BULK", &substrate, 0 );
	}
    }

    OH_ASSERT( ohGetByPropName(&master, &localtype, "JFETTYPE" ));
    nmos_flag = PROPEQ(&localtype, "NENH");

    modelname = localModel2spice( inst,
				 nmos_flag ? "njf": "pjf",
				 "JFETMODEL",
				 level );
    ohGetByPropName( inst, &model, "model" );
    fprintf(fp, "%s %s %s %s m%s ",
	    deviceName( &model ),
	    processInstTerm( inst, "DRAIN", &drain, "drain" ),
	    processInstTerm( inst, "GATE", &gate, "gate" ),
	    processInstTerm( inst, "SOURCE", &source, "source" ),
	    modelname );
    
    table2spice( fp, inst, jfetTable );
}

capacitor2spice( fp, inst)
    FILE *fp;
    octObject *inst;
{
    octObject  prop, top, bottom, value, master;
    
    char *modelname  = getLegalDeviceName( inst, "c" );

    fprintf(fp, "%s %s %s ",
	    deviceName( inst ),
	    processInstTerm( inst, "TOP", &top, "top" ), 
	    processInstTerm( inst, "BOTTOM", &bottom, "bottom" ));

    table2spice( fp, inst, capacitorTable );
}

capacitorGrd2spice( fp, inst)
    FILE *fp;
    octObject *inst;
{
    octObject  prop, top, bottom, value, master;
    
    char *modelname  = getLegalDeviceName( inst, "c" );

    fprintf(fp, "%s %s %s ",
	    deviceName( inst ),
	    processInstTerm( inst, "TOP", &top, "top" ),
	    getGround() );

    table2spice( fp, inst, capacitorTable );
}

resistor2spice( fp, inst)
    FILE *fp;
    octObject *inst;
{
    octObject prop, top, bottom, value, master;

    getLegalDeviceName( inst, "r" );


    fprintf(fp, "%s %s %s ",
	    deviceName( inst ),
	    processInstTerm( inst, "TOP", &top, "top" ),
	    processInstTerm( inst, "BOTTOM", &bottom, "bottom" ));
	    
    table2spice( fp, inst, resistorTable );

    if (ohGetByPropName (inst, &prop, "DISTO") == OCT_OK) {
	if ( strlen( getPropValue( &prop ) ) ) {
	    fprintf(fp, ".disto %s %s\n", deviceName( inst ), getPropValue( &prop ) );
	    disto_ok = 1;
	}
    }
}

inductor2spice( fp, inst)
    FILE *fp;
    octObject *inst;
{
    octObject prop, top, bottom, value, master;

    getLegalDeviceName( inst, "l" );


    fprintf(fp, "%s %s %s ",
	    deviceName( inst ),
	    processInstTerm( inst, "TOP", &top, "top" ),
	    processInstTerm( inst, "BOTTOM", &bottom, "bottom" ));
	    
    table2spice( fp, inst, inductorTable );

}

transformer2spice( fp, inst )
    FILE *fp;
    octObject *inst;
{
    octObject prop, top, bottom, value, master, net;
    octObject l1, l2, k, model2;
    char* name;

    name = getLegalDeviceName( inst, "l" );
    {
	char buf[64];
	sprintf( buf, "l%s", name );
	ohGetOrCreatePropStr( inst, &model2, "model2", "" );
	setPropValue( &model2, buf );
	octModify( &model2 );
    }
    

    ohGetByPropName( inst, &l1, "l1" );
    ohGetByPropName( inst, &l2, "l2" );
    ohGetByPropName( inst, &k, "k" );

    fprintf(fp, "%s %s %s %g\n",
	    deviceName( inst ),
	    processInstTerm( inst, "CTOP", &net, "ctop" ),
	    processInstTerm( inst, "CBOTTOM", &net, "cbottom"),
	    getPropValue( &l1 ) );
	    
    fprintf(fp, "l%s %s %s %g\n",
	    deviceName( inst ),
	    processInstTerm( inst, "TOP", &net, "top" ),
	    processInstTerm( inst, "BOTTOM", &net, "bottom" ),
	    getPropValue( &l2 ) );
    fprintf(fp, "%s %s l%s %g\n",
	    genmod("k"),
	    deviceName( inst ),
	    deviceName( inst ),
	    getPropValue( &k ) );
    
    table2spice( fp, inst, transformerTable );
}

int processNet( facet, net, level )
    octObject   *facet;
    octObject	*net;
    int		level;
{
    double cap = 0.0, mincap = 0.0;
    octObject nettype;
    if ( ohGetByPropName(net, &nettype, "NETTYPE" ) == OCT_OK ) {
	if (PROPEQ( &nettype, "GROUND" ) || PROPEQ( &nettype, "SUPPLY" ) ) {
	    return 0;		/* Ignore this net. */
	}
    }
    cap = pepCapacitance( net );

    mincap = getStepsOptionReal( facet, "CapThreshMin" ) ;
    
    if ( cap > mincap ) {
	fprintf( fp, "CP%s %s %s %sFarad\n",
		nodeNumber( net ), nodeNumber( net ), getGround(), PPP(cap));
    }
    return 1;			/* An interesting net. */
}    

processTerm(facet,term, level)
    octObject   *facet;
    octObject	*term;
    int		level;
{
    octObject direction, termtype;
    octObject net;
    
    if ( ohGetByPropName( term, &direction, "DIRECTION" ) != OCT_OK ) {
	errRaise( "STEPS", 1, "No DIRECTION prop on %s", ohFormatName( term ));
    }
    if ( ohGetByPropName( term, &termtype, "TERMTYPE" ) != OCT_OK ) {
	errRaise( "STEPS", 1, "No TERMTYPE prop on %s", ohFormatName( term ));
    }

    OH_ASSERT(ohTerminalNet( term, &net ));

    fprintf( fp, "* Term %-6s(%6s,%6s) is node %2s\n", term->contents.term.name,
	    getPropValue( &direction ), getPropValue( &termtype ), nodeNumber( &net ) );

    if (PROPEQ(&direction,"INPUT" )) {
	if (PROPEQ( &termtype, "SUPPLY" ) ) {
	    fprintf( fp, "%s %s %s\n",  deviceName( term ), nodeNumber( &net ), getGround() );
	    table2spice( fp, term, dcSourceTable );
	} else if (PROPEQ( &termtype, "GROUND" ) ) {
	    /* Do nothing */
	} else {
	    fprintf( fp, "%s %s %s\n" , deviceName( term ), nodeNumber( &net ), getGround() );
	    table2spice( fp, term, sourceTable );
	}
    } else if (PROPEQ(&direction,"OUTPUT") ) {
	if ( PROPEQ( &termtype, "SUPPLY" ) || PROPEQ( &termtype, "GROUND" ) ) {
	    /* Do nothing */
	} else {
	    char fmtC[256], fmtR[256], fmtL[256];
	    sprintf( fmtC, "CLOAD%s %s 0 %%sFarad\n", nodeNumber(&net), nodeNumber(&net) );
	    sprintf( fmtR, "RLOAD%s %s 0 %%sOhm\n", nodeNumber(&net), nodeNumber(&net) );
	    sprintf( fmtL, "LLOAD%s %s 0 %%sHenry\n", nodeNumber(&net), nodeNumber(&net) );
	    outputTable[0].spiceFormat = fmtC;
	    outputTable[1].spiceFormat = fmtR;
	    outputTable[2].spiceFormat = fmtL;

	    fprintf( fp, "**** LOAD on %s\n", ohFormatName( term ) );
	    table2spice( fp, term, outputTable );
		    
	}
    } 
}


processInst(facet,inst, level)
    octObject   *facet;
    octObject	*inst;
    int		level;
{
    octObject	master, cellclass, celltype, srctype, termtype, savFacet,
    net, term, top, bottom, base, collector, emitter, substrate,
    gate, source, drain, value, area, length, width, dope,
    risetime, falltime, vdd, modelname2, modelname3,
    delay, period, pwl_time, pwl_value,in,clk,
    out,in1,in2,in3,in4,capacitance, property, nodeprop,
    local, localtype, localmodel, property2;
    octId	*subPtr;
    octGenerator tgen;


    OH_ASSERT(ohOpenMaster( &master, inst, "interface", "r")); 



    OH_ASSERT(ohGetByPropName(&master, &cellclass, "CELLCLASS"));


    if (!PROPEQ(&cellclass, "LEAF")) {
	/*
	 **** SUBCKT call. Immediately open the contents facet of the master.
	 ** That is what we'll look at later on.
	 */
	OH_ASSERT(ohOpenMaster( &master, inst,  "contents", "r" ));
	getLegalDeviceName( inst, "X" );
	fprintf(fp, "%s ", deviceName( inst ));
	OH_ASSERT(octInitGenContentsSpecial(inst, OCT_TERM_MASK, &tgen));
	while (octGenerate(&tgen, &term) == OCT_OK) {
	    OH_ASSERT(octGenFirstContainer(&term, OCT_NET_MASK, &net));
	    fprintf(fp, "%s ", nodeNumber( &net ) );
	}
	fprintf(fp, "%s\n", subcktName( &master ) );


	if (!st_is_member(subckttable, subcktName(&master) )) {
	    st_insert(subckttable, subcktName( &master ), (char*)1);


	    /* can't nest subckt calls; store for later processing */
	    subPtr = ALLOC(octId,1 );
	    *subPtr = master.objectId;
	    lsNewEnd(subList, (lsGeneric) subPtr, LS_NH);
	}
	return 1;
    }


    OH_ASSERT(ohGetByPropName(&master, &celltype, "CELLTYPE"));


    /**************************************************************/
    if STREQ(celltype.contents.prop.value.string, "GROUND") {	  
	return 0;

    } else if STREQ(celltype.contents.prop.value.string, "MOSFET") {

	mosfet2spice( fp, inst, level );

    } else if STREQ(celltype.contents.prop.value.string, "JFET") {

	jfet2spice( fp, inst, level );

    } else if STREQ(celltype.contents.prop.value.string, "BJT") {

	bjt2spice( fp, inst, level );

    } else if STREQ(celltype.contents.prop.value.string, "DIODE") {

	diode2spice( fp, inst, level );

    } else if STREQ(celltype.contents.prop.value.string, "RESISTOR") {

	resistor2spice( fp, inst, level );

    } else if STREQ(celltype.contents.prop.value.string, "INDUCTOR") {

	inductor2spice( fp, inst );

    } else if STREQ(celltype.contents.prop.value.string, "TRANSFORMER") {

	transformer2spice( fp, inst, level );

    } else if STREQ(celltype.contents.prop.value.string, "CCCS") {

	depSource2spice( fp, inst );

    } else if STREQ(celltype.contents.prop.value.string, "CAPACITOR") {

	capacitor2spice( fp, inst );

    } else if STREQ(celltype.contents.prop.value.string, "CAPACITORGRD") {

	capacitorGrd2spice( fp, inst );

    } else if STREQ(celltype.contents.prop.value.string, "SOURCE") {

	indepSource2spice( fp, inst );

    }  else if STREQ(celltype.contents.prop.value.string, "CONTACT") {	  

	/* do nothing */

    } else {
	errRaise("STEPS", 1, "* unknown type of LEAF %s\n", celltype.contents.prop.value.string);
    }
}





getModelFromMaster( master, modelProp )
    octObject	*master;
    octObject   *modelProp;
{
    octObject celltype, modelBag, simBag, parm_prop;
    char bagName[256];
    

    OH_ASSERT(ohGetByPropName( master, &celltype, "CELLTYPE" ));

    sprintf(bagName,"%sMODEL", celltype.contents.prop.value.string );

    if ( ohGetByBagName( master, &modelBag, bagName ) != OCT_OK) {
	errRaise("STEPS",1, "%s primitive has no %s bag\n", ohFormatName(master), bagName);
    }

    if (ohGetByBagName(&modelBag, &simBag, "SPLICE3") != OCT_OK && 
	ohGetByBagName(&modelBag, &simBag, "SPICE3") != OCT_OK ) {
	errRaise("STEPS",1, "%s primitive has no SPLICE3 or SPICE3 model bag\n", 
		 ohFormatName(master));
    }


    /* If it has a MODEL then go with it. */
    if ( ohGetByPropName(&simBag, modelProp, "MODEL") == OCT_OK) {
	addModel( masterModelName(master), modelProp->contents.prop.value.string );
    } else {
	if (ohGetByPropName(&simBag, &parm_prop, "MODELTYPE") != OCT_OK) {
	    errRaise("STEPS",1,"%s has no MODELTYPE property\n", 
		     ohFormatName(master));
	} 
	modelProp->contents.prop.name = parm_prop.contents.prop.value.string;
	modelProp->contents.prop.value.string = "";
    }
}


model2spice(allfacet, master, bagName, level)
    octObject   *allfacet; 
    octObject	*master;
    char	*bagName;
    int		level;
    /*
     * Determines the model and prints the model statement
     */
{
    octObject model, simBag, parm_prop, bag;
    octGenerator mgen;
    modElem	*modPtr;
    int idx = 0;
    char *modelType;
    
    /* first check if the model already exists */
    if (st_is_member(modeltable, master->contents.facet.cell) == 1) {
	return 1;		/* Already done. */
    }

    if ( ohGetByBagName( master, &model, bagName ) != OCT_OK) {
	errRaise("STEPS",1, "%s primitive has no %s bag\n", master->contents.facet.cell, bagName);
    }
    if (ohGetByBagName(&model, &simBag, "SPLICE3") != OCT_OK && 
	ohGetByBagName(&model, &simBag, "SPICE3") != OCT_OK ) {
	errRaise("STEPS",1, "%s primitive has no SPLICE3 model bag\n", master->contents.facet.cell);
    }

    if (ohGetByPropName(&simBag, &parm_prop, "MODELTYPE") != OCT_OK) {
	errRaise("STEPS",1,"%s has no MODELTYPE property\n", master->contents.facet.cell);
    } 

    modelType = util_strsav(parm_prop.contents.prop.value.string);


    /* Save the model if it can't be printed because of a subcircuit def. */
    /* (waited until now because of low-level model names r, c, etc.)    */
    if (level) {
	st_insert(modeltable, master->contents.facet.cell, (char*)1);
	modPtr = ALLOC(modElem,1);
	modPtr->facetId = master->objectId;
	modPtr->modelBag = bagName;
	/* lsNewEnd(modList, (lsGeneric) modPtr, LS_NH); */
	return 1;
    }

    OH_ASSERT( ohGetOrCreateBag(allfacet, &bag, "PARAMETERS") );

    /* should print cell name, but without full path name before it */

    /* Is this any of the standard models? */
    {
	int i = -1;
	while ( modelTable[++i].propName ) {
	    octObject prop;
	    if ( !strcmp( modelTable[i].propName, modelType ) ) {
		if ( !st_is_member( modeltable, modelType ) ) {
		    ohGetOrCreatePropStr( &bag, &prop, modelType, modelTable[i].defaultValue );
		    fprintf(fp, ".model m%s %s %s\n", masterModelName( master ),
			    prop.contents.prop.name,
			    prop.contents.prop.value.string);
		    st_insert(modeltable, util_strsav(modelType), (char*)1);
		}
		return 1;
	    }
	}
    }

    
#ifdef NOTNEEDED
    /* Local model */
    fprintf(fp, ".model m%d %s ", master->objectId, modelType);
	
    OH_ASSERT( octInitGenContentsSpecial(&simBag, OCT_PROP_MASK, &mgen));
    while (octGenerate(&mgen, &parm_prop) == OCT_OK) {
	if ( PROPEQ(&parm_prop,"MODELTYPE") ) continue; /* Skip this one. */
	fprintf( fp,  "%s=%s ", parm_prop.contents.prop.name, getPropValue( &parm_prop ));
    }
    fprintf( fp, "\n" );

    st_insert(modeltable, master->contents.facet.cell, (char*)1);
#else 
    {
	char buf[256];
	sprintf( buf, "Unknown model for %s\n", ohFormatName( master ) );
	stepsEcho( buf );
	fprintf( fp, buf );
    }
#endif

    return 1;
} /* End model2spice() */


void addModel( modelname, value )
    char *modelname;
    char *value;
{
    octObject prop,bag;
    if ( ohGetByBagName(topFacetPtr, &bag, "MODELS") != OCT_OK ) {
	/* Initialize with default models */
	int i = -1;

	ohCreateBag(topFacetPtr, &bag, "MODELS");
	while ( modelTable[++i].propName ) {
	    ohGetOrCreatePropStr( &bag, &prop, modelTable[i].propName, modelTable[i].defaultValue );
	}
    }

    ohGetOrCreatePropStr( &bag, &prop, modelname, value );
}

void allModels2spice( fp )
    FILE* fp;
{
    octObject prop,bag;
    octGenerator genProp;

    fprintf( fp, "** START MODELS\n" );
    OH_ASSERT( ohGetOrCreateBag(topFacetPtr, &bag, "MODELS") );
    octInitGenContents( &bag, OCT_PROP_MASK, &genProp );
    while ( octGenerate( &genProp, &prop ) == OCT_OK ) {
	fprintf( fp, ".model %s %s\n", prop.contents.prop.name, prop.contents.prop.value.string );
    }
    fprintf( fp, "** END MODELS\n" );
    
}





    
char *
genmod(string)
    char *string;
    /* XXX has a bad memory leak. */
{
    static int countmod = 0;
    char buf2[1024];

    sprintf(buf2, "%s%d", string, countmod++);
    return(util_strsav(buf2));
}


nodeclear(facet)
    octObject *facet;
{
    octObject	ibag, inst;
    octGenerator gen;

    OH_ASSERT( ohGetByBagName( facet, &ibag, "INSTANCES" ));
    OH_ASSERT(octInitGenContentsSpecial(&ibag, OCT_INSTANCE_MASK, &gen));
    while (octGenerate(&gen, &inst) == OCT_OK) {
	nodeoff(&inst);
    }
    return 1;
}

nodeoff(inst)
    octObject	*inst;
{
    octObject	master, celltype, nodeprop;

    OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r"));
    OH_ASSERT( ohGetByPropName( &master, &celltype, "CELLTYPE" ));

    if STREQ(celltype.contents.prop.value.string, "RESISTOR") {

	ohCreateOrModifyPropStr( inst, &nodeprop, "top", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "bottom", "" );

    } else if STREQ(celltype.contents.prop.value.string, "CAPACITOR") {

	ohCreateOrModifyPropStr( inst, &nodeprop, "top", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "bottom", "" );

    } else if STREQ(celltype.contents.prop.value.string, "INDUCTOR") {

	ohCreateOrModifyPropStr( inst, &nodeprop, "top", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "bottom", "" );

    } else if STREQ(celltype.contents.prop.value.string, "MOSFET") {

	ohCreateOrModifyPropStr( inst, &nodeprop, "drain", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "gate", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "source", "" );

    } else if STREQ(celltype.contents.prop.value.string, "JFET") {

	ohCreateOrModifyPropStr( inst, &nodeprop, "drain", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "gate", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "source", "" );

    } else if STREQ(celltype.contents.prop.value.string, "BJT") {

	ohCreateOrModifyPropStr( inst, &nodeprop, "collector", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "base", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "emitter", "" );


    } else if STREQ(celltype.contents.prop.value.string, "DIODE") {
	ohCreateOrModifyPropStr( inst, &nodeprop, "top", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "bottom", "" );


    } else if STREQ(celltype.contents.prop.value.string, "SOURCE") {

	ohCreateOrModifyPropStr( inst, &nodeprop, "top", "" );
	ohCreateOrModifyPropStr( inst, &nodeprop, "bottom", "" );

    }
    return 1;
}

