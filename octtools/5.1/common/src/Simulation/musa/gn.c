/***********************************************************************
 * 
 * Filename: gn.c
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
 *	node *ElementToNode() - return node associated with given element.
 *	void gnSetDir() - set the current search directory.
 *	int16 gnInstPath() - get the specified instance.
 *	int16 gnCInst() - 'changeinst' command.
 *	int rpcMusaChangeInst() - rpc interface to 'changeinst' command.
 *	int16 gnGetElem() - get the specified element.
 *	void PrintName() - print name in next column.
 *	void PrintName() - print name in next column using fixed column size.
 *	int16 GetElement() - get next element of instance.
 *	void DumpList() - display list.
 *	void gnListNets() - 'listelems' command.
 *	int rpcMusaListElems() - rpc interface to 'listelems' command
 *	void ListLogiX() - list all elements with voltage levels X or x.
 *	int rpcMusaListLogiX() - rpc interface to 'listlogix' command.
 *	void gnListInsts() - 'listinsts' command.
 *	int rpcMusaListInsts() - rpc interface to 'listinsts' command.
 *	void PathNameGen() - generate path name of instance.
 *	void MakePath() - make path name.
 *	void gnPwd() - print current working instance.
 *	int rpcMusaPrintInst() - rpc interface to 'printinst' command.
 *	void MakeElemPath() - generate path name of element.
 *	void MakeConnPath() - generate path name of connection.
 *	MusaElement *NextEquiv() - get next equivalent element.
 *	void gnEquivElems() - 'equivalent' command.
 *	int rpcMusaEquivElems() - rpc interface to 'equivalent' command.
 *	void MakeVect() - define a new vector.
 *	int rpcMusaMakeVector() - rpc interface to 'makevect' command.
 *	char *VectString() - generate string representation of vector.
 *	char *convertIntoBitString() - convert hex to binary string.
 *	int16 ReadFormat() - read format string of 'show' command.
 *	int16 LookupVect() - lookup vector.
 *	void ShowValue() - display value of vector or element.
 *	int rpcMusaShow() - rpc interface to 'show' command.
 *	void SetVect() - set a vector to a voltage level.
 *	int rpcMusaSet() - rpc interface to 'set' command.
 *	void Verify() - verify voltage level for vector or element.
 *	int rpcMusaVerify() - rpc interface to 'verify' command.
 *	void Watch() - 'watch' command.
 *	int rpcMusaWatch() - rpc interface to 'watch' command.
 * Modifications:
 *	Rodney Lai			added reg-ex						02/20/89
 *	Rodney Lai			commented							08/10/89
 ***********************************************************************/

#define GN_MUSA
#include "musa.h"
#include "vov.h"
static musaInstance *searchd;

int isSiblingElement( elem, test )
    MusaElement *elem;
    MusaElement *test;
{
    MusaElement *e[2];
    MusaElement *t[2];
    int i;

    /* traverse the single linked list in starting from each element */
    e[0] = elem;  t[0] = test;	/* First pair */
    e[1] = test;  t[1] = elem;	/* Second pair */
    
    
    for ( i = 0; i < 2 ; i++ ) {
	while (1) {
	    if ( e[i] == t[i] ) return 1;
	    if ( (e[i]->data & NODE_PTR) == 0 && (e[i]->data & EPARENT_PTR) == 0) {
		e[i] = e[i]->equiv.sibling; /* Move on */
	    } else {
		break;		/* End of chain */
	    }
	} 
    }

    return 0;
}
/************************************************************************
 * ELEMENTTONODE
 * Map an element to it's node.
 *
 * Parameters:
 *	elem (MusaElement *) - the element. (Input)
 *
 * Return Value:
 *	The associated node for the given element. (node *)
 */
node* ElementToNode( elem )
	MusaElement *elem;
{
    while ((elem->data & NODE_PTR) == 0) {
	if ((elem->data & EPARENT_PTR) == 0) {
	    elem = elem->equiv.sibling;
	} else {
	    elem = elem->equiv.parent;
	}
    }
    return elem->equiv.node;
}

/**************************************************************************
 * GNSETDIR
 * Set current inst to newdir
 *
 * Parameters:
 *	newdir (musaInstance *) - the new directory to search. (Input)
 */
void gnSetDir(newdir)
    musaInstance *newdir;
{
    searchd = newdir;
} /* gnSetDir... */

/***************************************************************************
 * GNINSTPATH
 * Return the specified instance
 *
 * Parameters:
 *	p (char *) - the name of the instance. (Input)
 *	result (musaInstance *) - the new instance. (Output)
 *	messageFlag (int16) - if TRUE, then print error messages. (Input)
 *	ok (int16 *) - if TRUE, success.
 *
 * Return Value:
 *	TRUE, success.
 *	FALSE, unsuccessful.
 */
int16 gnInstPath(p, result, messageFlag, ok)
    char	*p;
    musaInstance	**result;
    int16	messageFlag;
    int16	*ok;
{
    musaInstance	*inst;
    char	*p2;

    *ok = TRUE;
    if (*p == separator) {
	searchd = topmusaInstance;
	p++;
    }				/* if... */
    while (*p != '\0') {
	if ((p2 = strchr(p, separator)) != NIL(char)) {
	    *p2 = '\0';
	}			/* if... */
	if (strcmp(p, "..") == 0) {
	    /* grab parent of searchd */
	    if (searchd->parent == NIL(musaInstance)) {
		*ok = FALSE;
		Warning("Can't go up from root instance.");
		sim_status |= SIM_NOTFOUND;
		return FALSE;
	    }			/* if... */
	    searchd = searchd->parent;
	} else {
	    if ((searchd->type != DEVTYPE_MODULE) || 
		!InstLookup(searchd, p, &inst)) {
		if ((messageFlag) || (p2 != NIL(char))) {
		    *ok = FALSE;
		    (void) sprintf(errmsg, "%s : No such instance.", p);
		    Warning(errmsg);
		}
		sim_status |= SIM_NOTFOUND;
		return(FALSE);
	    }		
	    searchd = inst;
	}		
	if (p2 == NIL(char)) {
	    *result = searchd;
	    return(TRUE);
	}		
	p = p2+1;
    }			
    *result = searchd;
    return TRUE;
}				/* gnInstPath... */

/**************************************************************************
 * GNCINST
 * Return the specified instance (used as part of changeinst command)
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	result (musaInstance **) - the new instance. (Output)
 *	cwd (musaInstance *) - current working instance. (Input)
 *
 * Return Value:
 *	TRUE, instance found.
 *	FALSE, instance not found.
 */
