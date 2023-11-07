/* hash.c -
 *
 * Copyright (C) 1982 by John K. Ousterhout
 * sccsid "@(#)hash.c	2.1 (Berkeley) 4/9/83"
 *
 * This module contains routines to manipulate a hash table.
 * See hash.h for a definition of the structure of the hash
 * table.  Hash tables grow automatically as the amount of
 * information increases.
 */

#include "port.h"
#include "hash.h"

/* Library routines: */

/* The following defines the ratio of # entries to # buckets
 * at which we rebuild the table to make it larger.
 */

static rebuildLimit = 3;

#define NIL ((HashEntry *) (1<<29))


HashInit(table, nBuckets)
register HashTable *table;
int nBuckets;			/* How many buckets to create for starters.
				 * This number is rounded up to a power of
				 * two.
				 */

/*---------------------------------------------------------
 *	This routine just sets up the hash table.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Memory is allocated for the initial bucket area.
 *---------------------------------------------------------
 */

{
    int i;
    HashEntry ** ptr;

    /* Round up the size to a power of two. */

    if (nBuckets < 0) nBuckets = -nBuckets;
    table->ht_nEntries = 0;
    table->ht_size = 2;
    table->ht_mask = 1;
    table->ht_downShift = 29;
    while (table->ht_size < nBuckets)
    {
	table->ht_size <<= 1;
	table->ht_mask = (table->ht_mask<<1) + 1;
	table->ht_downShift -= 1;
    }

    table->ht_table = (HashEntry **) malloc((unsigned) 4*table->ht_size);
    ptr = table->ht_table;
    for (i=0; i<table->ht_size; i++) *ptr++ = NIL;
}


int
hash(table, string)
register HashTable *table;
register char *string;

/*---------------------------------------------------------
 *	This is a local procedure to compute a hash table
 *	bucket address based on a string value.
 *
 *	Results:
 *	The return value is an integer between 0 and size-1.
 *
 *	Side Effects:	None.
 *
 *	Design:
 *	It is expected that most strings are decimal numbers,
 *	so the algorithm behaves accordingly.  The randomizing
 *	code is stolen straight from the rand library routine.
 *---------------------------------------------------------
 */

{
    int i = 0;

    while (*string != 0) i = (i*10) + (*string++ - '0');
    return ((i*1103515245 + 12345) >> table->ht_downShift) & table->ht_mask;
}


HashEntry *
HashFind(table, string)
register HashTable *table;	/* Hash table to search. */
char *string;			/* String whose entry is to be found. */

/*---------------------------------------------------------
 *	HashFind searches a hash table for an entry corresponding
 *	to string.  If no entry is found, then one is created.
 *
 *	Results:
 *	The return value is a pointer to the entry for string.
 *	If the entry is a new one, then the pointer field is
 *	zero.
 *
 *	Side Effects:
 *	Memory is allocated, and the hash buckets may be modified.
 *---------------------------------------------------------
 */

{
    register HashEntry *h;
    int bucket;

    bucket = hash(table, string);
    h = *(table->ht_table + bucket);
    while (h != NIL)
    {
	if (strcmp(h->h_name, string) == 0) return h;
	h = h->h_next;
    }

    /* The desired entry isn't there.  Before allocating a new entry,
     * see if we're overloading the buckets.  If so, then make a
     * bigger table.
     */

    if (table->ht_nEntries >= rebuildLimit*table->ht_size)
    {
	rebuild(table);
	bucket = hash(table, string);
    }
    table->ht_nEntries += 1;

    /* Not there, we have to allocate.  If the string is longer
     * than 3 bytes, then we have to allocate extra space in the
     * entry.
     */

    h = (HashEntry *) malloc((unsigned) (sizeof(HashEntry) +
	strlen(string) -3));
    (void) strcpy(h->h_name, string);
    h->h_pointer = 0;
    h->h_next = *(table->ht_table + bucket);
    *(table->ht_table + bucket) = h;
    return h;
}


