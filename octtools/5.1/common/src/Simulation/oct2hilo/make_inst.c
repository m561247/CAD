#include "copyright.h"
#include "port.h"
#include "oct.h"
#include "st.h"
#include "oh.h"
#include "head.h"

extern octObject cfacet;
extern char *usedoutput[];
extern char *outputs[];
extern char *input[];
extern char *supply1[];
extern char *supply0[];
extern int debug;

int inst_cnt=0;
int gate_cnt=0;
int incnt=0,outcnt=0;
int UNKN=0;

extern char* lower();

struct termStruct {
 char *type;
 char *term;
} inTerm[MAX_TERM], outTerm[MAX_TERM];



/*
 * make_inst() loads the symbolic netlist.  this routine could use
 *     alot of cleanup work.  it could use some reworking of the
 *     data structures as well.
 */

make_inst(bag)
  octObject *bag; {

  octObject  inst, term, net, fterm;
  char *thename;
  char *real_name;
  char *gate_name;
  char *termType;
  char *termName;
  char *direction;
  int i;
  int inst_place,gate_place,in_count,out_count,unk_count=0;
  octGenerator instgen, termgen, netgen, ftermgen;
  octObject term_name,term_dir;
  struct INT_term *tmp;

  /* prepare instance generator */
  o_init_gen_contents(bag, OCT_INSTANCE_MASK, &instgen);

  /* while we have more instances */
  while (octGenerate(&instgen, &inst) == OCT_OK) {

    /* inst.contents.instance.master is the name of the cell */
    /* convert name to something we can use */
    real_name = (char *) get_name(inst.contents.instance.master);

    /* add to this instance to cell list. each cell is
       given an index if it hasn't been inserted yet.  previously
       added cells are given the same number */
    inst_place = insert_inst(real_name,inst.contents.instance.master);


    /* inst.contents.instance.name is the name of instantiation */
    /* get a cleaned up gate name */
    gate_name = (char *)makeGateName(inst.contents.instance.name);

    /* add the gate to the instantiated gate list */
    gate_place = insert_gate(gate_name,inst_place);

    in_count =0;
    out_count =0;
    /* initialize terminal generator */
    o_init_gen_contents(&inst, OCT_TERM_MASK, &termgen);

    /* read in all the terms */
    while (octGenerate(&termgen, &term) == OCT_OK) {
      o_init_gen_containers(&term, OCT_TERM_MASK, &ftermgen);
      o_init_gen_containers(&term, OCT_NET_MASK, &netgen);

      if (octGenerate(&ftermgen, &fterm) == OCT_OK) {
        if(debug>=1) {
          fprintf(stderr,"make_inst: got formal terminal name\n");
          fflush(stderr);
          }
        termType = (char *)copy_str(term.contents.term.name,1);
	termName = (char *)copy_str(fterm.contents.term.name,1);
	} 
      else if (octGenerate(&netgen, &net) == OCT_OK) {
	if (net.contents.net.name!=NULL) {
          if(debug>=1) {
            fprintf(stderr,"make_inst: got terminal name from net\n");
            fflush(stderr);
            }
           termType = (char *)copy_str(term.contents.term.name,1);
	   termName = (char *)copy_str(net.contents.net.name,1);
	  } 
        else {
          if(debug>=1) {
            fprintf(stderr,"make_inst: didn't get terminal from net\n");
            fflush(stderr);
            }
	  termType = (char *)copy_str(term.contents.term.name,1);
	  termName = (char *)copy_str(term.contents.term.name,1);
	  }
        } 
      else {
        if(debug>=1) {
          fprintf(stderr,"make_inst: didn't get terminal from net or fterm\n");
          fflush(stderr);
          }
  	termType= (char *)copy_str("UNCONNECTED",1);
	termName= (char *)copy_str(term.contents.term.name,1);
        }

      if(debug>=1) {
        fprintf(stderr,"termType: %s termName %s\n",termType,termName);
        fflush(stderr);
        }

      /* I'm not really sure exactly what the following bit of code
         does.  It needs work.  */
      if(*termType == 'O')  {
        if(debug>=1) {
          fprintf(stderr,"*termType: O termName %s\n",termName);
          fflush(stderr);
          }
	outTerm[outcnt].type = (char *)copy_str(termType,1);
        tmp = (struct INT_term *)add_termName(termName,TERM_OUT);
        outTerm[outcnt].term = tmp->name;
        outcnt++;
        }
      else if ((isdigit(*termType)) || 
                 (isdigit(termType[(int)strlen(termType)-1])))  {
        if(debug>=1) {
          fprintf(stderr,"termType is digit %s %s\n",termType,termName);
          fflush(stderr);
          }
	inTerm[incnt].type = (char *)copy_str(termType,1);
       	tmp = (struct INT_term *)add_termName(termName,TERM_OUT);
      	inTerm[incnt].term = tmp->name;
	incnt++;
	check_supply(termName);
        } 
      else  {
        if(debug>=1) {
        fprintf(stderr,"Doesn't begin with O or digit: %s  %s\n",termType,termName);
        fflush(stderr);
          }
	termName=(char *)lower(termName);
	if((strncmp(termName,"vdd",3)!=0)&&(strncmp(termName,"gnd",3)!=0)){
        if(debug>=1) {
          fprintf(stderr,"Not vdd or gnd?:  termName %s\n",termName);
          fflush(stderr);
          }
	  if(strcmp(termType,"unconnected")==-32) {
	    outTerm[outcnt].type = (char *)copy_str(termName,1);
       	    outTerm[outcnt].term=(char *)malloc(sizeof (char)+1);
	    outcnt++;
	    } 
          else {
            if(debug>=1) {
              fprintf(stderr,"Vdd or GND? %s\n",termName);
              fflush(stderr);
              }
	    outTerm[outcnt].type = (char *)copy_str(termType,1);
       	    tmp = (struct INT_term *) add_termName(termName,TERM_OUT);
       	    outTerm[outcnt].term = tmp->name;
	    outcnt++;
	    UNKN = 1;
            insert_unk(outTerm[outcnt].term);
            }
          }
	}
      }
    write_terms(gate_place,inst_place);
    outcnt=0;
    incnt=0;
    }
  }