int16 gnCInst(argc, argv, result, cwd)
	int		argc;
	char	**argv;
	musaInstance	**result;
	musaInstance	*cwd;
{
	int16	junk;

#ifdef RPC_MUSA
	(void) ignore((char *) &argc);
#else
    if (argc < 1) {
		Usage("changeinst inst_path\n");
		sim_status |= SIM_USAGE;
		return(FALSE);
	} /* if... */
#endif
	gnSetDir(cwd);
	return(gnInstPath(argv[0], result, TRUE, &junk));
} /* gnCInst... */

#ifdef RPC_MUSA
/**************************************************************************
 * RPCMUSACHANGEINST
 * Rpc version of 'changeinst' command.  User move pointer over instance
 * to enter and then invokes command.  Program will then open a window for
 * the instance.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaChangeInst(spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	octObject	instance,facet;
	Window		theWin;
	musaInstance		*newdir;
	int16		junk;
	wnOpts		options;
	vemInfo		*info;

	(void) ignore((char *) &cmdList);

	if ((lsLength(cmdList) != 0) ||
		(vuFindSpot(spot,&instance,OCT_INSTANCE_MASK) != VEM_OK)) {
		Format("musa-changeinst",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
	OH_ASSERT(ohOpenMaster(&facet,&instance,"contents","r"));
	VOVinputFacet( &facet );
	theWin = vemOpenWindow(&facet,NIL(char));
	(void) vemWnGetOptions(theWin,&options);
	options.disp_options |= VEM_SHOWRPC;
	(void) vemWnSetOptions(theWin,&options);
	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	gnSetDir(cwd);
	(void) gnInstPath(ohGetName(&instance), &newdir, TRUE, &junk);
	(void) XSaveContext(theDisp,theWin+2,dirTable,(caddr_t) newdir);
	if (st_is_member(winTable,(char *) &facet.objectId)) {
		Warning("Facet opened twice.  There will be inconsistencies in the 'musa-showlevels' function.");
	} else {
		octId *id = ALLOC(octId, 1);
		info = ALLOC(vemInfo,1);
		info->spot = ALLOC(RPCSpot,1);
		info->spot->theWin = theWin+2;
		info->spot->facet = facet.objectId;
		info->spot->thePoint.x = 0;
		info->spot->thePoint.y = 0;
		info->selSet = rpcMusaSetSelSet (facet.objectId);
		*id = facet.objectId;
		st_add_direct(winTable,(char *) id, (char *) info);
	} /* if... */
	rpcMusaUpdateFacetSelSet(facet.objectId);
	return(RPC_OK);
} /* rpcMusaChangeInst... */
#endif

/**************************************************************************
 * GNGETELEM
 * Retrieve the element with 'name'.
 *
 * Parameters:
 *	name (char *) - the name of the element. (Input)
 *	result (element **) - the element if found. (Output)
 *
 * Return Value:
 *	TRUE, element found.
 *	FALSE, element not found.
 */
int16 gnGetElem(name, result)
    char	*name;
    MusaElement	**result;
{
	char	buffer[1024];
	musaInstance	*dummy;
	char	*p;
	int16	junk;

	(void) strcpy(buffer, name);
	if (sim_format || (p = strrchr(buffer, separator)) == 0) {
		p = buffer;
	} else {
		*p = '\0';
		p++;
		if (!gnInstPath(buffer, &dummy, TRUE, &junk)) {
			return(FALSE);
		} /* if... */
	} /* if... */
	if (ElemLookup(searchd, p, result)) {
		return(TRUE);
	} /* if... */
	(void) sprintf(errmsg, "%s : No such element.", p);
	Warning(errmsg);
	sim_status |= SIM_NOTFOUND | SIM_USAGE;
	return(FALSE);
} /* gnGetElem... */

/************************************************************************
 * PRINTNAME
 * Print 'name' in next column.
 *
 * Parameters:
 *	name (char *) - the name to print. (Input)
 *	init (int16) - if TRUE then initialize columns to 0. (Input)
 */
void PrintName(name, init)
	char	*name;
	int16	init;
{
    static int	column;
    int			length;
    char		buf[256];

    if (init) {
#ifdef RPC_MUSA
	MUSAPRINTINIT(" \0");
#endif
	column = 0;
    } else {
	length = strlen(name);
	if (length + column + 1 >= 80) {
	    (void) sprintf (buf,"\n %s",name);
	    column = 0;
	} else {
	    (void) sprintf (buf," %s",name);
	}
	MUSAPRINT(buf);
	column += length + 1;
    }		
}				/* PrintName... */

#ifndef RPC_MUSA
/************************************************************************
 * PRINTNAME2
 * Print 'name' in next column.
 *
 * Parameters:
 *	name (char *) - the name to print. (Input)
 *	init (int16) - if TRUE then initialize columns to 0. (Input)
 *	field (int16) - the size of a column. (Input)
 */
void PrintName2(name, init, field)
	char	*name;
	int16	init;
	int16	field;
{
	static int column;
	int length;

	if (init) {
		column = 0;
	} else {
		length = strlen(name);
		if (length > field) {
			if (column + length + 1 >= 80) {
				my_print("\n");
			} /* if... */
			my_print(name);
			my_print("\n");
			column = 0;
			return;
		} /* if... */
		if (field + column + 1 >= 80) {
			my_print("\n");
			column = 0;
		} /* if... */
		my_print(name);
		my_print(" ");
		column += field + 1;
		while ((column + field + 1 < 80) && (length++ < field)) {
			my_print(" ");
		} /* while... */
	} /* if... */
} /* PrintName2... */
#endif


/*************************************************************************
 * DUMPLIST
 * Dump a list.
 */
#ifdef RPC_MUSA
/*************************************************************************
 * DUMPLIST
 * DumpList for X11 'rpc-musa'
 */
void DumpList (name,help,list,select)
	char	*name;
	char	*help;
	lsList	list;
	char	**select;
{
	lsGen		gen;
	dmWhichItem	dialogList[256];
	int			count;
	int			itemSelect;
	char		*item;

	if (lsLength(list) == 0) {
		*select = NIL(char);
		return;
	} /* if... */
	count = 0;
	lsSort(list,strcmp);
	gen = lsStart(list);
	while (lsNext(gen,(lsGeneric *) &item, LS_NH) == LS_OK) {
		dialogList[count].flag = FALSE;
		dialogList[count].userData = NULL;
		dialogList[count++].itemName = item;
	} /* while... */
	lsFinish(gen);
	if (dmWhichOne (name,count, dialogList,&itemSelect,NIL_FN(int),help) == VEM_OK) {
		*select = dialogList[itemSelect].itemName;
	} else {
		*select = NIL(char);
	} /* if... */
	lsDestroy(list,NIL_FN(void));
}
#else
/*******************************************************************
 * DUMPLIST
 * DumpList for command line version of 'musa'.
 */
