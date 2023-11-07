/*
 * interactiveDM.c
 *
 * Emulate the itemFunct of the vem side dmWhichOne().
 */

#include "port.h"
#include "rpc.h"

idm_dummy_funct(){}

/*
 * idm_which_one
 *
 * Emulate the interactive qualities of the vem side dmWhichOne
 */
vemStatus
idm_which_one( title, count, items, itemSelect, itemFunc, help_buf)
char *title;
int count;
dmWhichItem *items;
int *itemSelect;
int (*itemFunc)();
char *help_buf;
{
    vemStatus ret;
    int sel = -1, lastsel = -1, i;

    /* clear all the flags						*/
    for( i = 0; i < count; ++i){
	items[i].flag = 0;
    }

    do{
	ret = dmWhichOne( title, count, items, &sel, idm_dummy_funct, help_buf);
	if(( ret == VEM_OK) && (sel != -1) && (itemFunc != (int (*)()) 0)){
	    itemFunc( &( items[ sel ]));
	    if( lastsel != -1){
		items[ lastsel].flag = 0;
	    }
	    items[ sel].flag = 1;
	    lastsel = sel;
	}
    } while(( ret == VEM_OK) || (sel != -1));

    *itemSelect = sel;
    return( ret);
}
