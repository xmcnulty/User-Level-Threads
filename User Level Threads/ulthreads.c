/* ulthreads.c
 * 
 * Implementes user level threads without the use of assembly calls.
 * These threads will instead rely on non-local jumbs and signals.
 * 
 * Created by Xavier McNulty
 * Partner: None
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void xstate_boot(void);

// Execution state (machine context) data structure
typedef struct xstate_t {
    jmp_buf jb; // jmp_buf used for context switching
    
    /* function and argument for xstate */
    void (*func) (void*);
    void *arg;
    
} xstate_t;

// global instance of xstate_t objects. Used for testing.
xstate_t *state1, *create_state;

// global starting address for state1 stack
void *state1_stack;

int num = 10; // global parameter for test function

/* Execution state control functions: save, restore, switch. */
#define xstate_save(xstate) setjmp(xstate->jb)
#define xstate_restore(xstate) longjmp(xstate->jb, 0)
#define xstate_switch(old_xstate, new_xstate) \
    if (setjmp(old_xstate->jb) == 0) \
        longjmp(new_xstate->jb, 1)


/* Private signal handler function that will be called with a signal stack.
   The execution state will be saved before returning from the handler. 
   The saved execution state will later be used to allow a non-local
   jump back to the signal handler. At this point, code will be executing
   with the alternative stack in place and the function xstate_boot will
   be called */
void xstate_capture_stack(int sig) {
	if (xstate_save(state1) == 0)
		return;
	else
		xstate_boot();
}

/* main is pointer to a possible argument to the main function.
   main_arg is a pointer to a possible argument to the main function.
   stack is the memory buffer set aside for the thread stack.
   stack_size is the size in bytes of the stack */
void xstate_create(xstate_t *xstate, void (*main) (void *), void *main_arg, void *stack, size_t stack_size) {
	/* set the xstate's function and argument */
	xstate->func = main;
	xstate->arg = main_arg;
	
	/* specifies alternate signal stack to be used as part of an execution state */
	stack_t ss = {
		.ss_size = stack_size,
		.ss_sp = stack,
		.ss_flags = 0,
	};
	
	/* specifies alternate signal stack action function */
	struct sigaction sa = {
		.sa_handler = xstate_capture_stack,
		.sa_flags = SA_ONSTACK,
	};
	
	/* set the alternate signal stack and alternate signal handler */
	stack_t old_stack; // old signal stack used for restoration.
	struct sigaction old_action; // old sigaction used for restoration.
	
	if (sigaltstack(&ss, &old_stack) == -1) { perror("sigaltstack\n"); exit(-1); }
	if (sigfillset(&sa.sa_mask) == -1) { perror("sigfillset\n"); exit(-1); }
	if (sigaction(SIGUSR1, &sa, &old_action) == -1) { perror("sigaction\n"); exit(-1); }
	
	/* mask for SIGUSR1 and old mask used for restoration */
	sigset_t mask, oldmask;
	
	// put SIGUSR1 in mask
	if (sigemptyset (&mask) == -1) { perror("sigemptyset\n"); exit(-1); }
	if (sigaddset (&mask, SIGUSR1) == -1) { perror("sigaddset\n"); exit(-1); }
	
	// block mask and save the old mask
	if (sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1) { perror("SIG_BLOCK\n"); exit(-1); }
	
	kill (getpid(), SIGUSR1); // hopefully goes into the alternate signal handler
	
	// unblock SIGUSR1
	if (sigprocmask(SIG_UNBLOCK, &mask, 0) == -1) { perror("SIG_UNBLOCK\n"); exit(-1); }
	
	// restore the old signal stack and handler.
	if (sigaltstack(&old_stack, 0) == -1) { perror("restoring old_stack\n"); exit(-1); }
	if (sigaction(SIGUSR1, &old_action, 0) == -1) { perror("restoring old_action\n"); exit(-1); }
	
		/* save the current state before going into the signal handler */
	xstate_t *create_state = (xstate_t *) malloc(sizeof(xstate_t));
	
	xstate_save(create_state);
	xstate_restore(state1);
}

/* Start the newly create thread by making a call to the main function of the thread. */
/* this test function squares a number and prints the result. */
void xstate_boot(void) {
    ((void (*) (int))state1->func) (*((int *) state1->arg));
}

/* test function */
void test (int n) {
    int i;
    
    for (i=0; i < n; i++) {
        printf("%d\n", i);
    }
}

/* Main function, used for testing. */
int main(int argc, char ** argv) {
    /* allocate space for the state1 stack */
    state1_stack = (void*) calloc(SIGSTKSZ, sizeof(void));
    
    /** allocate space for state1 */
    state1 = (xstate_t*) malloc(sizeof(xstate_t));
    
    /* create the xstate state1 */
    xstate_create(state1, (void (*) (void *)) &test, (void*) &num, state1_stack, SIGSTKSZ);
    
    //xstate_switch(create_state, state1);
}