void DumpList (list,max)
	lsList	list;
	int		max;
{
	lsGen	gen;
	char	*name;

	lsSort(list,strcmp);
	if (max > 80) {
		max = 0;
	} /* if... */
	PrintName2(NIL(char), TRUE, 0);
	gen = lsStart(list);
	while (lsNext(gen,(lsGeneric *) &name, LS_NH) == LS_OK) {
		PrintName2 (name, FALSE, max);
	} /* while... */
	lsFinish(gen);
	lsDestroy(list,NIL_FN(void));
	my_print("\n");
} /* DumpList... */
#endif

/***************************************************************************
 * GNLISTNETS
 * List elements in the current working instance, 'cwd'.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	cwd (musaInstance *) - current working instance. (Input)
 */
void gnListNets(argc, argv, cwd)
    int		argc;
    char	**argv;
    musaInstance	*cwd;
{
    musaInstance		*temp;
    MusaElement		*elem;
    int16		expression = FALSE;
    char		*p = 0;
    lsList		list = lsCreate();
    int			max = 0;
    int16		ok;
    st_generator* gen;
    char* key;
#ifdef RPC_MUSA
    char		*select;
#endif

    gnSetDir(cwd);
    if (argc != 0) {
	if ( (p = strrchr(argv[0],separator)) != NIL(char)) {
	    p++;
	} else {
	    p = argv[0];
	}			/* if... */
	if (!gnInstPath(argv[0], &temp, FALSE, &ok)) {
	    if (!ok) {
		return;
	    }			/* if... */
	    if (re_comp(p)) {
		Warning(re_comp(p));
		return;
	    } else {
		expression = TRUE;
	    }			/* if... */
	}			/* if... */
    }				/* if... */
    if (searchd->type != DEVTYPE_MODULE) {
	return;
    }				/* if... */
    elem = NIL(MusaElement);
    
    st_foreach_item( searchd->hashed_elements, gen, (char**)&key, (char**)&elem ) {
	if (sim_status & SIM_CMDINTERR) {
	    break;
	}			/* if... */
	if (expression) {
	    if (re_exec(elem->name)) {
		lsNewEnd(list,(lsGeneric) elem->name,LS_NH);	
		if ((strlen(elem->name) < 40) && (max < strlen(elem->name))) {
		    max = strlen(elem->name);
		}		/* if... */
	    }			/* if... */
	} else {
	    lsNewEnd(list,(lsGeneric) elem->name,LS_NH);	
	    if ((strlen(elem->name) < 40) && (max < strlen(elem->name))) {
		max = strlen(elem->name);
	    }			/* if... */
	}			/* if... */
    }				/* while... */
#ifdef RPC_MUSA
    DumpList("musa-listelems",NIL(char),list,&select);
    if (select != NIL(char)) {
	NodeInfo (1,&select,cwd);
    }				/* if... */
#else
    DumpList(list ,max );
#endif
}				/* gnListNets... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSALISTELEMS
 * Rpc interface to 'listelems' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaListElems (spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	struct RPCArg		*arg;
	int16				len = lsLength(cmdList);

    if ((len > 1) ||
		((len == 1) &&
		((lsDelEnd(cmdList,(lsGeneric *) &arg) != LS_OK) ||
		(arg->argType != VEM_TEXT_ARG)))) {
		Format("musa-listelems",(COMINFO *) cominfo);
		return (RPC_OK);
	} /* if... */
	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	if (len == 0) {
		gnListNets (0,NIL(char *),cwd);
	} else {
		gnListNets (1,&(arg->argData.string),cwd);
	} /* if... */
	return(RPC_OK);
} /* rpcMusaListElems... */
#endif

/*************************************************************************
 * LISTLOGIX 
 * List all nodes that have a voltage value of 'X' or 'x'.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	cwd (musaInstance *) - current working instance. (Input)
 */
void ListLogiX (argc, argv, cwd)
    int		argc;
    char	**argv;
    musaInstance	*cwd;
{
    MusaElement		*elem;
    node		*theNode;
    int16		flag = FALSE;
    st_generator* gen;
    char* key;

    (void) ignore((char *) argv);

#ifdef RPC_MUSA
    (void) ignore((char *) &argc);
#else
    if (argc != 0) {
	Usage("listlogicx\n");
	return;
    }				/* if... */
#endif
    PrintName(NIL(char),TRUE);
    gnSetDir(cwd);
    elem = NIL(MusaElement);
    st_foreach_item( searchd->hashed_elements, gen, (char**)&key, (char**)&elem ) {
	theNode = ElementToNode(elem);
	if ( (theNode->voltage & L_VOLTAGEMASK) == L_UNKNOWN) {
	    flag = TRUE;
	    PrintName(elem->name,FALSE);
	}			/* if... */
    }				/* while... */
    if (!flag) {
	MUSAPRINT("All elements have valid signals.\n");
    } else {
	MUSAPRINT("\n");
    }				/* if... */
#ifdef RPC_MUSA
    rpcMusaPrint("List LogiX");
#endif
}				/* ListLogiX... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSALISTLOGIX
 * Rpc interface to 'listlogix' command.
 */
int rpcMusaListLogiX (spot,cmdList,uow)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		uow;
{
	(void) ignore((char *) &cmdList);
	(void) ignore((char *) &uow);

	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	ListLogiX (0,NIL(char *),cwd);
	return(RPC_OK);
} /* rpcMusaListLogiX... */
#endif

/*************************************************************************
 * GNLISTINSTS
 * List instances in the current working instance, 'cwd'.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	cwd (musaInstance *) - current working instance. (Input)
 */