/*
 * insert_inst() inserts a cell type into my_inst.  if it already
 *     exists, it's index is returned.  otherwise, the cell is opened
 *     and the terminal names and direction are pulled out for future
 *     reference.
 */

int insert_inst(instance,path)
    char *instance; 
    char *path; 
{

    octObject cfacet;
    octObject ifacet;
    octObject termobj, term_name, term_dir,net;
    octGenerator tgen,ngen;
    int prim_num,found;
    int inst_place,i;
    char term[256],dir[256];

    /* do we already have instance data? if so, return it */
    inst_place = -1;
    for (i=0; i<inst_cnt; i++) {
	if (strcmp(my_inst[i].inst_numb,instance)==0) {
	    if(debug>=1) {
		fprintf(stderr,"make_inst: found instance %s num %d\n",instance,i);
		fflush(stderr);
	    }
	    inst_place = i;
	    break;
	}
    }

    /* otherwise, load it up! */
    if (inst_place == -1) {
    
	if(debug>=1) {
	    fprintf(stderr,"make_inst: didn't find instance %s loading as num %d\n",
		    instance,inst_cnt);
	    fflush(stderr);
	}
	inst_place = inst_cnt++;
	/* save name of gate and initialize structure */
	my_inst[inst_place].inst_numb = (char *) copy_str(instance,1); 
	my_inst[inst_place].gate = -1;
	my_inst[inst_place].term_cnt = 0;
	prim_num = my_inst[inst_place].prim_num = get_prim_num(instance);


	/* get term names & direction */
    
	/* open master gate for terminal information */
	open_old_facet(&cfacet, path, "physical", "contents", "r");
	open_old_facet(&ifacet, path, "physical", "interface", "r");

	/* initialize generator for cfacet */
	o_init_gen_contents(&cfacet, OCT_TERM_MASK, &tgen);

	/* while we have terms...*/
	while (octGenerate(&tgen, &termobj) == OCT_OK) {
	    strcpy(term,termobj.contents.term.name);

	    if(strcmp(term,"GND!")==0 || strcmp(term,"Vdd!")==0)
	      continue;

	    /* get terminal */
	    term_name.type = OCT_TERM;
	    term_name.contents.term.name = term;
	    if (octGetByName(&ifacet, &term_name)  != OCT_OK) {
		fprintf(stderr,"octGetByName failed\n");
		continue;
	    }

	    /* get direction of terminal */
	    term_dir.type = OCT_PROP;
	    term_dir.contents.prop.name = "DIRECTION";
	    if (octGetByName(&term_name, &term_dir) == OCT_OK) {
		strcpy(dir,term_dir.contents.prop.value.string);
		found = 0;
		for(i=0;i<primitives[prim_num].argCnt;i++) {
		    if(strcmp(term,primitives[prim_num].arg[i])==0) {
			found = 1;
			my_inst[inst_place].term_name[i] = (char *) copy_str(term,1); 
			if(strcmp(dir,"INPUT")==0) {
			    my_inst[inst_place].term_dir[i] = TERM_IN;
			}
			else if(strcmp(dir,"OUTPUT")==0) {
			    my_inst[inst_place].term_dir[i] = TERM_OUT;
			}
			else if(strcmp(dir,"INOUT")==0) {
			    my_inst[inst_place].term_dir[i] = TERM_INOUT;
			}
			else if(strcmp(dir,"CLOCK")==0) {
			    my_inst[inst_place].term_dir[i] = TERM_IN;
			}
			else if (debug>=1) {
			    fprintf(stderr,"insert_inst: unknown direction %s for %s in %s\n",
				    dir,term,instance);
			    my_inst[inst_place].term_dir[i] = TERM_UNK;
			}
			my_inst[inst_place].term_cnt += 1;
		    }
		}
		if(found==0) {
		    fprintf(stderr,"insert_inst:  term %s in %s not found in tech file\n",
			    term,instance);
		    fflush(stderr);
		}
	    }
	    else if(debug>=1) {
		fprintf(stderr,"insert_inst: unable to get direction of terminal %s\n",term);
		fflush(stderr);
	    }
	}

	/* check # of terminals */
	if(my_inst[inst_place].term_cnt != primitives[prim_num].argCnt) {
	    fprintf(stderr,"insert_inst: # of terms in %s does not match tech file\n",
		    instance);
	    fflush(stderr);
	}
	octCloseFacet(&cfacet);
	octCloseFacet(&ifacet);
    }
    return(inst_place);
}


