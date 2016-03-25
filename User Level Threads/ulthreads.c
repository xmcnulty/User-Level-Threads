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

// Execution state (machine context) data structure
typedef struct xstate_st {
    void (*main) (void *);
    jmp_buf jb;
} xstate_t;

/* Execution state control functions: save, restore, switch. */
void xstate_save(xstate_t *xstate);
void xstate_restore(xstate_t *xstate);
void xstate_switch(xstate_t *xstate);

/* main is pointer to a possible argument to the main function.
   main_arg is a pointer to a possible argument to the main function.
   stack is the memory buffer set aside for the thread stack.
   stack_size is the size in bytes of the stack */
void xstate_create(xstate_t *xstate, void (*main) (void *), void *main_arg, void *stack, size_t stack_size);

/* Private signal handler function that will be called with a signal stack.
   The execution state will be saved before returning from the handler. 
   The saved execution state will later be used to allow a non-local
   jump back to the signal handler. At this point, code will be executing
   with the alternative stack in place and the function xstate_boot will
   be called */
void xstate_capture_stack(int sig);

/* Start the newly create thread by making a call to the main function of the thread. */
void xstate_boot(void);