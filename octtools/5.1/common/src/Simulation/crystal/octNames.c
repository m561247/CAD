/*
 * octNames.c
 */

#include "crystal.h"
#include "hash.h"
#include "cOct.h"

extern char *VddName, *GroundName;

/* imported from other crystal files					*/
extern Node *VddNode, *GroundNode, *BuildNode();
extern Type TypeTable[];
extern HashTable NodeTable, FlowTable;

/* This is pretty ugly, but compatible with hash.c			*/
#define HASH_ENTRY_NIL ((HashEntry *) (1<<29))

/* globals used in this module						*/
st_table *CrNodeNames;
extern st_table *OctIdTable;
int ParseLevel = 0, InstCount = 0, NameCount = 0;
char *cOctGetNetName(), *newUniqueName(), *cOctInsertBackSlash();
Node *cOctBuildNode(), *cOctBuildVdd(), *cOctBuildGnd();
s_netHash *newNetHash();

/* property names and values						*/
#define DIRECTION_PROP "DIRECTION"
#define INPUT "INPUT"
#define OUTPUT "OUTPUT"

/*
 * cOctLinkFormals
 *
 * This rountine operates on the CrNodeNames hash table.
 * It creates a mapping from the nets that contain the formal terms
 * in the definition facet ("contents" or SIM_FACET) of the instance
 * to the nets that contain the actual terms of the instance.
 */
cOctLinkFormals( oDefFacet, oInst)
octObject *oDefFacet, *oInst;
{
    octObject oIterm, oFterm, oInet, oFnet;
    octGenerator gIterm;
    s_netHash *nh, *ph; /* new and parent hash values			*/
    int type;

    OH_ASSERT( octInitGenContents( oInst, OCT_TERM_MASK, &gIterm ));
    while( octGenerate( &gIterm, &oIterm ) == OCT_OK ) {
	
	/* get coresponding formal term in oFacet			*/
	OH_ASSERT( ohGetByTermName( oDefFacet, &oFterm,
					oIterm.contents.term.name));

	/* get both nets						*/
	if( octGenFirstContainer( &oIterm, OCT_NET_MASK, &oInet) != OCT_OK){
	    /* actual term is not attached to net in oDefFacet		*/
	    continue;
	}

	nh = newNetHash();
	nh->name = cOctGetNetName( &oInet);
	nh->parentId = oInet.objectId;

	if( st_lookup(CrNodeNames, (char *)oInet.objectId, (char **)&ph) == 0){
	    /* should never hapen					*/
	    errRaise( COCT_PKG_NAME, -1, "st package internal error detected");
	}

	nh->netType = ph->netType;

	if( octGenFirstContainer( &oFterm, OCT_NET_MASK, &oFnet) != OCT_OK){
	    /* formal isn't attached inside oDefFacet			*/
	    /*** warning ?						*/
	    continue;
	}

	type = cOctNetTermType( &oFnet, &oFterm, oDefFacet);

	if(( type != nh->netType) && ( type != 0)){
	    if( nh->netType != 0){
		/* Semanticaly, this is a power-ground short.  A net of type
		 * {power,ground} is connected to a net of type
		 * {ground,power}.  This is considered a fatal error.
		 */
		errRaise( COCT_PKG_NAME, -1,
		    "%s %s %s %s %s %s %s %s %s %s %s: %s",
		    ohFormatName( &oInet),
		    "is a",
		    (nh->netType == VDD_NET)?"SUPPLY":"GROUND",
		    "net, but is connected to",
		    ohFormatName( &oFnet),
		    "in",
		    ohFormatName( oDefFacet),
		    "which is a",
		    (type == VDD_NET)?"SUPPLY net.":"GROUND net.",
		    "This occures at",
		    ohFormatName( oInst),
		    "This power-to-ground short is a fatal error" );

	    } else {
		/* Even though this isn't a short, its still a big time error.
		 * The problems is that the parser may have
		 * aready used the node in the crystal data structure.  This
		 * means that the type of the parent node cannot change (because
		 * crystal has already stored the ADDRESS of the node struct,
		 * and to make a node Vdd or Ground, you must use the ADDRESS
		 * of the VddNode or GroundNode).  Thus, this is a pretty big
		 * problem and recovery would involve a complete traversal
		 * of the crystal data structure built up to this point.
		 * Since I'm not doing that, results cannot be guarenteed.
		 */
		errRaise( COCT_PKG_NAME, -1,
		    "%s %s %s %s %s %s %s %s: %s %s: %s",
		    "properties in",
		    ohFormatName( oDefFacet),
		    "change the type of the external-to-instance net",
		    ohFormatName( &oInet),
		    "from",
		    (nh->netType == 0)?"UNDEFINED": (nh->netType == VDD_NET)?
						    "SUPPLY":"GROUND",
		    "to",
		    (type == VDD_NET)?"SUPPLY":"GROUND",
		    "This occurs at",
		    ohFormatName( oInst),
		    "THE RESULTS OF THIS PARSE MAY BE WRONG." );
	    }
	} else {
	    nh->netType = type;
	}

	if( nh->netType != 0){
	    if( nh->netType == VDD_NET){
		if( VddNode == NIL(Node)){
		    /* Forget it, this is too rediculous to recover from */
		    errRaise( COCT_PKG_NAME, -1,"%s %s %s %s %s",
			"Using Supply node internally without",
			"having any external Vdd node",
			ohFormatName( oInst ),
			"at terminal",
			ohFormatName( &oFterm));
		} else {
		    nh->name = VddNode->n_name;
		}
	    } else if( nh->netType == GND_NET){
		if( GroundNode == NIL(Node)){
		    errRaise( COCT_PKG_NAME, -1,"%s %s %s %s %s",
			"Using Ground node internally without",
			"having any external Vdd node",
			ohFormatName( oInst ),
			"at terminal",
			ohFormatName( &oFterm));
		} else {
		    nh->name = GroundNode->n_name;
		}
	    }
	}

	/* make the octId of the net of the formal hash to the same name */
	if( st_insert( CrNodeNames, (char *)oFnet.objectId, (char *)nh) == -1){
	    errRaise( COCT_PKG_NAME, -1,"No more memory for CrNodeNames");
	}

    }
}

