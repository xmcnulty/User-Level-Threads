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
typedef struct xstate_t {
    jmp_buf jb; // jmp_buf used for context switching
    
    void (*func) (void*); // function called by xstate
    void *args; // arguments for func
    
} xstate_t;

/* Specifies state1_stack as the alternate stack on which the SIGUSR1
 signal is to be processed. */
stack_t ss;

// global instance of xstate_t objects. Used for testing.
xstate_t *state1;

// global starting address for state1 stack
void *state1_stack;

int num = 3; // global parameter for test function

/* Execution state control functions: save, restore, switch. */
#define xstate_save(xstate) setjmp(xstate->jb)
#define xstate_restore(xstate) longjmp(xstate->jb, xstate->setjmp_ret)
#define xstate_switch(old_xstate, new_xstate) \
    if (setjmp(old_xstate->jb) == 0) \
        longjmp(nex_xstate->jb, 1)

/* main is pointer to a possible argument to the main function.
   main_arg is a pointer to a possible argument to the main function.
   stack is the memory buffer set aside for the thread stack.
   stack_size is the size in bytes of the stack */
void xstate_create(xstate_t *xstate, void (*main) (void *), void *main_arg, void *stack, size_t stack_size) {
    /* sets values of ss to create the stack when the user signal is handled */
    ss.ss_size = stack_size;
    ss.ss_sp = stack;
    ss.ss_flags = SA_ONSTACK;
    
    /* allocate the xstate and set its contents */
    if ((xstate = (xstate_t*) calloc(1, sizeof(xstate_t))) == 0) {
        perror("allocating xstate.\n");
        exit(-1);
    }
    
    xstate->func = main;
    xstate->args = main_arg;
}

/* Private signal handler function that will be called with a signal stack.
   The execution state will be saved before returning from the handler. 
   The saved execution state will later be used to allow a non-local
   jump back to the signal handler. At this point, code will be executing
   with the alternative stack in place and the function xstate_boot will
   be called */
void xstate_capture_stack(int sig) {
    xstate_save(state1);
    
    printf("in xstate_capture_stack: %d\n", sig);
    fflush(stdout);
    
    exit(0);
}

/* Start the newly create thread by making a call to the main function of the thread. */
/* this test function squares a number and prints the result. */
void xstate_boot(void) {
    ((void (*) (int))state1->func) (*((int *) state1->args));
}

/* test function */
void test (int n) {
    int i;
    
    for (i=0; i < n; i++) {
        printf("%d\n", i);
        fflush(stdout);
    }
}

/* Specifies xstate_capture_stack as the alternal handler function to be called when
 a signal is processed. */
struct sigaction sa = {
    .sa_handler = &xstate_capture_stack,
    .sa_flags = SA_ONSTACK,
};

/* Main function, used for testing. */
int main(int argc, char ** argv) {
    /* allocate space for the state1 stack */
    state1_stack = (void*) calloc(MINSIGSTKSZ, sizeof(void));
    
    /* create the xstate state1 */
    xstate_create(state1, (void (*) (void *)) &test, (void*) &num, state1_stack, MINSIGSTKSZ);
    
    sigfillset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGUSR1);
    
    sigset_t oldset; // old signal set, (without SIGUSR1)
    
    sigprocmask(SIG_BLOCK, &sa.sa_mask, &oldset);
    
    kill(getpid(), SIGUSR1);
    
    sigprocmask(SIG_UNBLOCK, &sa.sa_mask, 0);
}