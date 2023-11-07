/*
 * octProps.c
 */

#include "cOct.h"

#define TRUE 1
#define FALSE 0

/* globals used in this module						*/
char *propTypeString();

/* property names and values						*/
#define MODEL_PROP "SIM_MODEL"
#define DIRECTION_PROP "DIRECTION"
#define INPUT "INPUT"
#define OUTPUT "OUTPUT"

/* facet that contains simulation/extraction data			*/
#define SIM_FACET "crystal"

/*
 * cOctGetProp
 *
 * Get the property from the corresponding object in the contents facet.
 */
cOctGetProp( oObj, oProp, type)
octObject *oObj; /* can be any object					*/
octObject *oProp;
enum octPropType type;
{
    oProp->type = OCT_PROP;
    if( octGetByName( oObj, oProp) != OCT_OK){
	return( FALSE);
    } else if( oProp->contents.prop.type != type){
	fprintf( stderr,"%s: prop %s %s %s %s %s %s %s %s %s\n",
	    COCT_PKG_NAME,
	    ohFormatName( oProp),
	    "found on object",
	    ohFormatName( oObj),
	    "named",
	    oProp->contents.prop.name,
	    "was found to be of type",
	    propTypeString( oProp->contents.prop.type),
	    "not specified type",
	    propTypeString( type));
	return( FALSE);
    }
    return( TRUE );
}

/*
 * cOctGetIntFacetProp
 *
 * Get the property from the corresponding object in the interface facet.
 */
cOctGetIntFacetProp( oObj, oProp, type)
octObject *oObj; /* can be a facet, net, or term			*/
octObject *oProp;
enum octPropType type;
{
    octObject oCfacet, oIntFacet, oIntObj;

    oProp->type = OCT_PROP;

    /* get the contents facet						*/
    if( oObj->type == OCT_FACET){
	oCfacet = *oObj;
    } else {
	if( octGenFirstContainer( oObj, OCT_FACET_MASK, &oCfacet) != OCT_OK){
	    return( FALSE);
	}
    }

    /* open the interface facet						*/
    oIntFacet = oCfacet;
    oIntFacet.contents.facet.facet = "interface";
    /* all interface facets are read only				*/
    oIntFacet.contents.facet.mode = "r";
    if( octOpenFacet( &oIntFacet) < OCT_OK){
	fprintf(stderr,"%s: Couldn't open %s:%s:%s %s %s: %s\n",
		COCT_PKG_NAME,
		oIntFacet.contents.facet.cell,
		oIntFacet.contents.facet.view,
		oIntFacet.contents.facet.facet,
		"to get interface facet prop",
		oProp->contents.prop.name,
		octErrorString());
	return( FALSE);
    }

    /* determine which object to look at in interface facet		*/
    if( oObj->type == OCT_FACET){
	oIntObj = oIntFacet;
    } else if( oObj->type == OCT_TERM){
	if( ohGetByTermName( &oIntFacet, &oIntObj, oObj->contents.term.name)
		< OCT_OK){
	    errRaise( COCT_PKG_NAME, 1,
		"Terms in contents and interface facets don't match");
	}
    } else if( oObj->type == OCT_NET){
	/* actually, there are no net in interfacet facets...		*/
	if( ohGetByNetName( &oIntFacet, &oIntObj, oObj->contents.net.name)
		< OCT_OK){
	    return( FALSE);
	}
    } else {
       fprintf(stderr, 
	   "%s: unknown object type %d at line %d of %s\n",
	    COCT_PKG_NAME, oObj->type, __LINE__, __FILE__);
	return( FALSE);
    }

    if( octGetByName( &oIntObj, oProp) != OCT_OK){
	return( FALSE);
    } else if( oProp->contents.prop.type != type){
	fprintf( stderr,"%s: %s %s %s %s %s %s:%s:%s %s %s %s %s %s %s\n",
	    COCT_PKG_NAME,
	    "interface inherited prop of obj",
	    ohFormatName( oObj),
	    "found on object",
	    ohFormatName( &oIntObj),
	    "of",
	    oIntFacet.contents.facet.cell,
	    oIntFacet.contents.facet.view,
	    oIntFacet.contents.facet.facet,
	    "named",
	    oProp->contents.prop.name,
	    "was found to be of type",
	    propTypeString( oProp->contents.prop.type),
	    "not specified type",
	    propTypeString( type));
	return( FALSE);
    }

