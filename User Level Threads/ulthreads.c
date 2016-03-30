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

// Execution state (machine context) data structure
typedef struct xstate_st {
    void (*main) (void *); // function to be execute by the thread
	void *main_arg; // arguments to passed to the main function
	void *stack; // stack utilized by the thread
	size_t stack_size; // size of the stack
	
	int setjmp_ret; // return value of set_jmp
	
    jmp_buf jb;
} xstate_t;

/* Execution state control functions: save, restore, switch. */
#define xstate_save(xstate) xstate->setjmp_ret = setjmp(xstate->jb)
#define xstate_restore(xstate) longjmp(xstate->jb, xstate->setjmp_ret)
void xstate_switch(xstate_t *xstate);

/* main is pointer to a possible argument to the main function.
   main_arg is a pointer to a possible argument to the main function.
   stack is the memory buffer set aside for the thread stack.
   stack_size is the size in bytes of the stack */
void xstate_create(xstate_t *xstate, void (*main) (void *), void *main_arg, void *stack, size_t stack_size) {
    // create the xstate
    xstate = (xstate_t*) calloc(1, sizeof(xstate_t));
}

/* Private signal handler function that will be called with a signal stack.
   The execution state will be saved before returning from the handler. 
   The saved execution state will later be used to allow a non-local
   jump back to the signal handler. At this point, code will be executing
   with the alternative stack in place and the function xstate_boot will
   be called */
void xstate_capture_stack(int sig) {
    printf("in xstate_capture_stack: %d\n", sig);
    fflush(stdout);
}

/* Start the newly create thread by making a call to the main function of the thread. */
/* this test function squares a number and prints the result. */
void xstate_boot(int n) {
    int square = n*n;
    
    printf("%d squared is: %d\n", n, square);
    fflush(stdout);
}

// global instance of xstate_t objects. Used for testing.
xstate_t *state1;

// global starting address for state1 stack
void *state1_stack;

int num = 3; // global parameter for xstate_boot

int main(int argc, char ** argv) {
    /* create the stack for state1 */
    state1_stack = (void*) malloc(MINSIGSTKSZ);
    
    /* Specifies state1_stack as the alternate stack on which the SIGUSR1
        signal is to be processed. */
    stack_t state1_ss = {
        .ss_size = MINSIGSTKSZ,
        .ss_sp = state1_stack
    };
    
    /* Specifies xstate_capture_stack as the alternal handler function to be called when
        a signal is processed. */
    struct sigaction state1_sa = {
        .sa_handler = xstate_capture_stack,
        .sa_flags = SA_ONSTACK
    };
    
    /* set state1_stack as the alternate stack used for signal processing. */
    sigaltstack(&state1_ss, 0);
    
    /* set the sigaction that calls xstate_capture_stack as the function to
       process a signal. */
    sigaction(SIGUSR1, &state1_sa, 0);
    
    /* Creates signal sets, one for the SIGUSR1 with the overriden sig_action
       and anotherone containing the old signal state */
    sigset_t usr1_set, oldset;
    
    /* fill oldset with the current signal set */
    sigfillset(&oldset);
    
    sigemptyset(&usr1_set); // empty the usr1 signal set
    sigaddset(&usr1_set, SIGUSR1); // add SIGUSR1 ot usr_set
  
    sigprocmask(SIG_BLOCK, &usr1_set, &oldset);
    kill(0, SIGUSR1);
    kill(0, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &usr1_set, &oldset);
}