#include "copyright.h"
#include "port.h"
#include "oct.h"
#include "st.h"
#include "oh.h"
#include "head.h"

char *circuitType;
int currSize;
char *currType, *currSource, *currDrain, *currGate, *termName, *termType;
int GmuInstCnt=0;
extern octObject cfacet;


/* 
 * the GMU code is not supported at this time -- jds
 */


/*
 *  makeGmuInst() loads the Gate Matrix netlist
 */

makeGmuInst() {
  extern char *getType();
  extern char *getCircType();
  extern int getSize();
  octObject  inst, term, net, fterm,bag;
  octGenerator instgen, termgen, netgen, ftermgen;

  circuitType = (char *)malloc(1 * sizeof(char));
  strcpy(circuitType,"B");
	
  bag.type = OCT_BAG;
  bag.contents.bag.name = "INSTANCES";
  if (octGetByName(&cfacet,&bag)==OCT_OK) {
    o_init_gen_contents(&bag, OCT_INSTANCE_MASK, &instgen);
    while (octGenerate(&instgen, &inst) == OCT_OK) {
      currSize = getSize(inst.contents.instance.name);
      currType = (char *)malloc(sizeof(getType(inst.contents.instance.name)));
      currType = (char *)getType(inst.contents.instance.name); 
      o_init_gen_contents(&inst, OCT_TERM_MASK, &termgen);
      while (octGenerate(&termgen, &term) == OCT_OK) {
        o_init_gen_containers(&term, OCT_TERM_MASK, &ftermgen);
        o_init_gen_containers(&term, OCT_NET_MASK, &netgen);
	if (octGenerate(&ftermgen, &fterm) == OCT_OK) {
	  termType = (char *)malloc(sizeof(term.contents.term.name));
	  termType = term.contents.term.name;
	  termName = (char *)malloc(sizeof(fterm.contents.term.name));
	  termName = fterm.contents.term.name;
	  } 
        else if (octGenerate(&netgen, &net) == OCT_OK) {
	  if (strlen (net.contents.net.name)) {
	    termType = (char *)malloc(sizeof(term.contents.term.name));
	    termType = term.contents.term.name;
	    termName = (char *)malloc(sizeof(net.contents.net.name));
	    termName = net.contents.net.name;
	    } 
          else {
	    termType = (char *)malloc(sizeof(term.contents.term.name));
	    termType = term.contents.term.name;
	    termName = (char *)malloc(sizeof(term.contents.term.name));
	    termName = term.contents.term.name;
	    }
	  }
	switch(termType[0]) {
	  case 'S': currSource = (char *)malloc(sizeof(termName));
	            currSource = termName;
		    break;;
	  case 'D': currDrain = (char *)malloc(sizeof(termName));
		    currDrain = termName;
		    break;;
  	  case 'G': currGate = (char *)malloc(sizeof(termName));
		    currGate = termName;
		    break;;
	  }

        }
      currDrain=(char *)get_termName(currDrain);
      currGate=(char *)get_termName(currGate);
      if(strcmp(circuitType ,"B")==0) {
        circuitType = (char *)getCircType(currSource,currDrain,currType);
        insertGMU(currDrain,currGate,currSize);
	}
      else {
        if (strcmp(circuitType,currType)==0) {
  	  insertGMU(currDrain,currGate,currSize);
          }
        }
      }
    }
  }



int getSize(instName)
  char *instName; {

  int thesize=0;
/*  int i=0,k=0,j;

  lenght=(char *)malloc(5*sizeof(char));
  width=(char *)malloc(5*sizeof(char));

  while(instName[i]!='x') {
    if(isdigit(instName[i])) {
      while(isdigit(instName[i++])) {
  	width[k++]=instName[i];
        }
      }  
    else { 
      i++;
      }
    }
  j=i;
  k=0;

  while(instName[j]!='\0') {
    if(isdigit(instName[j])) {
      while(isdigit(instName[j++])) {
   	lenght[k++]=instName[j];
        }
      } 
    else { 
      j++;
       }
    } */
  return thesize;
  }