void gnListInsts(argc, argv, cwd)
	int		argc;
	char	**argv;
	musaInstance	*cwd;
{
    musaInstance	*inst, *temp;
    int		expression = FALSE;
    char	*p;
    int		max = 0;
    lsList	list;
    int16	ok;
#ifdef RPC_MUSA
    char	*select;
#endif

    list = lsCreate();
    gnSetDir(cwd);
    if (argc != 0) {
	if ( (p = strrchr(argv[0],separator) ) != NIL(char) ) {
	    p++;
	} else {
	    p = argv[0];
	}
	if (!gnInstPath(argv[0], &temp, FALSE, &ok)) {
	    if (!ok) {
		return;
	    }
	    if (re_comp(p)) {
		Warning(re_comp(p));
		return;
	    } else {
		expression = TRUE;
	    }
	}	
    }		
    if (searchd->type != DEVTYPE_MODULE) {
	return;
    }		
    {
	st_generator* gen;
	char* key;
	st_foreach_item( searchd->hashed_children, gen, (char**)&key, (char**)&inst ) {
	    if (sim_status & SIM_CMDINTERR) {
		break;
	    }	
	    if (expression) {
		if (re_exec(inst->name)) {
		    lsNewEnd(list,(lsGeneric) inst->name,LS_NH);	
		    if ((strlen(inst->name) < 40) && (max < strlen(inst->name))) {
			max = strlen(inst->name);
		    }
		}	
	    } else {
		lsNewEnd(list,(lsGeneric) inst->name,LS_NH);	
		if ((strlen(inst->name) < 40) && (max < strlen(inst->name))) {
		    max = strlen(inst->name);
		}	
	    }		
	}		
	if (sim_status & SIM_CMDINTERR) {
	    return;
	}
    }			
#ifdef RPC_MUSA
    DumpList("musa-listinsts",NIL(char),list,&select);
    if (select != NIL(char)) {
	InstInfo (1,&select,cwd);
    }
#else
    DumpList (list,max);
#endif
} 

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSALISTINSTS
 * Rpc interface to the 'listinst' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaListInsts (spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	struct RPCArg		*arg;
	int16				len = lsLength(cmdList);

    if ((len > 1) ||
		((len == 1) &&
		((lsDelEnd(cmdList,(lsGeneric *) &arg) != LS_OK) ||
		(arg->argType != VEM_TEXT_ARG)))) {
		Format("musa-listinsts",(COMINFO *) cominfo);
		return (RPC_OK);
	} /* if... */
	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	if (len == 0) {
		gnListInsts (0,NIL(char *),cwd);
	} else {
		gnListInsts (1,&(arg->argData.string),cwd);
	} /* if... */	
	return(RPC_OK);
} /* rpcMusaListInsts... */
#endif

/**************************************************************************
 * PATHNAMEGEN
 * Recursively generate a path for the given instance.  Accomplished by
 * moving up the tree to the root.
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 *	string (char *) - the path for the instance. (Output)
 */
void PathNameGen(inst, string)
    musaInstance *inst;
    char	 *string;
{
    char	buf[2];

    /* get parent of inst and p_path it */
    buf[0] = separator;
    buf[1] = '\0';
    if ( inst && inst->parent != NIL(musaInstance)) {
	PathNameGen(inst->parent, string);
	(void) strcat(string, inst->name);
	(void) strcat(string, buf);
    } else {
	/* must be root */
	if (sim_format) {
	    *string = '\0';
	} else {
	    (void) strcpy(string, buf);
	}
    }		
}		

/************************************************************************
 * MAKEPATH
 * Generate a path for an instance. 
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 *	string (char **) - the path for the instance. (Output)
 */
void MakePath(inst, string)
	musaInstance	*inst;
	char	**string;
{
	char	buf[1024];

	PathNameGen(inst, buf);
	*string = util_strsav(buf);
} /* MakePath... */

/**************************************************************************
 * GNPWD
 * Print the current search inst.
 *
 * Parameters:
 *	None.
 */
void gnPwd()
{
	char *path;

	MakePath(searchd, &path);
#ifdef RPC_MUSA
	vemMessage (strcat(path,"\n"),MSG_DISP);
	vemPrompt ();
#else
	my_print(path);
	my_print("\n");
#endif
	FREE(path);
} /* gnPwd... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSAPRINTINST
 * Rpc interface to 'printinst' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaPrintInst (spot,cmdList,uow)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		uow;
{
	(void) ignore((char *) &cmdList);
	(void) ignore((char *) &uow);

	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	gnSetDir(cwd);
	gnPwd();
	return(RPC_OK);
} /* rpcMusaPrintInst... */
#endif

MusaElement* getMainSiblingElement( elem )
    MusaElement* elem;
{
    MusaElement* p = elem;
    while ( p != 0 && (p->data & PARENT_PTR) == 0 ) {
	p = p->u.sibling;
    }
    return p;
}

musaInstance* getParentInstance( elem )
    MusaElement* elem;
{
    MusaElement* p = getMainSiblingElement( elem );

    return p ? p->u.parentInstance : NIL(musaInstance);
}


/***********************************************************************
 * MAKEELEMPATH
 * Generate a path for an element structure.
 *
 * Parameters:
 *	elem (MusaElement *) - the element structure (Input)
 *	string (char *) - the path for the element. (Output)
 */
void MakeElemPath(elem, string)
    MusaElement	*elem;
    char	**string;
{
    if (elem == NIL(MusaElement)) {
	*string = util_strsav("No connection");
    } else {
	char		buf[1024];
	musaInstance* parentInst = getParentInstance( elem );
	PathNameGen( parentInst, buf);
	(void) strcat(buf, elem->name);
	*string = util_strsav(buf);
    }
}

/*************************************************************************
 * MAKECONNPATH
 * Generate path for connection structure, 'conn'
 * If terminal number is one to expand, synthesize name.
 *
 * Parameters:
 *	conn (connect *) - connection structure. (Inptu)
 *	string (char **) - the path for the connection. (Output)
 */
void MakeConnPath(conn, string)
    connect		*conn;
    char		**string;
{
    musaInstance	*device;			/* Pointer to device */
    int			devType;			/* Device type */
    int			i, expand;			/*  */
    char		buf[1024], buf2[64];		/*  */

    device = conn->device;
    PathNameGen(conn->device, buf);

    /* Find which connect of the inst this is. */
    for (i = 0; ; i++) {
	if (&(device->connArray[i]) == conn) {
	    break;
	}
    }	


    /* Do we have term names for this device type ? */
    devType = (int)device->type;
    if ( deviceTable[devType].term == 0 ) { /* No names */
	sprintf( buf2, "%s?term%d?", buf, i );
	strcpy( buf, buf2 );
    } else {
	expand = deviceTable[(int) device->type].expandable;
	if ((expand == 0) || (i < expand)) {
	    (void) strcat(buf, deviceTable[(int) device->type].term[i].name);
	} else {
	    /* synthesize name */
	    (void) sprintf(buf2, "%s%d", deviceTable[(int) device->type].term[expand].name,i - expand + 1);
	    (void) strcat(buf, buf2);
	}	
    }
    *string = util_strsav(buf);
}				/* MakeConnPath... */

/***********************************************************************
 * NEXTEQUIV
 * Return the next equivalent element in line after given element, 'elem'.
 *
 * Parameters:
 *	elem (element *) - the previous element. (Input)
 *	free (int16) - if TRUE, then free 'elem'. (Input)
 *
 * Return Value:
 *	The next equivalent element or NULL if no more. (element *) 
 */