/*
 * cOctBuildFormalNodes
 *
 * Set up the nodes for the formal terminals.
 * Generate each formal and its net
 * Checking each pair for:
 * 1) both NETTYPE "SUPPLY" or "GROUND" and TERMTYPE "SUPPLY" or "GROUND".
 *    If both are present, the value of NETTYPE is used.
 *    The net attached to this type of formal is hashed into the crystal
 *    struct as the GroundNode or VddNode (as appropriate). Multiple
 *    nets or formals of either type are supported.
 * 2) "DIRECTION" property of formal.
 *    Those with type INPUT or OUTPUT are flaged in crystals' data structure.
 * For each pair:
 *    Hash the formal or net name into a table with net octIds as the key.
 *    This allows acces by net id later when generating instance actuals.
 *
 * If no Vdd node or Ground node is found, the user is warned and advised.
 * Then the VddNode and/or GroundNode global is assigned whatever
 * VddName or GroundName hashes to.  This is to keep crystal from
 * immediately core dumping.
 */
cOctBuildFormalNodes( oTopFacet)
octObject *oTopFacet;
{
    octObject oTerm, oNet;
    octObject oProp;
    octGenerator gTerm;
    int type; /* store type of NETTYPE or TERMTYPE prop			*/
    Node *n;

    /* generate all the formal terminals and add them to CrNodeNames	*/
    OH_ASSERT( octInitGenContents( oTopFacet, OCT_TERM_MASK, &gTerm ));
    while( octGenerate( &gTerm, &oTerm ) == OCT_OK ) {

	if(
#if OCT_LEVEL == 2
	   ohTerminalNet( &oTerm, &oNet ) != OCT_OK){
#else
	   ohFormalTerminalNet( &oTerm, &oNet ) != OCT_OK){
#endif
	    fprintf( stderr, "%s: Warning: %s %s %s\n",
		COCT_PKG_NAME,
		ohFormatName( &oTerm),
		"not contained by any net in",
		ohFormatName( oTopFacet));
	    continue;
	}

	type = cOctNetTermType( &oNet, &oTerm, oTopFacet);

	if( type & VDD_NET){
	    n = cOctBuildVdd( &oNet, oTerm.contents.term.name);
	} else if( type & GND_NET){
	    n = cOctBuildGnd( &oNet, oTerm.contents.term.name);
	} else {
	    /* not Vdd or Ground. Build now so DIRECTIONS stuff works	*/
	    n = cOctBuildNode( &oNet, oTerm.contents.term.name );
	}

	if( st_insert( OctIdTable, (char *)n, (char *)(oNet.objectId))
			== -1){
	    errRaise( COCT_PKG_NAME, -1,"No more memory for OctIdTable");
	}

	/* check for direction property					*/
	oProp.contents.prop.name = DIRECTION_PROP;
	if( cOctGetIntFacetProp( &oTerm, &oProp, OCT_STRING) == TRUE){
	    if( !strcmp( oProp.contents.prop.value.string, INPUT)){
		printf(": inputs %s\n", cOctGetNetName( &oNet));
		MarkInput( cOctInsertBackSlash( cOctGetNetName( &oNet)));
	    } else if( !strcmp( oProp.contents.prop.value.string, OUTPUT)){
		printf(": outputs %s\n", cOctGetNetName( &oNet));
		MarkOutput( cOctInsertBackSlash( cOctGetNetName( &oNet)));
	    }
	}
    }

    if( GroundNode == NIL(Node)){
	fprintf(stderr, "Crystal Oct reader: %s\n%s\n%s\n%s\"%s\".\n",
	    "Warning: Could not find GROUND net.",
	    "\tAdd the NETTYPE property with value GROUND to one of the nets",
	    "\tor TERMTYPE prop with value GROUND to one of the formals.",
	    "\tGround node defaulting to ",
	     GroundName);
	GroundNode = BuildNode( GroundName );
    }
    if( VddNode == NIL(Node)){
	fprintf(stderr, "Crystal Oct reader: %s\n%s\n%s\n%s\"%s\".\n",
	    "Warning: Could not find SUPPLY net.",
	    "\tAdd the NETTYPE property with value SUPPLY to one of the nets",
	    "\tor TERMTYPE prop with value SUPPLY to one of the formals.",
	    "\tGround node defaulting to ",
	     VddName);
	VddNode = BuildNode( VddName );
    }
	    
    VddNode->n_flags |= NODEISINPUT | NODE1ALWAYS;
    GroundNode->n_flags |= NODEISINPUT | NODE0ALWAYS;
}