char *getType(GT_instName)
  char *GT_instName; 
{

    char *junk;
    char *name;
    junk = (char *)malloc(10 * sizeof(char));
    name = (char *)malloc(sizeof(char));
    sscanf(GT_instName,"%c%s",name,junk);
    return name ;
}

char *getCircType(source,drain,type)
  char *source;
  char *drain;
  char *type; 
{

    char *resType;
    resType = (char *)malloc(sizeof(char));

    if (strcmp(type,"n")==0) {
	if (strcmp(source,"GROUND")==0 && drain[0]=='[') {
	    strcpy(resType,"n");  
	    printf("CIRCUIT IS NOR TYPE\n");
	} 
	else {
	    strcpy(resType,"p"); 
	    printf("CIRCUIT IS NAND TYPE\n");
	}
    }
    else {
	if (strcmp(source,"Vdd")==0 && drain[0]=='[') {
	    strcpy(resType ,"p");
	    printf("CIRCUIT IS NAND TYPE\n");
	} 
	else {
	    strcpy(resType ,"n");
	    printf("CIRCUIT IS NOR TYPE\n");
	}
    }
    return resType;
}



insertGMU(IG_instName,outGMU,size)
  char *IG_instName;
  char *outGMU;
  int size; 
{

    int i,j,GmuNotThere=1,OutNotThere=1;

    for(i=0;i<GmuInstCnt;i++) {
	if (strcmp(IG_instName,gmuInstance[i].instName)==0) {
	    for(j=0;j<gmuInstance[i].numInst;j++) {
		if(strcmp(gmuInstance[i].output[j],outGMU)==0) {
		    OutNotThere=0; 
		}
	    }
	    if(OutNotThere) {
		gmuInstance[i].output[gmuInstance[i].numInst]=(char *)malloc(sizeof(outGMU));
		gmuInstance[i].output[gmuInstance[i].numInst]=outGMU;
		gmuInstance[i].numInst=gmuInstance[i].numInst+1;
	    }
	    GmuNotThere = 0;
	    break;
	}
    }
    if(GmuNotThere) {
	gmuInstance[GmuInstCnt].instName = (char *)malloc(sizeof(IG_instName));
	gmuInstance[GmuInstCnt].instName=IG_instName;
	gmuInstance[GmuInstCnt].output[0] = (char *)malloc(sizeof(outGMU));
	gmuInstance[GmuInstCnt].output[0]=outGMU;
	gmuInstance[GmuInstCnt].numInst = 1;
	if(strcmp(currType,"n")==0) {
	    gmuInstance[GmuInstCnt].ntypesize=size;
	} else {
	    gmuInstance[GmuInstCnt].ptypesize=size;
	}	
	GmuInstCnt++;
    }
}



insertSize(IS_instName,size) 
  char *IS_instName;
  int size; 
{

    int i,GmuNotThere=1;

    for(i=0;i<GmuInstCnt;i++) {
	if (strcmp(IS_instName,gmuInstance[i].instName)==0) {
	    if(strcmp(currType ,"n")==0) {
		gmuInstance[i].ntypesize=size;
	    } else {
		gmuInstance[i].ptypesize=size;
	    }	
	    GmuNotThere=0;
	}
    }
    if(GmuNotThere) {
	gmuInstance[GmuInstCnt].instName = (char *)malloc(sizeof(IS_instName));
	gmuInstance[GmuInstCnt].instName=IS_instName;
	gmuInstance[GmuInstCnt].numInst = 1;
	if(strcmp(currType,"n")==0) {
	    gmuInstance[GmuInstCnt].ntypesize=size;
	} else {
	    gmuInstance[GmuInstCnt].ptypesize=size;
	}	
	GmuInstCnt++;
    }
}

