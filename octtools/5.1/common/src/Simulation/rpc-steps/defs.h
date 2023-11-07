#define isize(iw)	(sizeof(iw)/sizeof(dmWhichItem))
#define tsize(tw)	(sizeof(tw)/sizeof(dmTextItem))


#define PPP(v) util_pretty_print( v )

#define PROPEQ(p,s) (!strcmp( (p)->contents.prop.value.string, (s)))

extern int stepsMultiText
  ARGS(( octObject *container, char* title, tableEntry *table ));


extern char* getPropValue();
extern char* getStepsOption();
extern double getStepsOptionReal();

extern octStatus getInheritedProp();

extern char* getGround();

extern char* nodeNumber
  ARGS( (octObject *net ));

extern char* processInstTerm
  ARGS( (octObject *inst, char*, octObject*, char* ));

extern renumberNodes
  ARGS((octObject *facet));


extern FILE* stepsFopen();

extern void stepsErrHandler();
#ifdef ERRTRAP_H
extern jmp_buf stepsJmp;
#endif