MusaElement *NextEquiv(elem, free)
    MusaElement		*elem;
    int16		free;
{
    MusaElement		*temp = 0;

    if (sim_format) {
	temp = elem->c.equiv;
	if (free) {
	    ElemUnlink(elem);
	    ElemFree(elem);
	}			/* if... */
	return temp;
    } 

    if (elem->c.child != NIL(MusaElement)) {
	return elem->c.child;
    }

    while ((elem->data & EPARENT_PTR) != 0) {
	/* climb up hierarchy */
	temp = elem->equiv.parent;
	if (free) {
	    ElemUnlink(elem);
	    ElemFree(elem);
	}		
	elem = temp;
    }		
    if ((elem->data & NODE_PTR) == 0) {
	/* go get sibling */
	temp = elem->equiv.sibling;
	if (free) {
	    ElemUnlink(elem);
	    ElemFree(elem);
	}		
	return temp;
    } else {
	/* no more to gen */
	if (free) {
	    ElemUnlink(elem);
	    ElemFree(elem);
	}		
	return NIL(MusaElement);
    }		
}

/***********************************************************************
 * GNEQUIVELEMS
 * List all equivalent elements of a given element.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	cwd (musaInstance *) - current working instance. (Input)
 */
void gnEquivElems(argc, argv, cwd)
	int		argc;
	char	**argv;
	musaInstance	*cwd;
{
    MusaElement		*data;
    MusaElement		*e;
    node		*theNode;
    char		*name;

#ifdef RPC_MUSA
    (void) ignore((char *) &argc);
#else
    if (argc < 1) {
	Usage("equivelems element\n");
	sim_status |= SIM_USAGE;
	return;
    }
#endif
    gnSetDir(cwd);
    if (!gnGetElem(argv[0], &data)) {
	return;
    }				/* if... */
    theNode = ElementToNode(data);
    PrintName(NIL(char), TRUE);
    for (e = theNode->rep;e != NIL(MusaElement);e = NextEquiv(e, FALSE)) {
	if (sim_status & SIM_CMDINTERR) {
	    break;
	}	
	MakeElemPath(e, &name);
	PrintName(name, FALSE);
	FREE(name);
    }		
    MUSAPRINT("\n");
#ifdef RPC_MUSA
    rpcMusaPrint("Equivalent Elements");
#endif
}				/* gnEquivElems... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSAEQUIVELEMS
 * Rpc interface to 'equivalent' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaEquivElems (spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	char			*argList[1];

	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
    if (lsLength(cmdList) == 0) {
		if (rpcMusaUnderSpot(spot,1,argList,0,FALSE,FALSE,gnEquivElems)) {
			return(RPC_OK);
		} else {
			Format("musa-equivalent",(COMINFO *) cominfo);
			return (RPC_OK);
		} /* if... */
	} /* if... */
	rpcMusaRunRoutine(spot,cmdList,1,argList,0,FALSE,FALSE,gnEquivElems);
	return(RPC_OK);
} /* rpcMusaEquivElems... */
#endif

/************************************************************************
 * MAKEVECT
 * Define a new vector.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	cwd (musaInstance *) - current working instance. (Input)
 */
void MakeVect(argc, argv, cwd)
	int		argc;
	char	**argv;
	musaInstance	*cwd;
{
	MusaElement	*data;
	vector	*result;
	vect	*new, *prev;		/* new vect in string */
	char	buffer[1024];
	char	*headend, *tail, *p;
	int		start, end, i, j;

#ifndef RPC_MUSA
	if (argc < 2) {
		Usage("MakeVector name node1 [node2 ...]\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	if (st_lookup(vectors, argv[0], (char **) &result)) {
		FREE(result->name);
		for (prev = result->highbit; prev != NIL(vect);) {
			new = prev;
			FREE(new);
			prev = new;
		} /* for... */
		FREE(result);
	} /* if... */
	result = ALLOC(vector, 1);
	result->name = util_strsav(argv[0]);
	new = NIL(vect);
	for (i = 1; i < argc; i++) {
		j = 0;
		/* processing for colon operator */
		for (p=argv[i]; ((*p != ':') && (*p != '\0')); p++) {
			buffer[j++] = *p;
		} /* for... */
		if (*p == ':') {
			buffer[j--] = '\0';
			/* index back for number */
			while (isdigit(buffer[j])) {
				j--;
			} /* while... */
			j++;
			headend = &(buffer[j]);
			start = atoi(headend);
			tail = &(p[1]);
			end = atoi(tail);
			while (isdigit(*tail)) {
				tail++;
			} /* while... */
		} else {
			buffer[j] = '\0';
			start = end = 0;
			headend = NIL(char);
		} /* if... */
		for (j = start;(start<end) ? j<=end : j>=end;(start<end) ? j++ : j--) {
			if (headend != NIL(char)) {
				(void) sprintf(headend, "%d", j);
				(void) strcat(headend, tail);
			} /* if... */
			gnSetDir(cwd);
			if (new == NIL(vect)) {
				result->highbit = new = ALLOC(vect, 1);
				new->prev = new->next = NIL(vect);
			} else {
				prev = new;
				prev->next = ALLOC(vect, 1);
				new = prev->next;
				new->prev = prev;
				new->next = NIL(vect);
		    } /* if... */
			if (gnGetElem(buffer, &data)) {
				new->is_blank = 0;
				new->data = data;
			} else {
				new->is_blank = 1;
			} /* if... */
		} /* for... */
	} /* for... */
	result->lowbit = new;
	(void) st_insert(vectors, result->name, (char *) result);
	return;
} /* MakeVect... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSAMAKEVECTOR
 * Rpc interface to 'makevector' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaMakeVector(spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	char			*argList[256];
	struct RPCArg	*name;

	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
    if ((lsLength(cmdList) < 2) ||
		(lsDelEnd(cmdList,(lsGeneric *) &name) != LS_OK) ||
		(name->argType != VEM_TEXT_ARG)) {
		Format("musa-makevector",(COMINFO *) cominfo);
		return (RPC_OK);
	} /* if... */
	argList[0] = name->argData.string;
	rpcMusaRunRoutine(spot,cmdList,2,argList,1,TRUE,FALSE,MakeVect);
	return(RPC_OK);
} /* rpcMusaMakeVector... */
#endif

/*************************************************************************
 * VECTSTRING
 * Generate a string that represents the value of the vector, 'highbit'
 * Warning: The string is static, not malloced.
 *
 * Parameters:
 *	highbit (vect *) - the vector. (Input)
 *	mode (int16) -	if 0, binary string.
 *					if 1, hex string.
 *					if 2, binary string - no underscores. (Input)
 *
 * Return Value:
 *	The string. (char *)
 */
