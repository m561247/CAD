/*
 * crystalDirection.c
 *
 * Functions to handle the editing of DIRECTION properties.
 */

#include "rpc_crystal.h"

char CDbuf[ 512 ];

/*
 * rpc_crystal_show_formal_dir
 *
 * Get the interface facet, then dump the direction props of each formal term.
 */
rpc_crystal_show_formal_dir( fctId)
octId fctId;
{
    octObject oFacet, oTerm, oProp;
    octGenerator gTerm;

    oFacet.objectId = fctId;
    OH_ASSERT( octGetById( &oFacet));

    oProp.type == OCT_PROP;
    oProp.contents.prop.name = "DIRECTION";
    oProp.contents.prop.type = OCT_STRING;

    sprintf( CDbuf, "Terms of %s", ohFormatName( &oFacet));
    echoCmd( CDbuf);
    OH_ASSERT( octInitGenContents( &oFacet, OCT_TERM_MASK, &gTerm));
    while( octGenerate( &gTerm, &oTerm) == OCT_OK){
	if (octIdIsNull(oTerm.contents.term.instanceId)) {
	    if( cOctGetIntFacetProp( &oTerm, &oProp, OCT_STRING) == TRUE){
		rpc_crystal_format_term_dir( &oTerm, &oProp);
	    } else{
		rpc_crystal_format_term_dir( &oTerm, NIL(octObject));
	    }
	}
    }
    echoCmd( "");
}

/*
 * rpc_crystal_show_instance_dir
 *
 * For each terminal, dump the inherited direction property.
 */
rpc_crystal_show_instance_dir( oInst)
octObject *oInst;
{
    octObject oTerm, oProp;
    octGenerator gTerm;

    oProp.type == OCT_PROP;
    oProp.contents.prop.name = "DIRECTION";
    oProp.contents.prop.type = OCT_STRING;

    sprintf( CDbuf, "Terms of %s", ohFormatName( oInst));
    echoCmd( CDbuf);
    OH_ASSERT( octInitGenContents( oInst, OCT_TERM_MASK, &gTerm));
    while( octGenerate( &gTerm, &oTerm) == OCT_OK){
	if( cOctGetInheritedProp( &oTerm, &oProp, OCT_STRING) == TRUE){
	    rpc_crystal_format_term_dir( &oTerm, &oProp);
	} else{
	    rpc_crystal_format_term_dir( &oTerm, NIL(octObject));
	}
    }
    echoCmd( "");
}

/*
 * rpc_crystal_show_terminal_dir
 *
 * Given an actual or formal terminal, give the inherited direction.
 */
rpc_crystal_show_terminal_dir( oTerm)
octObject *oTerm;
{
    octObject oProp;

    oProp.type == OCT_PROP;
    oProp.contents.prop.name = "DIRECTION";
    oProp.contents.prop.type = OCT_STRING;

    if (octIdIsNull(oTerm->contents.term.instanceId)) {
	if( cOctGetIntFacetProp( oTerm, &oProp, OCT_STRING) == TRUE){
	    rpc_crystal_format_term_dir( oTerm, &oProp);
	} else{
	    rpc_crystal_format_term_dir( oTerm, NIL(octObject));
	}
    } else {
	if( cOctGetInheritedProp( oTerm, &oProp, OCT_STRING) == TRUE){
	    rpc_crystal_format_term_dir( oTerm, &oProp);
	} else{
	    rpc_crystal_format_term_dir( oTerm, NIL(octObject));
	}
    }
}

/*
 * rpc_crystal_format_term_dir
 *
 * Given an actual or formal terminal and a property,
 * write out the direction data.
 */
rpc_crystal_format_term_dir( oTerm, oProp)
octObject *oTerm, *oProp;
{
    char buf[ 512 ];

    if( oProp == NIL(octObject)){
	sprintf( buf, "has no inherited direction properties");
    } else {
	sprintf( buf, "has inherited direction property \"%s\"",
		oProp->contents.prop.value.string);
    }
    sprintf( CDbuf, "     %s terminal %s %s",
		octIdIsNull(oTerm->contents.term.instanceId) ?
			"Formal" : "Actual",
		oTerm->contents.term.name,
		buf);
    echoCmd( CDbuf);
}

/*
 * rpc_crystal_add_formal_dir
 *
 * Given a formal terminal and a DIRECTION property, try to add
 * the prop onto the same terminal of the interface facet of the facet.
 * Based on the limitations of RPC and vem, the only way this could be
 * a formal terminal is if its a formal of the top level facet.
 */