rebuild(table)
HashTable *table;		/* Table to be enlarged. */

/*---------------------------------------------------------
 *	The rebuild routine makes a new hash table that
 *	is larger than the old one.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The entire hash table is moved, so any bucket numbers
 *	from the old table are invalid.
 *---------------------------------------------------------
 */

{
    int oldSize, bucket;
    HashEntry **oldTable, **old2, *h, *next;

    oldTable = table->ht_table;
    old2 = oldTable;
    oldSize = table->ht_size;

    HashInit(table, table->ht_size*4);	/* Quadruple size of table. */
    for ( ; oldSize>0; oldSize--)
    {
	h = *old2++;
	while (h != NIL)
	{
	    next = h->h_next;
	    bucket = hash(table, h->h_name);
	    h->h_next = *(table->ht_table + bucket);
	    *(table->ht_table + bucket) = h;
	    table->ht_nEntries += 1;
	    h = next;
	}
    }

    free((char *) oldTable);
}


HashStats(table)
HashTable *table;

/*---------------------------------------------------------
 *	This routine merely prints statistics about the
 *	current bucket situation.
 *
 *	Results:	None.
 *
 *	Side Effects:	Junk gets printed.
 *---------------------------------------------------------
 */

{
    int count[10], overflow, i, j;
    HashEntry *h;

    for (i=0; i<10; i++) count[i] = 0;
    overflow = 0;
    for (i=0; i<table->ht_size; i++)
    {
	j = 0;
	h = *(table->ht_table+i);
	while (h != NIL)
	{
	    j += 1;
	    h = h->h_next;
	}
	if (j<10) count[j] += 1;
	else overflow += 1;
    }

    for (i=0;  i<10; i++)
    {
	printf("Number of buckets with %d entries: %d.\n", i, count[i]);
    }
    printf("Number of buckets with >9 entries: %d.\n", overflow);
}


HashStartSearch(hs)
HashSearch *hs;			/* Area in which to keep state about search.*/

/*---------------------------------------------------------
 *	This procedure sets things up for a complete search
 *	of all entries recorded in the hash table.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The information in hs is initialized so that successive
 *	calls to HashNext will return successive HashEntry's
 *	from the table.
 *---------------------------------------------------------
 */

{
    hs->hs_nextIndex = 0;
    hs->hs_h = NIL;
}

HashEntry *
HashNext(table, hs)
HashTable *table;		/* Table to be searched. */
HashSearch *hs;			/* Area used to keep state about search. */

/*---------------------------------------------------------
 *	This procedure returns successive entries in the
 *	hash table.
 *
 *	Results:
 *	The return value is a pointer to the next HashEntry
 *	in the table, or NULL when the end of the table is
 *	reached.
 *
 *	Side Effects:
 *	The information in hs is modified to advance to the
 *	next entry.
 *---------------------------------------------------------
 */

{
    HashEntry *h;

    while (hs->hs_h == NIL)
    {
	if (hs->hs_nextIndex >= table->ht_size) return NULL;
	hs->hs_h = *(table->ht_table + hs->hs_nextIndex);
	hs->hs_nextIndex += 1;
    }
    h = hs->hs_h;
    hs->hs_h = h->h_next;
    return h;
}


HashKill(table)
register HashTable *table;	/* Hash table whose space is to be freed. */

/*---------------------------------------------------------
 *	This routine removes everything from a hash table
 *	and frees up the memory space it occupied.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Lots of memory is freed up.
 *---------------------------------------------------------
 */

{
    HashEntry *h;
    int i;

    for (i=0;  i<table->ht_size;  i++)
    {
	h = *(table->ht_table + i);
	while (h != NIL)
	{
	    free((char *) h);
	    h = h->h_next;
	}
    }

    free((char *) table->ht_table);

    /* Set up the hash table to cause memory faults on any future
     * access attempts until re-initialization.
     */

    table->ht_table = (HashEntry **) (1<<29);
}