char *VectString(highbit, mode)
	vect			*highbit;
	int16			mode;	/* print in mode (single bit is always binary) */
{
    static char		string[1030];
    node			*theNode;
    vect			*v;
    int				i = 0, j = 0, k;
    int				acc; /* > 15 implies X */

    for (v = highbit; v != NIL(vect); v = v->next) {
	j++;
    }				/* for... */
    if (mode == 1 && (highbit->next != NIL(vect))) {
	string[i++] = 'H';
	v = highbit;
	while (j > 0) {
	    if ((j % 16 == 0) && (v != highbit)) {
		string[i++] = '_';
	    }			/* if... */
	    for (k = (j-1)%4, acc = 0; k >= 0; k--, j--) {
		acc <<= 1;
		if (!v->is_blank) {
		    theNode = ElementToNode(v->data);
		    if (UNKNOWN(theNode)) {
			acc = 16;
		    } else {
			if (HIGH(theNode)) {
			    acc++;
			}
		    }
		}
		v = v->next;
	    }	
	    if (acc > 15) {
		string[i++] = 'X';
	    } else {
		string[i++] = "0123456789ABCDEF"[acc];
	    }	
	}	
    } else {
	for(v = highbit; v != NIL(vect); v = v->next) {
	    if ((j % 8 == 0) && (v != highbit) && (mode != 2)) {
		string[i++] = '_';
	    }			
	    if (v->is_blank) {
		string[i++] = '.';
	    } else {
		theNode = ElementToNode(v->data);
		string[i++] = LETTER(theNode);
	    }
	    j--;
	}	
    }		
    string[i] = '\0';
    return(string);
}				/* VectString... */

/************************************************************************
 * convertIntoBitString
 * Make a binary string from the given (possible hex) string, 'input'.
 *
 * Parameters:
 *	input (char *) - the input string. (Input)
 *
 * Return Value:
 *	The converted string. (char *) 
 */
char *convertIntoBitString( input )
    char    *input;
{
    static char	string[1030];
    char   *p;
    int	   number, i = 0, j;

    if ((*input != 'h') && (*input != 'H')) {
	for (i = 0, p = input; *p != '\0'; p++) {
	    if (*p != '_') {
		string[i++] = *p;
	    }
	}	
    } else {			/* Hexadecimal input. */
	p = input;
	p++;			/* skip over 'h' */
	for ( ; *p != '\0'; p++) {
	    if ((*p == 'x') || (*p == 'X')) {
		(void) strcpy(&(string[i]), "XXXX");
		i += 4;
		continue;
	    } else if (*p == '.') {
		(void) strcpy(&(string[i]), "....");
		i += 4;
		continue;
	    } else if (*p == '_') {
		continue;
	    } else if ((*p >= '0') && (*p <= '9')) {
		number = *p - '0';
	    } else if ((*p >= 'a') && (*p <= 'f')) {
		number = *p - 'a' + 10;
	    } else if ((*p >= 'A') && (*p <= 'F')) {
		number = *p - 'A' + 10;
	    } else {
		string[i++] = *p;
		break;
	    }	
	    for (j=3; j>=0; j--) {
		if ((number & (1 << j)) == 0) {
		    string[i++] = '0';
		} else {
		    string[i++] = '1';
		}
	    }		
	}		
    }			
    string[i] = '\0';
    return string ;
}				/* convertIntoBitString... */

char* convertIntoString( value )
    int value;
{
    static char buffer[64];
    sprintf( buffer, "H%08x", value ); /* Print in hex. */
    return  buffer;
}
/************************************************************************
 * READFORMAT
 * Parse a format string for the 'show' command.
 *
 * Parameters:
 *	format (char **) - the format string, parsed part of string 
 *		will be removed. (Input/Output)
 *
 * Return Value:
 *	0, if binary.
 *	1, if hex.
 */
int16 ReadFormat(format)
	char	**format;
{
    int16	i = 0;
    int16	result = 1;	/* default */
    char	buffer[1024];
    char	*c;

    c = *format;
    if (*c == '\"') c++;
    for ( ; (*c != '\0') && (*c != '\"'); c++) {
	if (*c == '\\') {
	    if (*(++c) == 'n') {
		buffer[i++] = '\n';
		buffer[i] = '\0';
		PrintName(buffer, FALSE);
		i = 0;
		PrintName(NIL(char), TRUE); /* re-initialize */
	    } else if (*c == 't') {
		buffer[i++] = '\t';
	    } else {
		buffer[i++] = *c;
	    }	
	} else if (*c == '%') {
	    c++;		/* skip '%' */
	    if (upper(*c) != 'H' && upper(*c) != 'X') {
		result = 0;
	    }
	    c++;		/* skip type letter (H or X, etc.) */
	    break;
	} else {
	    buffer[i++] = *c;
	}	
    }		
    if (*c == '\"') c++;
    buffer[i] = '\0';
    if (i > 0) {
	PrintName(buffer, FALSE);
    }		
    *format = c;
    return(result);
}				/* ReadFormat... */

/*******************************************************************************
 * LOOKUPVECT
 * Given the 'name' of a vector or MusaElement, return the 'vect' for it.
 *
 * Parameters:
 *	name (char *) - the name of the vector or MusaElement. (Input)
 *	highbit (vect **) - high bit of vector. (Output)
 *	lowbit (vect **) - low bit of vector. (Output)
 *	cwd (musaInstance *) - current working instance. (Input)
 *
 * Return Value:
 *	TRUE, vector or MusaElement found.
 *	FALSE, vector or MusaElement not found.
 */
int16 LookupVect(name, highbit, lowbit, cwd)
	char		*name;
	vect		**highbit, **lowbit;
	musaInstance		*cwd;
{
    vector		*theVector;
    static vect	elemvect;

	if (st_lookup(vectors, name, (char **) &theVector)) {
		*highbit = theVector->highbit;
		*lowbit = theVector->lowbit;
	} else {
		elemvect.next = elemvect.prev = NIL(vect);
		elemvect.is_blank = 0;
		gnSetDir(cwd);
		if (!gnGetElem(name, &(elemvect.data))) {
			return(FALSE);
		} /* if... */
		*highbit = *lowbit = &elemvect;
	} /* if... */
	return(TRUE);
} /* LookupVect... */

/***********************************************************************
 * SHOWVALUE
 * Display a vector or MusaElement, used for 'show' command.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	cwd (musaInstance *) - current working directory. (Input)
 */