/*
 * cOctNetTermType
 *
 * Find out it this is a SUPPLY, GROUND, or neither net/term pair.
 */
cOctNetTermType( oNet, oTerm, oFacet)
octObject *oNet, *oTerm, *oFacet;
{
    int type = 0;
    octObject oNetProp, oTermProp;

    /* locate the Vdd and GND nets, checking the formal and the
     * net for the appropriate properties
     */
    oNetProp.contents.prop.name = "NETTYPE";
    if( cOctGetProp( oNet, &oNetProp, OCT_STRING) == TRUE){
	if ( !strcmp( oNetProp.contents.prop.value.string, "SUPPLY")){
	    type |= VDD_NET;
	} else if(!strcmp( oNetProp.contents.prop.value.string, "GROUND")){
	    type |= GND_NET;
	}
    }
    oTermProp.contents.prop.name = "TERMTYPE";
    if(( cOctGetIntFacetProp( oTerm, &oTermProp, OCT_STRING) == TRUE)
      || (cOctGetProp( oTerm, &oTermProp, OCT_STRING) == TRUE)){
	if ( !strcmp( oTermProp.contents.prop.value.string, "SUPPLY")){
	    if( type & GND_NET){
		cOctFormalPropConflict( oFacet, oTerm, &oTermProp,
					oNet, &oNetProp);
	    } else {
		type |= VDD_NET;
	    }
	} else if(!strcmp( oTermProp.contents.prop.value.string,"GROUND")){
	    if( type & VDD_NET){
		cOctFormalPropConflict( oFacet, oTerm, &oTermProp,
					oNet, &oNetProp);
	    } else {
		type |= GND_NET;
	    }
	}
    }

    return( type );
}


/*
 * cOctBuildNode
 *
 * Add a top-level formal terminal name as a name for this net.
 */
Node *
cOctBuildNode( oNet, name )
octObject *oNet; /* the net to which the term is attached		*/
char *name; /* a name that must hash to the node in crystal		*/
{
    HashEntry *h;
    Node *n;
    s_netHash *nh;

    if( st_lookup( CrNodeNames, (char *)oNet->objectId, (char **)&nh) == 1){
	/* The net is already in the table, so add alias.		*/
	/*** should check for name conficts (prev netName vs term name)	*/
	n = BuildNode( nh->name );
	h = HashFind( &NodeTable, name);
	HashSetValue(h, n);
    } else {
	/* create new node						*/
	if( strcmp( oNet->contents.net.name, "")){
	    /* add both names as names for this node			*/
	    n = BuildNode( oNet->contents.net.name);
	    h = HashFind( &NodeTable, name);
	    HashSetValue(h, n);
	} else {
	    /* add just the one name 					*/
	    n = BuildNode( name);
	}

	nh = newNetHash();
	nh->parentId = oct_null_id;
	nh->name = n->n_name;
	if( st_insert( CrNodeNames, (char *)oNet->objectId, (char *)nh)
			== -1){
	    errRaise( COCT_PKG_NAME, -1,"No more memory for CrNodeNames");
	}
    }
    return (n);
}