char *
rpc_crystal_add_formal_dir( oTerm, fctId, oProp)
octObject *oTerm;
octId fctId;
octObject *oProp;
{
    octObject oFacet, oIntFacet, oEProp;
    octStatus sFacet;

    oFacet.objectId = fctId;

    if( octGetById( &oFacet) != OCT_OK){
	sprintf( CDbuf, "Couldn't get spot facet by id: %s",
		octErrorString() );
	return( CDbuf);
    }

    oIntFacet.type = OCT_FACET;
    oIntFacet.contents.facet.cell = oFacet.contents.facet.cell;
    oIntFacet.contents.facet.view = oFacet.contents.facet.view;
    oIntFacet.contents.facet.facet = "interface";
    oIntFacet.contents.facet.version = "";
    oIntFacet.contents.facet.mode = "a";

    if((( sFacet = octOpenFacet( &oIntFacet)) != OCT_OLD_FACET)
	&& ( sFacet != OCT_ALREADY_OPEN)){
	if( sFacet == OCT_NEW_FACET){
	    if( !rpc_crystal_open_is_ok( &oIntFacet)){
		if( octFreeFacet( &oIntFacet) != OCT_OK){
		    sprintf( CDbuf, "Couldn't free %s: %s",
			ohFormatName( &oIntFacet),
			octErrorString() );
		}
	    }
	} else if( sFacet == OCT_NO_PERM){
	    sprintf( CDbuf, "Couldn't open %s: %s",
		ohFormatName( &oIntFacet),
		"permission denied" );
	    return( CDbuf);
	} else {
	    sprintf( CDbuf, "Couldn't open interface of %s: %s",
		ohFormatName( &oFacet),
		octErrorString() );
	    return( CDbuf);
	}
    }

    /* to be here means we've got an interface facet			*/
    if( octGetByName( &oIntFacet, oTerm) != OCT_OK){
	sprintf( CDbuf, "Couln't get fterm %s from %s: %s",
		oTerm->contents.term.name,
		octErrorString() );
	return( CDbuf);
    }

    if( ohGetByPropName( oTerm, &oEProp, "DIRECTION") == OCT_OK){
	if( octDelete( &oEProp) != OCT_OK){
	    sprintf( CDbuf, "%s: %s",
		"Couldn't delete existing \"DIRECTION\" prop",
		octErrorString() );
	}
    }
    if( octCreate( oTerm, oProp) != OCT_OK){
	sprintf( CDbuf, "couldn't create \"%s\" prop \"%s\"  %s %s: %s",
		oProp->contents.prop.name,
		oProp->contents.prop.value.string,
		"attached to term",
		oTerm->contents.term.name,
		octErrorString() );
	return( CDbuf);
    }
    return( NIL(char));
}

/*
 * rpc_crystal_open_is_ok
 *
 * Confirm from the user that its OK to open the new facet.
 */
rpc_crystal_open_is_ok( oFacet)
octObject *oFacet;
{
    vemStatus ret;

    sprintf( CDbuf, "Is it OK to open %s:%s:%s in order\nto add the desired \"DIRECTION\" property?",
		oFacet->contents.facet.cell,
		oFacet->contents.facet.view,
		oFacet->contents.facet.facet );

    ret = dmConfirm( "Open Facet", CDbuf, "Do It", "Don't Do It");
    return( ret == VEM_OK);
}

/*
 * rpc_crystal_add_actual_dir
 *
 * Given an actual terminal, an instance, and a prop, add the prop to
 * the aterm.
 * The instance may someday be used to map from oct to crystal and
 * do some data structure manipulation (so there is no required (re)build).
 */
/*ARGSUSED*/
char *
rpc_crystal_add_actual_dir( oTerm, oInst, oProp)
octObject *oTerm, *oInst, *oProp;
{
    octObject oEProp;

    if( ohGetByPropName( oTerm, &oEProp, "DIRECTION") == OCT_OK){
	if( octDelete( &oEProp) != OCT_OK){
	    sprintf( CDbuf, "%s: %s",
		"Couldn't delete existing \"DIRECTION\" prop",
		octErrorString() );
	}
    }
    if( octCreate( oTerm, oProp) != OCT_OK){
	sprintf( CDbuf, "couldn't create \"%s\" prop \"%s\"  %s %s: %s",
		oProp->contents.prop.name,
		oProp->contents.prop.value.string,
		"attached to term",
		oTerm->contents.term.name,
		octErrorString() );
	return( CDbuf);
    }
    return( NIL(char));
}
