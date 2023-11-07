#include "port.h"
#include "utility.h"
#include "rpc.h"
#include "oh.h"
#include "table.h"
#include "defs.h"

extern char* getPropValue();





int stepsMultiText( container, title, table )
    octObject *container;
    char* title;
    tableEntry *table;
{
    int size = tableSize( table );
    int i, rtn;
    octObject master;
    octObject *props = ALLOC( octObject, size );
    dmTextItem *items = ALLOC(dmTextItem, size );

    for( i = 0; i < size ; i++ ) {
	items[i].itemPrompt = table[i].desc;
	items[i].cols =       table[i].width;
	items[i].rows =       1;
	items[i].value =      0;
	items[i].userData = 0;

	props[i].type = OCT_PROP;
	props[i].contents.prop.type = table[i].type;
	props[i].contents.prop.name = table[i].propName;
	
	if ( getInheritedProp( container, &props[i] ) != OCT_OK ) {
	    setPropValue( &props[i], table[i].defaultValue );
	    octCreate( container, &props[i] );
	}
	items[i].value = util_strsav( getPropValue( &props[i] ) );
    }
    

    rtn =  dmMultiText( title, size, items );
	
    if (rtn == VEM_OK ) {
	/* First update all vars, then dive into sub tables */
	for ( i = 0; i < size ; i++ ) {
	    setPropValue( &props[i], items[i].value );
	    octModify( &props[i] );
	}
	for ( i = 0; i < size ; i++ ) {
	    if ( table[i].table ) {
		if ( doSubtable(&props[i])  ) {
		    stepsMultiText(container, table[i].desc, table[i].table );
		} 
	    }
	    /* FREE( items[i].value ); */ /* Not good under RPC */
	}
    }
    FREE(props);
    FREE(items);

    return rtn;;
}


int stepsMultiWhich( container, title, table , help_str)
    octObject *container;
    char* title;
    tableEntry *table;
    char* help_str;
{
    int size = tableSize( table );
    int i, rtn;
    octObject *props = ALLOC( octObject, size );
    dmWhichItem *items = ALLOC(dmWhichItem, size );

    for( i = 0; i < size ; i++ ) {
	items[i].itemName = table[i].desc;
	items[i].flag =       1;
	items[i].userData = 0;

	props[i].contents.prop.type = table[i].type;
	if (ohGetByPropName( container, &props[i], table[i].propName ) != OCT_OK ) {
	    setPropValue( &props[i], table[i].defaultValue );
	    octCreate( container, &props[i] );
	}
	items[i].flag  =  props[i].contents.prop.value.integer;
    }
    

    rtn =  dmMultiWhich( title, size, items, 0, help_str );
	
    for ( i = 0; i < size ; i++ ) {
	if ( rtn == VEM_OK ) {
	    props[i].contents.prop.value.integer = items[i].flag;
	    octModify( &props[i] );
	}
    }
    FREE(props);
    FREE(items);

    return rtn;;
}

