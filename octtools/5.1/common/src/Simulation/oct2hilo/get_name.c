#include "copyright.h"
#include "port.h"
#include "oct.h"
#include "st.h"
#include "oh.h"
#include "head.h"
#include <ctype.h>

extern int debug;

/*  
 *  get_name() strips path information (if any) and get's rid
 *    of any non-alphanumeric characters except for _ and .
 *    which are converted to _ and __ respectively.  
 *    other common chars are '[' and ']' which are ignored.
 *    A malloc'd string is returned.    this routine is used 
 *    cleaning up cell instantiation names.
 */

char *get_name(path_name)
  char *path_name; {

  char *name;
  char *retName;
  char *ptr1,*ptr2;
  char buf[128];

  /* get tail of path_name */
  name = strrchr(path_name,'/');
  if (name==NULL) 
    name = path_name;
  else 
    name++;

  /* pull out non-alnum's except . and _ */
  for(ptr1=name,ptr2=buf;*ptr1!='\0'; ptr1++) {
    if(isalnum(*ptr1)) {
      *ptr2++ = *ptr1;
      }
    /* change '.' to '__' */
    else if(*ptr1=='.'){
      *ptr2++ = '_';
      *ptr2++ = '_';
      }
    /* keep '_' */
    else if(*ptr1=='_'){
      *ptr2++ = '_';
      }
    else if(debug>=1) {
      fprintf(stderr,"Warning: non-alphanumeric %c character found in name %s\n",
         *ptr1,name);
      fflush(stderr);
      }
    }
  *ptr2 = '\0';

  retName=(char *)copy_str(buf,1);

  if (debug>=1){
    fprintf(stderr,"get_name: path_name: %s retName: %s\n",path_name,retName);
    fflush(stderr);
    }

  return (retName);
  }
