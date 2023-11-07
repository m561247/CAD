typedef struct macro_struct macdef;
struct macro_struct {
	char	*name;
	char	*def;		/* definition */
	macdef	*next;
}; /* macro_struct... */


typedef struct smplace_struct smplace;      /* place in ma or sr */
struct smplace_struct {
    int	is_macro;				/* if false then source */
    int	interactive;			/* break has occured */
    union {
	struct {
	    char	*place;			/* place in macro */
	    char	**params;
	    int		numparams;
	} ma;
	struct {
	    FILE	*file;			/* source file */
	    int		stop;			/* conditions for break */
	} sr;
    } u;
}; /* smplace_struct... */ 



extern smplace 		smstack[];
extern int		smlevel;        /* current place in smstack */
extern int		last_source;    /* most recent sr in stack */


extern int simStackPush();
extern void simStackPushFile( /* fp */ );
extern void simStackPushMacro( /* m, argc, argv */ );
extern void simStackPop( );
extern void simStackReset( /* interactive */ );
extern int simStackGetMacroLine( /* line */ );
extern FILE* simStackFile();
extern int simStackInteractive();
extern int simStackMacro();