/*
 * cOctBuildVdd
 *
 * Add a top-level formal terminal name as a name for the Vdd Node.
 * If it doesn't exist yet, create the VddNode, else add the name
 * as an alias for Vdd.
 */
Node *
cOctBuildVdd( oNet, name )
octObject *oNet; /* the net to which the term is attached		*/
char *name; /* a name that must hash to Vdd in crystal			*/
{
    HashEntry *h;
    s_netHash *nh;

    if( st_lookup( CrNodeNames, (char *)oNet->objectId, NIL(char *)) == 1){
	/* the net is already in the table, so add alias. This guarentees
	 * that the VddNode has already been built, since the net has been
	 * seen before and net props override term props.
	 */
	h = HashFind( &NodeTable, name);
	HashSetValue(h, VddNode);
    } else {
	if( VddNode == NIL(Node)){
	    /* create new Vdd node					*/
	    if( strcmp( oNet->contents.net.name, "")){
		/* add net name as Vdd					*/
		VddNode = BuildNode( oNet->contents.net.name);
		/* also add name if its different from the net's name	*/
		if( strcmp( oNet->contents.net.name, name)){
		    h = HashFind( &NodeTable, name);
		    HashSetValue(h, VddNode);
		}
	    } else {
		/* add just the one name as Vdd				*/
		VddNode = BuildNode( name);
	    }
	} else {
	    /* create the alias to Vdd					*/
	    h = HashFind( &NodeTable, name);
	    HashSetValue(h, VddNode);
	}

	/* make net hash to Vdd name					*/
	nh = newNetHash();
	nh->name = VddNode->n_name;
	nh->netType = VDD_NET;
	nh->parentId = oct_null_id;
	if( st_insert( CrNodeNames, (char *)oNet->objectId, (char *)nh)
			== -1){
	    errRaise( COCT_PKG_NAME, -1,"No more memory for CrNodeNames");
	}
    }
    return ( VddNode);
}

/*
 * cOctBuildGnd
 *
 * Add a top-level formal terminal name as a name for the Gnd Node.
 * If it doesn't exist yet, create the GndNode, else add the name
 * as an alias for Gnd.
 */
Node *
cOctBuildGnd( oNet, name )
octObject *oNet; /* the net to which the term is attached		*/
char *name; /* a name that must hash to Gnd in crystal			*/
{
    HashEntry *h;
    s_netHash *nh;

    if( st_lookup( CrNodeNames, (char *)oNet->objectId, NIL(char *)) == 1){
	/* the net is already in the table, so add alias. This guarentees
	 * that the GroundNode has already been built, since the net has been
	 * seen before and net props override term props.
	 */
	h = HashFind( &NodeTable, name);
	HashSetValue(h, GroundNode);
    } else {
	if( GroundNode == NIL(Node)){
	    /* create new Ground node					*/
	    if( strcmp( oNet->contents.net.name, "")){
		/* add the net's name as Ground				*/
		GroundNode = BuildNode( oNet->contents.net.name);
		/* also add name if its different from the net's name	*/
		if( strcmp( oNet->contents.net.name, name)){
		    h = HashFind( &NodeTable, name);
		    HashSetValue(h, GroundNode);
		}
	    } else {
		/* add just the one name as Ground			*/
		GroundNode = BuildNode( name);
	    }
	} else {
	    /* create the alias to Ground				*/
	    h = HashFind( &NodeTable, name);
	    HashSetValue(h, GroundNode);
	}
	/* make net hash to Ground name					*/
	nh = newNetHash();
	nh->name = GroundNode->n_name;
	nh->netType = GND_NET;
	nh->parentId = oct_null_id;
	if( st_insert( CrNodeNames, (char *)oNet->objectId, (char *)nh)
			== -1){
	    errRaise( COCT_PKG_NAME, -1,"No more memory for CrNodeNames");
	}
    }
    return( GroundNode);
}


/*
 * cOctGetNetName
 *
 * Given an Oct net, this routine returns the crystal name for it.
 * If it is not already in the hash table, it is not a major net, so a
 * name is created "at random".
 */
