/* expand.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)expand.c	2.2 (Berkeley) 10/28/83"
 *
 * This file provides name expansion by replacing a pattern
 * with successive integers.  For example, it will turn
 * Bus<15:0> into Bus15, Bus14, ... Bus0.  It will also
 * search every node for a particular pattern in a name.
 */

#include "crystal.h"
#include "hash.h"

/* Library imports: */

#define LENGTH 200
char prefix[LENGTH+1];		/* Holds stuff before number. */
char postfix[LENGTH+1];		/* Holds stuff after number. */
int next = -2;			/* Remaining range of variable.  If next */
int last;			/* is -1, then no number substitution is
				 * to be done.  If next is -2, then we're
				 * already done.
				 */
int hashSearchOn = FALSE;	/* TRUE if in middle of exhaustive search. */
HashSearch hs;			/* Used for exhaustive searches. */


ExpandInit(string)
char *string;			/* Pattern string. */

/*---------------------------------------------------------
 *	This routine accepts the pattern, and sets up
 *	for later calls to ExpandNext.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Internal variables are set up.  The pattern string
 *	may have any of three forms:
 *	1. "*x":  in this case we'll return ALL nodes in
 *	the database that contain the substring x.  This
 *	may be slow since it involves a complete search.
 *	2. "a<x:y>b":  in this case we'll generate several
 *	strings, of the form "acb" where c gets successive
 *	numerical values between x and y, inclusive.  Each
 *	string will be looked up in the hash table.  This
 *	is relatively fast.  Use backslash as a quote character
 *	to get < and > characters in the string without special
 *	interpretation.
 *	3. Strings not in the above categories:  they are
 *	just looked up as they are.
 *
 *	There may also be combinations of "*" and "<>" forms.
 *---------------------------------------------------------
 */

{
    register char *pattern, *p;

    /* Don't permit overlong strings. */

    if (strlen(string) > LENGTH)
    {
	next = -2;
	return;
    }
    hashSearchOn = FALSE;

    /* Copy away the prefix. */

    pattern = string;
    p = prefix;
    postfix[0] = 0;
    while (1)
    {
	if (*pattern == 0)
	{
	    /* There isn't any number substitution to be done. */

	    next = -1;
	    *p = 0;
	    return;
	}

	/* Skip quoted stuff. */

	if (*pattern == '\\')
	{
	    pattern++;
	    if (*pattern != 0) *p++ = *pattern++;
	    continue;
	}

	if (*pattern == '<') break;
	*p++ = *pattern++;
    }

    *p = 0;
    if (sscanf(pattern, "<%d:%d>", &next, &last) != 2)
    {
	printf("Bad pattern \"%s\", must be of form \"a<x:y>b\"\n", string);
	next = -2;
    }
    if ((next < 0) || (last < 0)) next = last = -2;

    /* Scan off the postfix.  For consistency, handle backslash
     * as a quote character.
     */

    while ((*pattern != 0) && (*pattern != '>')) pattern++;
    p = postfix;
    if (*pattern == '>') pattern++;
    while (*pattern != 0)
    {
	if (*pattern == '\\')
	{
	    pattern++;
	    if (*pattern != 0) *p++ = *pattern++;
	    continue;
	}
	*p++ = *pattern++;
    }

    *p = 0;
    return;
}


char *
ExpandNext(table)
HashTable *table;		/* Hash table to use for expansions. */

/*---------------------------------------------------------
 *	This routine returns nodes corresponding to expanded strings.
 *
 *	Results:
 *	The return value is the node corresponding to the next
 *	value of the pattern string.  If all possible values have
 *	already	been returned, then NULL is returned.
 *
 *	Side Effects:
 *	Our internal structure is advanced to the next string.
 *---------------------------------------------------------
 */

{
    HashEntry *h;
    char *value;
    static char name[LENGTH+20];

    while (TRUE)
    {
	if (hashSearchOn)
	    while (TRUE)
	    {
		h = HashNext(table, &hs);
		if (h == NULL) break;
		value = HashGetValue(h);
		if (value == 0) continue;
		if (match(name+1, h->h_name)) return value;
	    }

	if (next == -2) return (char *) NULL;

	if (next == -1)
	{
	    (void) sprintf(name, "%s%s", prefix, postfix);
	    next = -2;
	}
	else
	{
	    (void) sprintf(name, "%s%d%s", prefix, next, postfix);
	    if (next == last) next = -2;
	    else if (next > last) next -= 1;
	    else next += 1;
        }
	if (name[0] != '*')
	{
	    h = HashFind(table, name);
	    value = HashGetValue(h);
	    if (value == (char *) 0)
            {
		printf("%s isn't in table!\n", name);
		continue;
	    }
	    return value;
	}
	else
	{
	    HashStartSearch(&hs);
	    hashSearchOn = TRUE;
	    continue;
	}
    }
}


int
match(s1, s2)
char *s1, *s2;

/*---------------------------------------------------------
 *	This procedure sees if one string is contained in
 *	another.
 *
 *	Results:
 *	TRUE is returned if s1 is a substring of s2.
 *	Otherwise, FALSE is returned.  If s1 is NULL,
 *	it matches everything.  If s2 is NULL, it matches
 *	only NULL.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    char *t1, *t2;

    if (*s1 == 0) return TRUE;
    while (TRUE)
    {
	/* Find a character in s2 that matches the first character of s1. */

	while (*s1 != *s2) if (*s2++ == 0) return FALSE;

	/* See if the rest of s1 matches successive characters in s2. */

	t2 = ++s2;
	t1 = s1;

	while (TRUE)
	{
	    if (*++s1 == 0) return TRUE;
	    if (*s1 != *s2++) break;
	}
	
	s1 = t1;
	s2 = t2;
    }
}