void ShowValue(argc, argv, cwd)
	int			argc;
	char		**argv;
	musaInstance		*cwd;
{
	watchlist	*wl;
	watchl		*w;
	vect		*highbit, *lowbit;
	int			i = 0;
	int			value;
	char		*format;		/* gives the format of a print */
	int16		formatstat = 0;	/* 0 if binary, 1 if hex */

	if (argv[i][0] == '\"') {
		format = argv[i++];
	} else {
		format = "";
	} /* if... */
	PrintName(NIL(char), TRUE);		/* initialize print funct */
	formatstat = ReadFormat(&format);
	for (; i<argc; i++) {
		if (sim_status & SIM_CMDINTERR) break;
		if (argv[i][0] == '#') {
			if (!st_lookup(constants, argv[i], (char **) &value)) {
				value = 0;
			} /* if... */
			(void) sprintf(msgbuf, "%d", value);
			PrintName(msgbuf, FALSE);
		} else if (st_lookup(watchlists, argv[i], (char **) &wl)) {
			for (w = wl->first; w != NIL(watchl); w = w->next) {
				if (sim_status & SIM_CMDINTERR) break;
				if (LookupVect(w->name, &highbit, &lowbit, cwd)) {
					(void) sprintf(msgbuf, "%s:%s", w->name, VectString(highbit, formatstat));
					PrintName(msgbuf, FALSE);
				} /* if... */
			} /* for... */
		} else {
			if (LookupVect(argv[i], &highbit, &lowbit, cwd)) {
				(void) sprintf(msgbuf,"%s:%s",argv[i],VectString(highbit, formatstat));
				PrintName(msgbuf, FALSE);
			} /* if... */
		} /* if... */
		formatstat = ReadFormat(&format);
	} /* for... */
	MUSAPRINT("\n");
#ifdef RPC_MUSA
	rpcMusaPrint("Show Value(s)");
#endif
} /* ShowValue... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSASHOW
 * Rpc interface to 'show' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaShow(spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	char			*argList[256];
	struct RPCArg	*name;
	char			buffer[256];

    if (lsLength(cmdList) == 0) {
		Format("musa-show",(COMINFO *) cominfo);
		return (RPC_OK);
	} /* if... */
	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	if ((lsLength(cmdList) > 1) && 
		(lsLastItem(cmdList,(lsGeneric *) &name, LS_NH) == LS_OK) &&
		(name->argType == VEM_TEXT_ARG)) {
		(void) lsDelEnd(cmdList,(lsGeneric *) &name);
		(void) sprintf (buffer,"\"%s\"",name->argData.string);
		argList[0] = util_strsav(buffer);
		rpcMusaRunRoutine(spot,cmdList,2,argList,1,TRUE,FALSE,ShowValue);
	} else {
		rpcMusaRunRoutine(spot,cmdList,2,argList,0,TRUE,FALSE,ShowValue);
	} /* if... */
	return(RPC_OK);
} /* rpcMusaShow... */
#endif

int getNumericArg( arg, value )
    char* arg;
    int* value;
{
    *value = 0;			/* Default. */
    
    if ( *arg == '#' ) {
	/* Setting to a constant. */ 
	if (!st_lookup(constants, arg, (char **) value)) {
	    *value = 0;
	}
	return 1;		/* Ok */
    }

    if (isdigit( *arg ) ) {
	*value = atoi( arg );
	return 1;
    }

    if ( *arg == 'h' || *arg == 'H' ) {
	char* p = arg + 1;
	while ( *p != '\0' ) {
	    *value *= 16;
	    if ( *p >= '0' && *p <= '9' ) {
		*value += *p - '0';
	    } else if ( *p >= 'A' && *p <= 'F' ) {
		*value += *p - 'A' + 10;
	    } else if ( *p >= 'a' && *p <= 'f' ) {
		*value += *p - 'a' + 10;
	    } else {
		sprintf( errmsg, "Illegal hex value %s", arg );
		Warning( errmsg  );
		return 0;
	    }
	    p++;
	}
	return 1;
    }
    if ( *arg == 'b' || *arg == 'B' ) {
	char* p = arg + 1;
	
	while ( *p != '\0' ) {
	    *value <<= 1;
	    if ( *p == '1' ) {
		*value += 1;
	    } else if ( *p != '0' ) {
		sprintf( errmsg, "Illegal binary value %s", arg );
		Warning( errmsg  );
		return 0;
	    }
	    p++;
	}
	return 1;
    }
    return 0;    /* Warning( "Non numeric arg" ); */
}



/*************************************************************************
 * SETVECT
 * 'set' a vector to a specified value.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	cwd (musaInstance *) - current working directory. (Input)
 */
void SetVect(argc, argv, cwd)
    int		argc;
    char	**argv;
    musaInstance *cwd;
{
    char    *value;
    node    *theNode;
    vect    *v, *high;
    int     acc = 0;
    int     i;
    int	    temp;		/* Used to handle constants */
    char    tempbuffer[32];	/* Used to write the value of constants. */
    char    pending = '=';
    char    *name;

    if (*(argv[0]) == '#') {
#ifndef RPC_MUSA
	if (argc < 2) {
	    Usage("set #constant equation\n");
	    sim_status |= SIM_USAGE;
	    return;
	}		
#endif
	for (i=1; i<argc; i++) {
	    if ( ! getNumericArg( argv[i], &temp ) ) {
		pending = *(argv[i]);
		continue;
	    }		
	    switch (pending) {
	    case '=':		/* Assignment */
		acc = temp;
		break;
	    case '+':		/* Plus */
		acc += temp;
		break;
	    case '-':		/* Minus */
		acc -= temp;
		break;
	    case '*':		/* Multiply */
		acc *= temp;
		break;
	    case '/':		/* Divide */
		acc /= temp;
		break;
	    case '%':		/* Modulo */
		acc %= temp;
		break;
	    case '|':		/* Or */
		acc |= temp;
		break;
	    case '&':		/* And */
		acc &= temp;
		break;
	    case '^':
		acc ^= temp;	/* Xor */
		break;
	    case '<':
		acc <<= temp;
		break;
	    case '>':
		acc >>= temp;
		break;
	    default:
		sim_status |= SIM_USAGE;
		Usage("set #constant = <expression>");
		return;
	    }		
	}		
	name = util_strsav(argv[0]);
	if (st_insert(constants, name, (char *) acc)) {
	    FREE(name);
	}		
    } else {
#ifndef RPC_MUSA
	if (argc != 2) {
	    Usage("set name value\n");
	    sim_status |= SIM_USAGE;
	    return;
	}		
#endif
	if (!LookupVect(argv[0], &high, &v, cwd)) return;

	if ( getNumericArg( argv[1], &temp ) ) {
	    value = convertIntoBitString( convertIntoString( temp ) );
	} else {
	    value = convertIntoBitString( argv[1] );
	}
	i = strlen(value) - 1;
	for( ; v != NIL(vect); v = v->prev) {
	    while (i >= 0 && value[i] == '_') i--;
	    if (i < 0) break;
	    if (!v->is_blank) {
		theNode = ElementToNode(v->data);
		switch (value[i]) {
		case '1': case 'I':
		    musaRailDrive(theNode, TRUE);
		    break;
		case '0': case 'O':
		    musaRailDrive(theNode, FALSE);
		    break;
		case 'i': case 'o': case 'x': case 'X':
		    musaRailUndrive(theNode, TRUE);
		    musaRailUndrive(theNode, FALSE);
		    break;
		case '.':
		    break;
		default:
		    (void) sprintf(errmsg, "%c : Unknown character.",value[i]);
		    Warning (errmsg);
		    sim_status |= SIM_USAGE;
		    break;
		}		/* switch... */
	    } else {
		(void) sprintf(errmsg, "Trying to set a value in a partially defined vector.");
		Warning(errmsg);
	    }		
	    i--;
	}		
    }			
}				/* SetVect... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSASET
 * Rpc interface to 'set' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaSet (spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	char			*argList[2];
	struct RPCArg	*name;

    if ((lsLength(cmdList) < 2) ||
		(lsDelEnd(cmdList,(lsGeneric *) &name) != LS_OK) ||
		(name->argType != VEM_TEXT_ARG)) {
		Format("musa-set",(COMINFO *) cominfo);
		return (RPC_OK);
	} /* if... */
	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	argList[1] = name->argData.string;
	rpcMusaRunRoutine(spot,cmdList,2,argList,0,FALSE,FALSE,SetVect);
	rpcMusaUpdateFacetSelSet(spot->facet);
	return(RPC_OK);
} /* rpcMusaSet... */
#endif