    return( TRUE);
}

/*
 * cOctGetInheritedProp
 *
 * If oObj is an inst, check for the property, then open the master
 * to check the prop if necesary.
 */
cOctGetInheritedProp( oObj, oProp, type )
octObject *oObj;
octObject *oProp;
enum octPropType type;
{
    octObject oMaster, oInst, oMasterTerm;

    oProp->type = OCT_PROP;
    if( octGetByName( oObj, oProp ) != OCT_OK ){

	/* look on the master for the prop				*/
	if( oObj->type == OCT_TERM){
	    if (octIdIsNull(oObj->contents.term.instanceId)) {
		oInst.objectId = oObj->contents.term.instanceId;
		OH_ASSERT( octGetById( &oInst));
	    } else {
		return( FALSE);
	    }
	} else {
	    /* have instance object					*/
	    oInst = *oObj;
	}
	oMaster.contents.facet.cell = oMaster.contents.facet.view = NIL(char); 
	if( ohOpenMaster( &oMaster, &oInst, "interface", "r" ) < OCT_OK ){
	    fprintf( stderr, "%s: %s %s:%s:%s of %s: %s\n",
		     COCT_PKG_NAME,
		     "Couldn't open master",
		     oMaster.contents.facet.cell,
		     oMaster.contents.facet.view,
		     oMaster.contents.facet.facet,
		     oInst.contents.instance.name,
		     octErrorString());
	    return( FALSE);
	}
	if( oObj->type == OCT_TERM){
	    oMasterTerm = *oObj;
	    if( octGetByName( &oMaster, &oMasterTerm) != OCT_OK){
		fprintf( stderr, "%s: %s %s %s from master %s: %s\n",
			 COCT_PKG_NAME,
			"Couldn't get",
			"formal term of actual term",
			ohFormatName( oObj ),
			ohFormatName( &oMaster ),
			octErrorString());
		return( FALSE);
	    }
	    if( octGetByName( &oMasterTerm, oProp ) != OCT_OK ){
		fprintf( stderr, "%s: %s %s prop from %s or %s: %s\n",
			 COCT_PKG_NAME,
			"Couldn't get",
			 oProp->contents.prop.name,
			 ohFormatName( oObj),
			 ohFormatName( &oMasterTerm ),
			 octErrorString());
		return( FALSE);
	    } 
	} else {
	    if( octGetByName( &oMaster, oProp ) != OCT_OK ){
		fprintf( stderr, "%s: %s %s prop from %s or %s: %s\n",
			 COCT_PKG_NAME,
			"Couldn't get",
			 oProp->contents.prop.name,
			 ohFormatName( &oInst),
			 ohFormatName( &oMaster ),
			 octErrorString());
		return( FALSE);
	    } 
	}
    }
    if( oProp->contents.prop.type != type ){
	fprintf( stderr,
		 "%s: Inherited %s prop of %s is not a %s\n",
		 COCT_PKG_NAME,
		 oProp->contents.prop.name,
		 (oObj->type == OCT_TERM) ?
			ohFormatName( oObj) : ohFormatName( &oInst),
		 propTypeString( type ));
	return( FALSE);
    }

    return( TRUE);
}


/*
 * propTypeString
 *
 * return a string version of the type of a prop
 */
char *
propTypeString( type )
    enum octPropType type;
{
    static char *retval;
    switch( type ){
	case OCT_NULL: return( retval = "OCT_NULL" );
	case OCT_INTEGER: return( retval = "OCT_INTEGER" );
	case OCT_REAL: return( retval = "OCT_REAL" );
	case OCT_STRING: return( retval = "OCT_STRING" );
	case OCT_ID: return( retval = "OCT_ID" );
	case OCT_STRANGER: return( retval = "OCT_STRANGER" );
	case OCT_REAL_ARRAY: return( retval = "OCT_REAL_ARRAY" );
	case OCT_INTEGER_ARRAY: return( retval = "OCT_INTEGER_ARRAY" );
    }
    retval = "";
    return( retval );
}
