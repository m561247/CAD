/* hash.h -
 *
 * Copyright (C) 1982 by John K. Ousterhout
 * sccsid "@(#)hash.h	2.1 (Berkeley) 4/9/83"
 *
 * This file contains definitions used by the hash module,
 * which maintains hash tables.
 */

/* The following defines one entry in the hash table. */

typedef struct h1 {
    char *h_pointer;		/* Pointer to anything. */
    struct h1 *h_next;		/* Next element, zero for end. */
    char h_name[4];		/* Text name of this entry.  Note: the
				 * actual size may be longer if necessary
				 * to hold the whole string. This MUST be
				 * the last entry in the structure!!!
				 */
    } HashEntry;

/* A hash table consists of an array of pointers to hash
 * lists:
 */

typedef struct h3 {
    HashEntry **ht_table;	/* Pointer to array of pointers. */
    int ht_size;		/* Actual size of array. */
    int ht_nEntries;		/* Number of entries in the table. */
    int ht_downShift;		/* Shift count, used in hashing function. */
    int ht_mask;		/* Used to select bits for hashing. */
    } HashTable;

extern HashInit(), HashStats();
extern HashEntry *HashFind();

/* char * HashGetValue(h); HashEntry *h; */

#define HashGetValue(h) ((h)->h_pointer)

/* HashSetValue(h, val); HashEntry *h; char *val; */

#define HashSetValue(h, val) ((h)->h_pointer = (char *) (val))

/* The following structure is used by the searching routines
 * to record where we are in the search.
 */

typedef struct h2 {
    int hs_nextIndex;		/* Next bucket to check (after current). */
    HashEntry * hs_h;		/* Next entry to check in current bucket. */
    } HashSearch;

extern HashStartSearch();
extern HashEntry *HashNext();