/***************************************************************************
 * VERIFY
 * Check if an MusaElement has a specified voltage level.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - arguments. (Input)
 *	cwd (musaInstance *) - current working directory. (Input)
 */
void Verify(argc, argv, cwd)
	int		argc;
	char	**argv;
	musaInstance	*cwd;
{
	vect	*low, *high;
	char	*realvalue, *inputvalue;
	int16	same = TRUE;
	int		i, j;
	int     value;

#ifdef RPC_MUSA
	(void) ignore((char *) &argc);
#else
	if (argc != 2) {
		Usage("Verify name value\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	if (!LookupVect(argv[0], &high, &low, cwd)) return;
	if ( getNumericArg( argv[1], &value ) ) {
	    inputvalue = convertIntoBitString( convertIntoString( value ) );
	} else {
	    inputvalue = convertIntoBitString( argv[1] );
	}


	realvalue = VectString(high, 2);

	/* Start comparison from end of strings. */
	i = strlen(inputvalue) - 1;
	j = strlen(realvalue) - 1;

	for ( ; j>=0; j--) {
		if (i < 0) break;
		switch (inputvalue[i]) {
		case '1': case 'I': case 'i':
		    if ((realvalue[j] != '1') && (realvalue[j] != 'I') &&
			(realvalue[j] != 'i') && (realvalue[j] != '.')) {
			same = FALSE;
		    } 
		    break;
		case '0': case 'O': case 'o':
		    if ((realvalue[j] != '0') && (realvalue[j] != 'O') &&
			(realvalue[j] != 'o') && (realvalue[j] != '.')) {
			same = FALSE;
		    } 
		    break;
		case 'X': case 'x':
		    if ((realvalue[j] != 'X') && (realvalue[j] != 'x') &&
			(realvalue[j] != '.')) {
			same = FALSE;
		    } 
		    break;
		case '.':
		    break;
		default:
		    (void) sprintf(errmsg, "%c : Unknown character.", inputvalue[i]);
		    Warning(errmsg);
		    sim_status |= SIM_USAGE;
		    break;
		}		/* switch... */
		i--;
	}
	if (!same) {
		(void) sprintf(errmsg, "Verify failed: %s has value %s not %s.",
			argv[0], &realvalue[j+1], &inputvalue[i+1]);
		Warning(errmsg);
		sim_status |= SIM_VERIFY;
	} 
} /* Verify... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSAVERIFY
 * Rpc interface to 'verify' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaVerify (spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	char			*argList[2];
	struct RPCArg	*name;

    if ((lsLength(cmdList) < 2) ||
		(lsDelEnd(cmdList,(lsGeneric *) &name) != LS_OK) ||
		(name->argType != VEM_TEXT_ARG)) {
		Format("musa-verify",(COMINFO *) cominfo);
		return (RPC_OK);
	} /* if... */
	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	argList[1] = name->argData.string;
	rpcMusaRunRoutine(spot,cmdList,2,argList,0,FALSE,FALSE,Verify);
	return(RPC_OK);
} /* rpcMusaVerify... */
#endif

/***************************************************************************
 * WATCH
 * Add specified vectors and elements to a watch list.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) - the arguments. (Input)
 *	cwd (musaInstance *) - current working instance. (Input)
 */
void Watch(argc, argv, cwd)
	int			argc;
	char		**argv;
	musaInstance		*cwd;
{
	watchlist	*wl;
	watchl		*w;
	vect		*high, *low;
	int			i;

#ifndef RPC_MUSA
	if (argc < 2) {
		Usage("watch watchlist names ...\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	if (!st_lookup(watchlists, argv[0], (char **) &wl)) {
		wl = ALLOC(watchlist, 1);
		wl->name = util_strsav(argv[0]);
		wl->first = NIL(watchl);
		st_add_direct(watchlists, wl->name, (char *) wl);
	} 
	if (wl->first == NIL(watchl)) {
		w = NIL(watchl);
	} else {
		for (w = wl->first; w->next != NIL(watchl); w = w->next){
		    Warning( "Watch: Not fully implemented" );
		}
	} 
	for (i=1; i<argc; i++) {
		if (LookupVect(argv[i], &high, &low, cwd)) {
			if (w == NIL(watchl)) {
				w = wl->first = ALLOC(watchl, 1);
			} else {
				w->next = ALLOC(watchl, 1);
				w = w->next;
			} /* if... */
			w->name = util_strsav(argv[i]);
			w->next = NIL(watchl);
			w->mode = 0;
		} /* if... */
	} /* for... */
} /* Watch... */

#ifdef RPC_MUSA
/*************************************************************************
 * RPCMUSAWATCH
 * Rpc interface routine to the 'watch' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - argument list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	RPC_OK, success.
 */
int rpcMusaWatch (spot,cmdList,cominfo)
	RPCSpot		*spot;
	lsList		cmdList;
	int32		cominfo;
{
	char			*argList[256];
	struct RPCArg	*name;

    if ((lsLength(cmdList) < 2) ||
		(lsDelEnd(cmdList,(lsGeneric *) &name) != LS_OK) ||
		(name->argType != VEM_TEXT_ARG)) {
		Format("musa-watch",(COMINFO *) cominfo);
		return (RPC_OK);
	} /* if... */
	(void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
	argList[0] = name->argData.string;
	rpcMusaRunRoutine(spot,cmdList,2,argList,1,TRUE,FALSE,Watch);
	return(RPC_OK);
} /* rpcMusaWatch... */
#endif


