/*
 * octStHelper.c
 *
 * Utilities to interface to the st package.
 */

#include "cOct.h"

/*
 * sthelp_findkey
 *
 * Find the key in table associated with value.
 * Return the key if found, else return (char *) 0.
 */
char *
sthelp_findkey(table, targetValue)
st_table *table;
char *targetValue;
{
    st_generator *stGen;
    char *value, *key;

    if(( stGen = st_init_gen( table)) == NIL(st_generator)){
	return( NIL(char));
    }

    while( st_gen( stGen, &key, &value) ){
	if( value == targetValue){
	    st_free_gen( stGen);
	    return( key );
	}
    }
    st_free_gen( stGen);
    return( NIL(char));
}
