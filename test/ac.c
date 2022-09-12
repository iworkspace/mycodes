#include <stdio.h>

#ifdef DEBUG
#define debug(fmt,...) do { printf(fmt,##__VA_ARGS__);} while(0)
#else
#define debug(fmt,...) do {} while(0)
#endif

struct list_head {
	struct list_head *prev,*next;
};

void list_init(struct list_head *l)
{
	l->prev = l->next = l;
}

void list_add(struct list_head *l,struct list_head *n)
{
	n->prev = l->prev;
	n->next = l;
	
	l->prev->next = n;
	l->prev = n;
}

int list_is_empty(struct list_head *l)
{
	return l == l->next;
}

#define list_foreach(l,i) for(i=((struct list_head *)(l))->next;i!=l;i=((struct list_head*)(i))->next)

struct output {
	struct list_head list;
	char state;
	char keys[0];
};

struct state{
	struct state *g[128];//goto
	char c;//tire_tree
	int  f;//fail_function
	struct list_head o; //output
};

#define MAX_STATES (100)
static int states_num = 0;
struct state states[MAX_STATES];
#define ROOT_STATES (&states[0])
#define LAST_STATES (&states[states_num])
//#define GET_STATES(S) ((((int)S) - ((int)ROOT_STATES))/sizeof(struct state))
#define GET_STATES(S)  (((char*)(S) - (char*)(ROOT_STATES))/sizeof(struct state))
#define ADDR_STATES(s) (&states[s])

//simple stack & fifo queue
static void* ac_stack[MAX_STATES];
static int ac_stack_index = 0;
static int ac_queue_head = 0;
static int ac_queue_tail = 0;
#define AC_STACK_PUSH(o) { ac_stack[ac_stack_index+1 < MAX_STATES ? ++ac_stack_index : 0 ] = o; }
#define AC_STACK_POP()	( ac_stack[ac_stack_index-1 >=0 ? ac_stack_index-- : 0 ] )
#define AC_RESET() (ac_stack_index=0)
#define AC_STACK_ISEMPTY() (ac_stack_index == 0)

#define AC_QUEUE_RESET() (ac_queue_head = ac_queue_tail = 0 )
#define AC_QUEUE_ENQ(e)	(ac_stack[ac_queue_tail++ % MAX_STATES] = e)
#define AC_QUEUE_DEQ()  (ac_stack[ac_queue_head++ % MAX_STATES])
#define AC_QUEUE_ISEMPTY() (ac_queue_head == ac_queue_tail)
/*
	construction goto function
	
	Input: Set of keywords K = {y1,y2,....yk}
	Output: Goto function g and a partially computed output function output.
	
	Method: 
		We assume output(s) is empty when state s is first created, 
	and g(s, a) = fail if a is undefined or if g(s, a) has not yet been defined.
	The procedure enter(y) inserts into the goto graph a path that spells out y.

	begin
		newstate <-- 0
		for i <-- 1 until k do enter(yi)
		for all a such that g(0, a) == fail do g(0,a) <-- 0
	end
	
	procedure enter(a1a2...am)
	begin
		state <--0; j <-- 1
		while g(state,aj) != fail do
			begin
				state <-- g(state,aj)
				j <-- j+1
			end
		for p <-- j until m do 
			begin 
				newstate <-- newstate + 1
				g(state,ap) <-- newstate
				state <-- newstate
			end
		output(state) <-- {a1a2...am}
	end
	
*/

void ac_add_pat(char *pat)
{
	struct state *s,*new;
	char *p;
	struct output *o;
	int len = strlen(pat);
		
	for(p = pat,s = ROOT_STATES;*p;s = s->g[*p++])
	{
		if( !s->g[*p]){ //new states
			states_num++;
			s->g[*p] = LAST_STATES;
			s->g[*p]->c = *p;
		}
	}
	
	//todo same pat ...
	o = malloc(sizeof(*o)+len+1);
	list_init(o);
	sprintf(o->keys,"%s",pat);	
	o->state = GET_STATES(LAST_STATES);
	list_add(&LAST_STATES->o,o);
}

