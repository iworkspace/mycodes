/* https://swtch.com/~rsc/regexp/nfa.c.txt */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
	//0-255 ascii ouput
	//256 match
	//257 split
	Match = 256, 
	Split = 257
};

struct State {
	int c;
	struct State *out,*out1;
        int lastlist;	
};
typedef struct State State; 
State matchstate = { Match } ;//init state is match any eat char.

/* Allocate and initialize State */
State*
state(int c, State *out, State *out1)
{
	State *s;
	
	nstate++;
	s = malloc(sizeof *s);
	s->lastlist = 0;
	s->c = c;
	s->out = out;
	s->out1 = out1;
	return s;
}

char *re2post(char *re)
{
	static char buf[8000];
	char *regex = buf;
	int nalt=0 , natom = 0;
	struct {
		int nalt;
		int natom;
	} parent [100], *p = parent;

	if(strlen(re) >= sizeof(buf)/2 ){
		return NULL;
	}
#define concatenate(pending) do{ \
				if(natom > 1) {\
					natom--;\
					*regex++ = '.';\	
				}\
				while( pending && --natom) {\
				       	*regex++ = '.';\
				}\
			} while(0)

#define splitconcat() do{ \
				while(nalt ) {\
					nalt--;\
				       	*regex++ = '|';\
				}\
			} while(0)


#define subregex_in() do{ \
			p->nalt = nalt;\
			p->natom = natom;\
			p++;\
			nalt = 0;\
			natom = 0;\
			} while(0)

#define subregex_out() do{\
			--p;\
			nalt = p->nalt;\
			natom = p->natom ;\
			}while(0)

#define prefix_exist_check() do { if( natom < 1 ) return NULL; } while(0)

	for(;*re;re++) {
		switch (*re) {
			default:{
				concatenate(0);
				*regex++ = *re;
				natom++;
				break;	
			}
			case '(' :{ //start an sub regex
				concatenate(0);
				subregex_in();
				break;
			}
			case ')':{
				if(p == parent){
					return NULL;
				}
				prefix_exist_check();
				concatenate(1);
				splitconcat();
				subregex_out();
				natom++;
				break;
			}
			case '|':{
				prefix_exist_check();
				concatenate(1);
				nalt++;
				break;	 
			}
			case '+': case '?': case '*':{
				prefix_exist_check();
				*regex++ = *re;
				break;			     
			}
		}
	}
	//subreg is done?
	if(p != parent){
		return NULL;
	}

	concatenate(1);
	splitconcat();

	*regex = '\0';
	return buf;
}

typedef union Ptrlist;
typedef struct Frag Fra;

union Ptrlist {
	Ptrlist *next;
	State *s;
};

struct Frag {
	State *start;
	Ptrlist *out;	
};

/* Initialize Frag struct. */
Frag
frag(State *start, Ptrlist *out)
{
	Frag n = { start, out };
	return n;
}

/* Create singleton list containing just outp. */
Ptrlist*
list1(State **outp)
{
	Ptrlist *l;
	
	l = (Ptrlist*)outp;
	l->next = NULL;
	return l;
}

/* Join the two lists l1 and l2, returning the combination. */
Ptrlist*
append(Ptrlist *l1, Ptrlist *l2)
{
	Ptrlist *oldl1;
	
	oldl1 = l1;
	while(l1->next)
		l1 = l1->next;
	l1->next = l2;
	return oldl1;
}

/* Patch the list of states at out to point to start. */
void
patch(Ptrlist *l, State *s)
{
	Ptrlist *next;
	
	for(; l; l=next){
		next = l->next;
		l->s = s;
	}
}


State *post2nfa(char *regx)
{
	Frag stack[1000],*stackp = stack,e1,e2,e;
	State *s;
#define push(s) *stackp++ = s
#define pop() *--stackp
	for(p=regx;*p;p++){
		switch(*p) {
			default: {
				s = state(*p,NULL,NULL);//new state.
				push(frag(s,list1(&s->out)));
				break;	 
			}
			case '.': { /* catenate */
				e2 = pop();
				e1 = pop();
				patch(e1.out,e2.start);
				push(frag(e1.start,e2.out));
				break;
			}
			case '|': {
				e2 = pop();
				e1 = pop();
				s = patch(Split,e1.start,e2.start);
				push(frag(s,append(e1.out,e2.out)));//append(e2.out,e1.out)
				break;	  
			}
			case '?': {
				e = pop();
		      		s = state(Split,e.start,NULL);		
				push(frag(s,append(e.out,list1(&s->out1))));
				break;
			}
			case '*': {
				e = pop();	
				s = state(Split,e.start,NULL);
				patch(e.out,s);
				push(frag(e.out,list1(&s->out1)));
				break;  
			}
		}
		
	}

	e = pop();
	if(stackp != stack ) 
		return NULL;

	patch(e.out,&matchstate);
	return e.start;

#undef pop
#undef push
}


typedef struct List List;
struct List
{
	State **s;
	int n;
};
List l1, l2;
static int listid;

void addstate(List *l,State *s)
{
	if(!s || s->lastlist == listid ) 
		return;
	s->lastlist = lastid;
	if(s->c == Split) {
		addstate(l,s->out);
		addstate(l,s->out1);
		return;
	}
	l->s[l->n++] = s;
}

int main(int argc,char *argv[])
{
	int i;
	char *regex;
	State *start;

	if(argc < 3 ) {
		printf("%s regexp text ... \n",argv[0]);
		return -1;
	}

	regex = re2post(argv[1]);
	if(regex){
		printf("the regex is : %s \n",regex);
	}else{
		printf("build regex err\n");
		return -1;
	}

	start = post2nfa(regex);
	if(start == NULL){
		fprintf(stderr, "error in post2nfa %s\n", regex);
		return 1;
	}
	
	//dump state list recursion.
	
	l1.s = 	
	

}
