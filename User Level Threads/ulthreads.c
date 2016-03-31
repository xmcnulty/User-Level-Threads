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

// Execution state (machine context) data structure
typedef struct xstate_st {
    jmp_buf jb;
} xstate_t;

/* Execution state control functions: save, restore, switch. */
#define xstate_save(xstate) xstate->setjmp_ret = setjmp(xstate->jb)
#define xstate_restore(xstate) longjmp(xstate->jb, xstate->setjmp_ret)
#define xstate_switch(old_xstate, new_xstate) \
    if (setjmp(old_xstate->jb) == 0) \
        longjmp(nex_xstate->jb, 1)

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
    
    exit(0);
}

/* Start the newly create thread by making a call to the main function of the thread. */
/* this test function squares a number and prints the result. */
void xstate_boot(void) {
    
}

void test (int n) {
    int i;
    
    for (i=0; i < n; i++) {
        printf("%d\n", i);
        fflush(stdout);
    }
}

// global instance of xstate_t objects. Used for testing.
xstate_t *state1;

// global starting address for state1 stack
void *state1_stack;

int num = 3; // global parameter for test function

/* Main function, used for testing. */
int main(int argc, char ** argv) {
    /* create the stack for state1 */
    state1_stack = (void*) malloc(MINSIGSTKSZ);
    
    /* Specifies state1_stack as the alternate stack on which the SIGUSR1
        signal is to be processed. */
    stack_t ss = {
        .ss_size = MINSIGSTKSZ,
        .ss_sp = state1_stack,
    };
    
    /* Specifies xstate_capture_stack as the alternal handler function to be called when
        a signal is processed. */
    struct sigaction sa = {
        .sa_handler = &xstate_capture_stack,
        .sa_flags = SA_ONSTACK,
    };
    
    /* set up the alternate stack and signal action */
    sigaltstack(&ss, 0);
    //sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, 0);
    
    sigset_t mask, oldmask; // mask for blocing SIGUSR1 and the oldset before that
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    
    kill(getpid(), SIGUSR1);
    
    sigprocmask(SIG_UNBLOCK, &mask, &oldmask);
}