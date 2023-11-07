#include <stdio.h>
#include <ctype.h>
#include "head.h"
#include "copyright.h"
#include "oct.h"
#include "port.h"
#include "st.h"

extern char *buffer;
extern int  debug;
struct INT_terms INT_terms;

char util_tolower(ch)
  char ch; {

  char rch;

#ifdef sun
  if (isupper(ch))
    rch = tolower(ch);
  else
    rch = ch;
#else
  rch = tolower(ch);
#endif sun

  return rch;
  }


/* 
 *  lower() returns malloc'd lower case string 
 */

char *lower(string)
    char *string; 
{
    int i;
    char *low;

    low=(char *)malloc(strlen(string)+1);
    for(i=0;string[i]!=NULL;i++)  
      low[i]= util_tolower(string[i]);
    low[i]='\0';
    return(low);
}

/* 
 *  get_prim_num() looks up gate in table from tech file 
 *      and returns it's location 
 */

int get_prim_num(thename)
  char *thename; {
  int i;
  extern int inst;

  for (i=0;i<inst;i++) 
    if (strcmp(primitives[i].number_name,thename)==0) 
      return(i);
  return(-1);
  }



/* 
 * makeGateName() makes a gate name from running gateNmae through get_name 
 *   and prepending a G.  frees space from get_name call 
 */

char *makeGateName(gateName)
  char *gateName; {
  char *real_gateName;                 
  char *final_gateName;

  real_gateName=(char *)get_name(gateName);
  final_gateName = (char *)malloc(strlen(real_gateName) + 2);

  strcpy(final_gateName,"G");
  strcat(final_gateName,real_gateName);
  free(real_gateName);
  if(debug>=1) {
    fprintf(stderr,"makeGateName: gateName %s final_gateName %s \n",
      gateName,final_gateName);
    fflush(stderr);
    }
  return final_gateName;
  }


/* 
 * no_dots() returns 1 if no dots are found in str, otherwise 0
 */

no_dots(str)
  char *str; {
  int i;
  for(;*str!='\0';str++)
    if(*str=='.') return 0;
  return 1;
  }

/* 
 * fix_termName() checks for existence of termName and returns entry if found.  
 *     if not found, it prepends a T and returns it. the name is converted
 *     as noted below
 *
 *
 * format mapping is: 
 *   incoming terminal names          outgoing terminal name 
 *     foo                              Tfoo
 *     foo<0>                           Tfoo0
 *     foo<0>.1                         Tfoo_0_1
 *     [345]                            T345
 *     [345].1                          T345_1
 * format mapping should be:  (when vectors are implemented)
 *   incoming terminal names          outgoing terminal name 
 *     foo                              Tfoo
 *     foo<0>                           Tfoo<0>
 *     foo<0>.1                         Tfoo_0_1
 *     [345]                            T345
 *     [345].1                          T345_1
 */
  
char *fix_termName(termName)
  char *termName; {
  char buf[128];
  char *final;
  char *ptr1,*ptr2;
  
  char end_char; 
  
  end_char = termName[strlen(termName)-1];
  /* in the future this should leave solo <>'s alone */
  for (ptr1=termName,ptr2=buf; *ptr1 != '\0'; ptr1++) {
    if (isalnum(*ptr1)) 
      *ptr2++ =  util_tolower(*ptr1);
    else if(*ptr1=='.')
      *ptr2++ = '_';
    else if((*ptr1=='<') && (end_char == '>')) {
      if (prog==PROG_HILO)
        *ptr2++ = '[';
      else
        *ptr2++ = '(';
      }
    else if(*ptr1=='<')
      *ptr2++ = '_';
    else if((*ptr1=='>') && (end_char != '>')){
      *ptr2++ = '_';
      ptr1++;
      }
    else if(*ptr1=='>') 
      if (prog==PROG_HILO)
        *ptr2++ = ']';
      else /* VHDL */
        *ptr2++ = ')';
    }
  *ptr2 = '\0';

  final = (char *) malloc(strlen(buf) + 2);
  strcpy(final,"T");
  strcat(final,buf);

  if(debug>0){
    fprintf(stderr,"fix_termName: termName %s final %s\n",termName,final);
    fflush(stderr);
    }
  return final;
  }



/*
 * add_termName() adds the termName to the list of terminals 
 */