/*
 * insert_gate() adds the gate to the instantiation list of a cell in
 *     my_inst.
 */

int insert_gate(gate,inst_place)
  char *gate;
  int inst_place; {

  int i;

  /* these data structures are sort of gross here.. */
  my_gate[gate_cnt].next = (int )malloc(sizeof(int));
  my_gate[gate_cnt].next = -1; 
  if (my_inst[inst_place].gate == -1) {
    my_inst[inst_place].gate = gate_cnt;
    } 
  else {
    for (i=my_inst[inst_place].gate;my_gate[i].next!=-1;i=my_gate[i].next);
    my_gate[i].next=gate_cnt;
    }
  my_gate[gate_cnt].g_name = (char *)malloc(strlen (gate)+1);
  strcpy(my_gate[gate_cnt].g_name , gate);
  gate_cnt++;
  return(gate_cnt-1);
  }

/* It seems insert_output is a remnant.  Print some debug info and return */
insert_output(Tname)
char *Tname; {

  int i,Tnoexist;
  int Found;

  if(debug>0) {
    fprintf(stderr,"insert_output: Tname %s\n",Tname);
    fflush(stderr);
    }
  return;

#ifdef notdef
  Tnoexist=1;
  Found=0;
  if(*Tname!=NULL) {
    for(i=0;i<j_o;i++) {
      if(strcmp(outputs[i], Tname)==0) {
	Tnoexist = 0;
	break;
        }
      }
    if (Tnoexist) {
      outputs[j_o]=(char *)malloc(strlen(Tname)+1);
      strcpy(outputs[j_o++],Tname);
      }
    }
#endif notdef

  }
	

/* It seems insert_unk is a remnant.  Print some debug info and return */
insert_unk(Uname)
  char *Uname; {

  int i,Tnoexist;

  if(debug>0) {
    if(Uname==NULL) {
      fprintf(stderr,"insert_unk: Uname is NULL\n",Uname);
      fflush(stderr);
      }
    else {
      fprintf(stderr,"insert_unk: Uname %s\n",Uname);
      fflush(stderr);
      }
    }
  return;

#ifdef notdef
  Tnoexist = 1;
  for(i=0;i<j_o;i++) {
    if(strcmp(outputs[i], Uname)==0) {
      Tnoexist = 0;
      break;
      }
    }
  if (Tnoexist) {
    for(i=0;i<j_i;i++) {
      if(strcmp(input[i], Uname)==0) {
	return;
        }
      }
    }
  outputs[j_o]=(char *)malloc(strlen(Uname)+1);
  insert_output(Uname);
#endif notdef

  }
	
insert_supply1(S1name)
  char *S1name; {

  int i,Tnoexist;

  if(debug>0) {
    fprintf(stderr,"insert_supply1: S1name %s\n",S1name);
    fflush(stderr);
    }
  return;

#ifdef notdef
  Tnoexist=1;
  for(i=0;i<j_s1;i++) {
    if(strcmp(supply1[i], S1name)==0) {
      Tnoexist = 0;
      break;
      }
    }
  if (Tnoexist!=0) {
    supply1[j_s1]=(char *)malloc(strlen(S1name)+1);
    strcpy(supply1[j_s1],S1name);
    j_s1++;
    }
#endif notdef

  }
	
