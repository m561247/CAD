/*
 * octCap.c
 *
 * Utilities used in estimating capacitances in symbolic oct facets
 */

#include "cOct.h"

/*
 * cOctFindBBLayers
 *
 * Find the bounding oBox of the actual terminal and find all layers
 * that it is implemented on.
 */
cOctFindBBLayers(oTerm, tBB, layerBits)
octObject *oTerm;
struct octBox *tBB;
int *layerBits;
{
    octObject oInst, oMaster, oFterm, oBox, oLayer;
    octGenerator gBox;
    struct octTransform *transform;
    octCoord t;

    *layerBits = 0;
    if (octIdIsNull(oTerm->contents.term.instanceId)) {
	return;
    }

    /* Get the instance from the terminal				*/
    OH_ASSERT( ohGetById(&oInst, oTerm->contents.term.instanceId));

    /* open the interface facet of the master of the instance		*/
    OH_ASSERT( ohOpenMaster(&oMaster, &oInst, "interface", "r"));

    /* Find formal terminal (which matches this actual terminal)	*/
    OH_ASSERT( ohGetByTermName(&oMaster, &oFterm, oTerm->contents.term.name));

    /* Get the first 'terminal frame' (which must be a box)		*/
    if(octGenFirstContent(&oFterm, OCT_BOX_MASK, &oBox) != OCT_OK){
	/*** message here ? */
	return;
    }
    *tBB = oBox.contents.box;

    /* Transform the box into the instances coordinates			*/
    transform = &oInst.contents.instance.transform;
    tr_oct_transform(transform, &(tBB->lowerLeft.x), &(tBB->lowerLeft.y));
    tr_oct_transform(transform, &(tBB->upperRight.x), &(tBB->upperRight.y));
    if (tBB->lowerLeft.x > tBB->upperRight.x) {
	t = tBB->lowerLeft.x;
	tBB->lowerLeft.x = tBB->upperRight.x;
	tBB->upperRight.x = t;
    }
    if (tBB->lowerLeft.y > tBB->upperRight.y) {
	t = tBB->lowerLeft.y;
	tBB->lowerLeft.y = tBB->upperRight.y;
	tBB->upperRight.y = t;
    }

    /* find the layers that contain the box.  Layer names are coded in here
     * because crystal is already has such technology dependancies, so
     * doing something more clever wouldn't be worth the effort.
     */
    OH_ASSERT( octInitGenContents( &oFterm, OCT_BOX_MASK, &gBox));
    while( octGenerate( &gBox, &oLayer) == OCT_OK){
	if( octGenFirstContainer(&oBox, OCT_LAYER_MASK, &oLayer) == OCT_OK){
	    if( !strcmp( "NDIF", oLayer.contents.layer.name)){
		*layerBits |= COCT_NDIF_MASK;
	    } else if( !strcmp( "PDIF", oLayer.contents.layer.name)){
		*layerBits |= COCT_PDIF_MASK;
	    } else if( !strcmp( "POLY", oLayer.contents.layer.name)){
		*layerBits |= COCT_POLY_MASK;
	    } else if( !strcmp( "MET1", oLayer.contents.layer.name)){
		*layerBits |= COCT_MET1_MASK;
	    } else if( !strcmp( "MET2", oLayer.contents.layer.name)){
		*layerBits |= COCT_MET2_MASK;
	    }
	}
    }

}
