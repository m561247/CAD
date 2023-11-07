
/***********************************************************************
 * 
 * Filename: yylex.c
 * Program: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Design: Russell Segal
 * Organization:  University Of California, Berkeley
 *                Electronic Research Laboratory
 *                Berkeley, CA 94720
 * Routines:
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define YYLEX_MUSA
#include "musa.h"

#define GETC()	    (npushback != 0 ? pushback[--npushback] : getc(lexin))
#define UNGETC(c)   (pushback[npushback++] = c);


char	*yyfile;			/* name of file (or name of macro) */
int		yyline;				/* current line number */
int		npushback;			/* number of characters pushed back */
char	pushback[10];		/* characters pushed back on this stream */
char	yytext[1024];
FILE	*lexin;				/* file pointer */


/*************************************************************************
 * READ_STRING
 * read text characters up to a specified terminator 
 */
int read_string(word, terminator)
	char	*word;
	int		terminator;		/* character to terminate on */
{
	register int i = 0, c;

	while ((c = GETC()) != EOF && (c != terminator)) {
		word[i++] = c;
	} /* while... */
	word[i] = '\0';
	return i;
} /* read_string... */

/*************************************************************************
 * READ_TOKEN
 * read characters until white space 
 */
int read_token(word, first_char)
	register char *word;
	register int first_char;
{
	register int i = 0, c;

	word[i++] = first_char;
	while (((c = GETC()) != EOF) && !isspace(c)) {
		word[i++] = c;
	} /* while... */
	word[i] = '\0';
	UNGETC(c);
	return i;
} /* read_token... */

/**************************************************************************
 * YYLEX
 */
char *yylex()
{
	int yylength;
	register int c;

start:
	c = GETC();

	/* Check for end of file, keep returning EOF until yacc satisfied */
	if (c == EOF) {
		yyclosefile();
		return NIL(char);
	} else if (c == '\n') {
		/* stall on end of line */
		UNGETC(c);
		return "\n";
	} else if (isspace(c)) {
		/* skip over white space */
		goto start;
	/* check for quoted name */
	} else if (c == '"') {
		yylength = read_string(yytext, '"');
	} else {
		yylength = read_token(yytext, c);
	} /* if... */
	return yytext;
} /* yylex... */

/**********************************************************************
 * YYLF
 * skip to next line of input; return 0 on EOF
 */
yylf()
{
	register int c;

	while (((c = GETC()) != EOF) && (c != '\n')) {}
	yyline++;
} /* yylf... */

/************************************************************************
 * YYERROR
 */
yyerror(s)
char *s;
{
    (void) fprintf(stderr, "%s in file %s at line %d\n", s, yyfile, yyline);
    VOVend(-1);
} 

/************************************************************************
 * YYOPENFILE
 * return 1 if sucessful
 */
int yyopenfile(s, use_stdin)
	char *s;				/* name of the file */
	int use_stdin;			/* if 1, use standard input */
{
    yyfile = util_strsav(s);
    yyline = 1;
    npushback = 0;

    if (use_stdin) {
	lexin = stdin;
    } else {
	if ((lexin = fopen(s, "r")) == NIL(FILE)) {
	    (void) fprintf(stderr, "%s: Unable to open \"%s\"\n",progName,s);
	    return 0;
	}			/* if... */
    }				/* if... */
    return 1;
}				/* yyopenfile... */

/************************************************************************
 * YYCLOSEFILE
 */
yyclosefile()
{
	if (lexin != stdin) {
		(void) fclose(lexin);
	} /* if... */
} /* yyclosefile... */