struct INT_term *add_termName(termName,list)
  char *termName;  
  int  list;  {
  char *fixed;
  struct INT_term *tmp,*tmp2;
  struct INT_term **list_head;
  struct INT_term **list_cur;
  struct INT_term **all_head;
  struct INT_term **all_cur;

  all_head = (struct INT_term **)&INT_terms.all_head;
  all_cur  = (struct INT_term **)&INT_terms.all_cur;

  /* get 'fixed' terminal name */
  fixed = fix_termName(termName);

  /* is it already defined?  if so, inc it's usage*/
  for(tmp = *all_head;tmp!=NULL;tmp=tmp->next_all){
    if(strcmp(fixed,tmp->name)==0) {
      tmp->cnt += 1;
      free(fixed);
      return tmp;
      }
    }

  switch(list) {
    case TERM_IN:     list_head = (struct INT_term **)&(INT_terms.in_head);
                      list_cur  = (struct INT_term **)&(INT_terms.in_cur);
                      break;
    case TERM_OUT:    list_head = (struct INT_term **)&(INT_terms.out_head);
                      list_cur  = (struct INT_term **)&(INT_terms.out_cur);
                      break;
    case TERM_INOUT:  list_head = (struct INT_term **)&(INT_terms.inout_head);
                      list_cur  = (struct INT_term **)&(INT_terms.inout_cur);
                      break;
    case TERM_SUPPLY0:list_head = (struct INT_term **)&(INT_terms.supply0_head);
                      list_cur  = (struct INT_term **)&(INT_terms.supply0_cur);
                      break;
    case TERM_SUPPLY1:list_head = (struct INT_term **)&(INT_terms.supply1_head);
                      list_cur  = (struct INT_term **)&(INT_terms.supply1_cur);
                      break;
    }


  /* make new term */
  tmp = (struct INT_term *) malloc(sizeof(struct INT_term));
  tmp->name = fixed;
  tmp->type = list;
  tmp->cnt = 1;
  tmp->next = NULL;
  tmp->next_all = NULL;

  if(*all_head == NULL) {
    *all_head = tmp;
    *all_cur = tmp;
    }
  else {
    (*all_cur)->next_all = tmp;
    *all_cur = tmp;
    }

  if(*list_head == NULL) {
    *list_head = tmp;
    *list_cur = tmp;
    }
  else {
    (*list_cur)->next = tmp;
    *list_cur = tmp;
    }
  
  return tmp;
  }


/*
 * get_termName() looks up termName in the specified list
 */

struct INT_term *get_termName(termName,list)
  char *termName;   
  int  list;  {
  struct INT_term *tmp;
  struct INT_term *list_head;

  list_head = (struct INT_term *) INT_terms.all_head;

  for(tmp=list_head;tmp!=NULL;tmp=tmp->next){
    if(strcmp(termName,tmp->name)==0) {
      return tmp;
      }
    }
  return(0);
  }


/*
 *  o_init_gen_containers() just calls octInitGenContainers 
 */

o_init_gen_containers(object, mask, generator)
  octObject *object;
  octObjectMask mask;
  octGenerator *generator; {

  if (octInitGenContainers(object, mask, generator) != OCT_OK) {
    octError("cannot oct init gen contents");
    exit(-1);
    }
  }


/* 
 * o_init_gen_contents() just calls octInitGenContents 
 */

o_init_gen_contents(container, mask, generator)
  octObject *container;
  octObjectMask mask;
  octGenerator *generator; {

  if (octInitGenContents(container, mask, generator) != OCT_OK) {
    octError("cannot oct init gen contents");
    exit(-1);
    }
  }


/* 
 *  alnum() returns a temporary string good until the next reference 
 *      to alnum, so 'take a picture, it lasts longer' 
 */

char *alnum(str)
  char *str; {
  char buf[1024];
  char *ptr;

  for(ptr=buf;*str!='\0';str++) 
    if(isalnum(*str))
      *ptr++ = *str;
  *ptr='\0';
  return(buf);
  }
   

/* 
 *  copy_str(str,xtra):  returns string with ptr to malloc'ed space
 *   the size of str plus extra that contains a copy of str 
 */
char *copy_str(str,xtra)
  char *str;
  int  xtra;  {
   
  char *ptr;

  if(str==NULL) {
    fprintf(stderr,"NULL pointer passed to copy_str. exiting\n");
    exit(-1);
    }
  if(xtra<0) {
    fprintf(stderr,"negative number passed to copy_str. exiting\n");
    exit(-1);
    }
  ptr = (char *) malloc(strlen(str)+xtra);
  strcpy(ptr,str);
  return ptr;
  }


/*
 * IO_2_sortlist() converts the IO terminal list to a sorlist usable
 *     by sortlist() and vectorize()
 */