char *
cOctGetNetName( oNet )
octObject *oNet;
{
    s_netHash *nh;

    if( st_lookup( CrNodeNames, (char *)oNet->objectId, (char **)&nh) == 0){
	/* must create a name for this net				*/
	nh = newNetHash();
	nh->parentId = oct_null_id;
	nh->name = oNet->contents.net.name;
	if(( !strcmp( nh->name, "")) || cOctNotUniqueNodeName( nh->name)){
	    nh->name = newUniqueName();
	}
	if( st_insert( CrNodeNames, (char *)oNet->objectId, (char *)nh) == -1){
	    errRaise( COCT_PKG_NAME, -1,"No more memory for CrNodeNames");
	}
    }

    return( nh->name);
}

/*
 * newUniqueName
 *
 * Create a name that is not already in the crystal hash table.
 * Name is of the form "cr%d_%d_%d" with the %d's being the current level,
 * inst number, and name count.
 */
char *
newUniqueName()
{
    char name[ 100 ];

    do {
	sprintf(name,"cr%d_%d_%d", ParseLevel, InstCount, NameCount);
	++ NameCount;
    } while( cOctNotUniqueNodeName( name));

    return( util_strsav( name ));
}

/*
 * cOctNotUniqueNodeName
 *
 * return TRUE if the name is not unique in the CRYSTAL NODE HASH TABLE,
 * else return false.
 */
cOctNotUniqueNodeName( name )
char *name;
{
    return( cOctNotInCrystalTable( name, &NodeTable));
}

/*
 * cOctNotUniqueFlowName
 *
 * return TRUE if the name is not unique in the CRYSTAL FLOW HASH TABLE,
 * else return false.
 */
cOctNotUniqueFlowName( name )
char *name;
{
    return( cOctNotInCrystalTable( name, &FlowTable));
}

/*
 * cOctNotInCrystalTable
 *
 * See if name is unique in the given Crystal Hash Table.
 */
cOctNotInCrystalTable( key, table)
char *key;
HashTable *table;
{
    HashEntry *h;
    int bucket;

    bucket = hash( table, key);
    h = *(table->ht_table + bucket);
    while (h != HASH_ENTRY_NIL){
	if( strcmp( h->h_name, key) == 0) break;
	h = h->h_next;
    }
    return( h != HASH_ENTRY_NIL);
}

static char *BSRetSt = NIL(char);
static int BSRetSize = 0;
/*
 * cOctInsertBackSlash
 *
 * Insert a back slash before each "<" and ">" character, because in
 * crystal, these are meta-characters.
 */
char *
cOctInsertBackSlash( st )
char *st;
{
    int size, numbs;
    char *c, *bsc;

    size = numbs = 0;
    for( c = st; *c != '\0'; ++c){
	++size;
	if(( *c == '>') || (*c == '<')) ++numbs;
    }

    if( numbs == 0){
	return( st);
    } else {
	if( (size + numbs + 1) > BSRetSize){
	    BSRetSt = REALLOC( char, BSRetSt, size + numbs + 1);
	    BSRetSize = size + numbs + 1;
	}
	for( c = st, bsc = BSRetSt; *c != '\0'; ++c){
	    if(( *c == '<') || ( *c == '>')){
		*bsc = '\\';
		++bsc;
	    }
	    *bsc = *c;
	    ++bsc;
	}
	*bsc = '\0';
	return( BSRetSt);
    }

}


/*
 * cOctFormalPropConflict
 *
 * print warning message about inconsistant term and net props
 */
cOctFormalPropConflict( oDfacet, oTerm, oTermProp, oNet, oNetProp)
octObject *oDfacet, *oTerm, *oTermProp, *oNet, *oNetProp;
{
    fprintf(stderr,"Crystal Oct parser: Warning: %s has %s with %s that conficts with %s of %s (the net that contains the formal). Using the net's property.\n",
	ohFormatName( oDfacet), ohFormatName( oTerm), ohFormatName( oTermProp),
	ohFormatName( oNetProp), ohFormatName( oNet));
}

/*
 * newNetHash
 *
 * malloc a new netHash struct and initialize it to zero.
 */
s_netHash *
newNetHash()
{
    s_netHash *nh;

    if(( nh = ALLOC( s_netHash, 1)) == NIL(s_netHash)){
	errRaise( COCT_PKG_NAME, -1,"No more memory for s_netHash.");
    }
    (void) memset((char *) nh, 0, sizeof(s_netHash));

    return( nh );
}