void ac_tire_tree_dump(struct state *S)
{
	int i,o;
	struct list_head *iter;

	o = (GET_STATES(S) <<8 )|S->c;

	if(S!=ROOT_STATES)
		AC_STACK_PUSH(o);
	
	if(!list_is_empty(&S->o)) {
		for(i=1;i<=ac_stack_index;i++){
			o=(int)ac_stack[i];
			debug("%d(%c)->",o>>8,o&0xff);
		}
		if(S->f)
			debug("[F]:%d",S->f);
		debug("[o]:");
		list_foreach(&S->o,iter) {
			debug("%d:%s ",((struct output*)iter)->state,((struct output*)iter)->keys);
		}
		debug("\n");
	}

	for(i=0;i<128;i++){
		if(S->g[i]){
			ac_tire_tree_dump(S->g[i]);
		}
	}
	
	AC_STACK_POP();
}

/*
 * Construction of the failure function
 * Input: Goto function g and output function o
 * Output: Failure function f and output function o
 *
 * Method:
 * begin
 * 	queue <-- empty
 * 	for each a such that g(0,a) =s != 0 do
 * 		begin
 * 			queue <-- queue U {s}
 * 			f(s)<--0
 * 		end
 *
 * 	while queue != empty do
 * 		begin
 * 			let r be the next state in queue
 * 			queue <-- queue - {r}
 * 			for each a such that g(r,a) = s != fail do
 * 				begin
 * 					queue <-- queue U {s}
 * 					state <-- f(r)
 *					
 *					#recursion find father's failure function.
 * 					while g(state,a) == fail do state <-- f(state)
 * 					#hold
 * 					f(s) <-- g(state,a)
 *					output(s) <-- output(s) U output(f(s))
 *				end
 *		end
 * end
 *
 * */
void ac_calc_fail_function()
{
	struct state *S,*F,*R;
	struct output *o,*iter;
	int i;

	//init empty set.
	AC_QUEUE_RESET();
	S = ROOT_STATES;
	for(i=0;i<128;i++){
		if(S->g[i]){
			AC_QUEUE_ENQ(S->g[i]);
			//S->g[i].f = 0;
		}
	}

	//dfs
	while( ! AC_QUEUE_ISEMPTY()  ){
		R = AC_QUEUE_DEQ();

		for(i=0;i<128;i++){
			S = R->g[i];
			if(!S){
				continue;
			}

			AC_QUEUE_ENQ(S);
			F=ADDR_STATES(R->f);
			while ( (!F->g[i]) && ( F != ROOT_STATES ) ){
				F=ADDR_STATES(F->f);
			}
			F = F->g[i];
			if(!F){
				S->f = 0;
				continue;
			}
			S->f = GET_STATES(F);
#if 1
			if(!list_is_empty(&F->o)){
				//todo same entry.
				list_foreach(&F->o,iter){
					o = malloc(strlen(iter->keys)+1+sizeof(struct output));
					list_init(&o->list);
					sprintf(o->keys,"%s",iter->keys);
					o->state = iter->state;
					list_add(&S->o,o);
					debug("output list add: %d %s[%d] \n",GET_STATES(S),iter->keys,iter->state);
				}
			}
#endif
		}	
			
	
	}

}

void ac_match_machine_lunch(char *text)
{
	struct state *S;
	struct output *iter;
	char *p;
	debug("%s\n",text);
	for(p=text,S=ROOT_STATES; *p; ){
		if ( S->g[*p] )	{
			S = S->g[*p];		
			if(!list_is_empty(&S->o)){
				list_foreach(&S->o,iter){
					debug("%-*s\^%s\n",p-text," ",iter->keys);
				}	
			}
			p++;
		}else{
			S = ADDR_STATES(S->f);
			if(S == ROOT_STATES)
				p++;
		}
	}
}	

int main()
{
	int i;
	char *pats[] = {
		"he",
		"she",
		"his",
		"hers",
		NULL
	};
	
	//init 
	for(i = 0 ; i < MAX_STATES; i++)
	{
		memset(ADDR_STATES(i),0,sizeof(struct state));
		list_init(&(ADDR_STATES(i)->o));
	}	

	//build trie_tree and goto_tbl (ori_output tbl)
	for(i=0;i<sizeof(pats)/sizeof(char *) -1;i++)
		ac_add_pat(pats[i]); 
	
	ac_tire_tree_dump(ROOT_STATES);
	
	//calc_fail_function
	ac_calc_fail_function();	

	debug("----------------------\n");
	ac_tire_tree_dump(ROOT_STATES);
	
	//ac match test
	char *teststr = "ushers";
	ac_match_machine_lunch(teststr);	
}