struct sortlist *IO_2_sortlist(head)
  struct IO_term *head;  {

  struct IO_term *tmp;
  struct sortlist *ent,*sl_head,*sl_cur;

  
  sl_head = sl_cur = NULL;
  if(head==NULL)
    return NULL;
  for(tmp=head;tmp!=NULL;tmp=tmp->next) {
    ent = (struct sortlist *) malloc(sizeof(struct sortlist));
    ent->next = NULL;
    ent->str = copy_str(tmp->termname,1);
    if(sl_head == NULL) {
      sl_head = ent;
      sl_cur = ent;
      }
    else {
      sl_cur->next = ent;
      sl_cur = ent;
      }
    }
  return sl_head;
  }


/*
 * INT_2_sortlist() converts the INT terminal list to a sorlist usable
 *     by sortlist() and vectorize()
 */

struct sortlist *INT_2_sortlist(head)
  struct INT_term *head;  {

  struct INT_term *tmp;
  struct sortlist *cur,*sl_head,*sl_cur;

  
  sl_head = sl_cur = NULL;
  if(head==NULL)
    return NULL;
  for(tmp=head;tmp!=NULL;tmp=tmp->next) {
    cur = (struct sortlist *) malloc(sizeof(struct sortlist));
    cur->next = NULL;
    cur->str = copy_str(tmp->name,1);
    if(sl_head == NULL) {
      sl_head = cur;
      sl_cur = cur;
      }
    else {
      sl_cur->next = cur;
      sl_cur = cur;
      }
    }
  return sl_head;
  }


/*
 * free_sortlist() completely frees a sortlist 
 */

free_sortlist(head)
  struct sortlist *head; {

  struct sortlist *cur,*tmp;

  for(cur=head;cur!=NULL;) {
    free(cur->str);
    tmp = cur;
    cur = cur->next;
    free(tmp);
    }
  }


/*
 *  sort_list() alphabetically sorts a sortlist and returns a pointer
 *      to the sorted list
 */

sort_list(head)
  struct sortlist **head;  {

  struct sortlist *cur,*next,*prev,*tmp2,fake_root;
  int flag;

  if(*head!=NULL)  {
    flag = 1;
    fake_root.next = *head;
    while( flag == 1) {
      flag = 0;
      for(cur = fake_root.next,prev= &fake_root;cur->next!=NULL;prev=prev->next) {
        if(strcmp(cur->str,(cur->next)->str)<0){
          next = cur->next;
          prev->next = next;
          cur->next = next->next;
          next->next = cur;
          flag = 1;
          }
        else cur=cur->next;
        }
      }
    *head = fake_root.next;
    }
  }
      
  


/*
 *  vectorize() vectorizes  a sortlist and returns a pointer
 *      to the vectorized list
 */

struct sortlist *vectorize(head) 
  struct sortlist *head; {

  char *ptr1,*ptr2;
  int i,j,k;
  struct sortlist *tmp,*tmp2,*tmp3,*new_head,*new_cur ;
  char buf1[128];
  char buf2[128];
  int vec_hi,vec_lo,vec;

  new_head = new_cur = NULL;
  /* write output signal names */
  for (tmp=head;tmp!=NULL;tmp=tmp->next) {
    if((strrchr(tmp->str,']'))!=NULL) {
      vec_hi = vec_lo = -1;
      strcpy(buf1,tmp->str);
      ptr1 = strchr(buf1,'[');
      *ptr1++ = '\0';
      ptr2 = strrchr(ptr1,']');
      *ptr2 = '\0';
      vec = atoi(ptr1);
      vec_hi = vec_lo = vec;
      for(tmp2=tmp;tmp2->next!=NULL;tmp2=tmp2->next) {
        if(strrchr((tmp2->next)->str,']')==NULL) {
          tmp = tmp2;
          break;
          }
        strcpy(buf2,(tmp2->next)->str);
        ptr1 = strchr(buf2,'[');
        *ptr1++ = '\0';
        ptr2 = strrchr(ptr1,']');
        *ptr2 = '\0';
        vec = atoi(ptr1);
        if(strcmp(buf1,buf2)!=0) {
          tmp = tmp2;
          break;
          }
        if(vec>vec_hi) vec_hi = vec;
        if(vec<vec_lo) vec_lo = vec;
        }
      if(vec_hi != vec_lo)
        sprintf(buf2,"%s[%d:%d]",buf1,vec_hi,vec_lo);
      else
        sprintf(buf2,"%s[%d]",buf1,vec_hi);
      tmp = tmp2;
      }
    else
      strcpy(buf2,tmp->str);

    tmp3 = (struct sortlist *) malloc(sizeof(struct sortlist));
    tmp3->next = NULL;
    tmp3->str =  copy_str(buf2,1);
   
    if(new_head==NULL) {
      new_head = tmp3;
      new_cur  = tmp3;
      }
    else {
      new_cur->next = tmp3;
      new_cur = tmp3;
      }
    }
  free_sortlist(head);
  return new_head;
  }
              