insert_supply0(S0name)
  char *S0name; {

  int i,Tnoexist;

  if(debug>0) {
    fprintf(stderr,"insert_supply0: S0name %s\n",S0name);
    fflush(stderr);
    }
  return;

#ifdef notdef
  Tnoexist = 1;
  for(i=0;i<j_s0;i++) {
    if(strcmp(supply0[i], S0name)==0) {
      Tnoexist = 0;
      break;
      }
    }
  if (Tnoexist!=0) {
    supply0[j_s0]=(char *)malloc(strlen(S0name)+1);
    strcpy(supply0[j_s0],S0name);
    j_s0++;
    }
#endif notdef

  }


check_supply(termName)
  char *termName; {

  char *lowerName;
  char *supply;
  int i;
  struct INT_term *tmp;

  lowerName=(char *)lower(termName); 
  supply=(char *)add_termName(termName,0);
  if(strncmp(lowerName,"vdd",3)==0){
    insert_supply1(supply);
    }
  else if(strncmp(lowerName,"gnd",3)==0) {
    insert_supply0(supply); 
    }
  }

write_terms(gate_place,inst_place)
  int gate_place;
  int inst_place; {

  extern int ARGFLAG;
  int loc;
  extern int outcnt,incnt;
  extern struct termStruct outTerm[], inTerm[]; 
  int pos,i,j;

  if(ARGFLAG) {
    alphaOrder(gate_place);
    }
  else {
    loc=get_primitive_loc(my_inst[inst_place].inst_numb);
    if((loc==-1) || primitives[loc].argCnt==0) {
      fprintf(stderr,"WARNING: %s has no arguments specified on Tech File \n",
         my_inst[inst_place].inst_numb);
      fprintf(stderr,"         Terminals ordered by Name\n");
      alphaOrder(gate_place);
      }
    else {
      for (i=0;i<outcnt;i++) {
        for(j=0;j<primitives[loc].argCnt ;j++) {
          pos=0;
          if(strcmp(lower(outTerm[i].type),lower(primitives[loc].arg[j]))==0) {
	    pos=j;
	    break;
            }
          else {
	    pos = -1;
            }
          }
        if (pos==-1) {
          fprintf(stderr,"WARNING: %s Argument %s: %s is not in techFile\n",
             my_inst[inst_place].inst_numb,outTerm[i].type,outTerm[i].term);
          fprintf(stderr,"         Terminals ordered by Name\n");
          alphaOrder(gate_place);
          return;
          }
        my_gate[gate_place].term[pos]=malloc(strlen(outTerm[i].term)+1);
        strcpy(my_gate[gate_place].term[pos],outTerm[i].term);
        insert_output(outTerm[i].term);
        }
      for (i=0;i<incnt;i++) {
        for(j=0;j<primitives[loc].argCnt ;j++) {
          if(strcmp(lower(inTerm[i].type),lower(primitives[loc].arg[j]))==0) {
	    pos=j;
	    break;
            }
          else { 
	    pos = -1;
            }
          }
        if (pos==-1) {
          fprintf(stderr,"WARNING: %s Argument %s: %s is not in techFile\n",
             my_inst[inst_place].inst_numb,inTerm[i].type,inTerm[i].term);
          fprintf(stderr,"         Terminals ordered by Name\n");
          alphaOrder(gate_place);
          return;
          }
        my_gate[gate_place].term[pos]=malloc(strlen(inTerm[i].term)+1);
        strcpy(my_gate[gate_place].term[pos],inTerm[i].term);
        }
      }
    }
  }


/*
 * alphaOrder()  sorts the terminal names of a cell
 */

alphaOrder(gateP)
  int gateP; {

  int j,i;
  int pos;

  for(i=0;i<outcnt;i++) {
    pos=0;
    for (j=0;j<outcnt;j++) {
      /* this looks like a waste of space */
      if((strcmp(lower(outTerm[i].type),lower(outTerm[j].type)))>0) {
	pos++;
        }
      } 
    my_gate[gateP].term[pos]=malloc(strlen(outTerm[i].term)+1);
    strcpy(my_gate[gateP].term[pos],outTerm[i].term);
    insert_output(outTerm[i].term);
    }
  for(i=0;i<incnt;i++) {
    pos=outcnt;
    for (j=0;j<incnt;j++) {
      if((strcmp(lower(inTerm[i].type),lower(inTerm[j].type)))>0) {
	pos++;
        }
      }
    my_gate[gateP].term[pos]=malloc(strlen(inTerm[i].term)+1);
    strcpy(my_gate[gateP].term[pos],inTerm[i].term);
    }
  }


/* 
 * get_primitive_loc() returns the cell number of the given cell 
 */

int get_primitive_loc(inst_name)
  char *inst_name; {
  int i;
  extern int inst;

  for(i=0;i<=inst;i++) {
    if(strcmp(primitives[i].number_name,inst_name)==0)
    return(i);
    }
  return(-1);
  }
