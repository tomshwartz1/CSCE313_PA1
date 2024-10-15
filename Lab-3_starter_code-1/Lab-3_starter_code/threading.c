#include "threading.h"

/**
 * Initializes the array of contexts and sets initial state.
 */
void t_init() {
    for (int i = 0; i < NUM_CTX; i++) {
        contexts[i].state = INVALID;
    }
    current_context_idx = NUM_CTX;
}

/**
 * t_create function creates a new worker context
 */
int32_t t_create(fptr foo, int32_t arg1, int32_t arg2) {
    for (volatile int i = 0; i < NUM_CTX; i++) {
        if (contexts[i].state == INVALID) {
            if (getcontext(&contexts[i].context) == -1) {
                return 1;
            }

            contexts[i].context.uc_stack.ss_sp = malloc(STK_SZ);
            if (contexts[i].context.uc_stack.ss_sp == NULL) {
                return 1; 
            }
            contexts[i].context.uc_stack.ss_size = STK_SZ;
            contexts[i].context.uc_stack.ss_flags = 0;
            contexts[i].context.uc_link = NULL;

            makecontext(&contexts[i].context, (ctx_ptr)foo, 2, arg1, arg2);

            contexts[i].state = VALID;
            return 0;
        }
    }
    return 1; 
}

/**
 * t_yield function switches control between contexts
 */
int32_t t_yield() {
    int next_context_idx = -1;
    for (int i = 0; i < NUM_CTX; i++) {
        int idx = (current_context_idx + i) % NUM_CTX;
        if (contexts[idx].state == VALID) {
            next_context_idx = idx;
            break;
        }
    }

    if (next_context_idx == -1) {
        return -1;
    }

    int32_t valid_count = 0;
    for (int i = 0; i < NUM_CTX; i++) {
        if (contexts[i].state == VALID) {
            valid_count++;
        }
    }

    uint8_t old_context_idx = current_context_idx;
    current_context_idx = (uint8_t)next_context_idx;
    swapcontext(&contexts[old_context_idx].context, &contexts[current_context_idx].context);

    return valid_count - 1;
}

/**
 * t_finish function marks a context as DONE and releases resources
 */
void t_finish() {
    if (contexts[current_context_idx].state == VALID) {
        free(contexts[current_context_idx].context.uc_stack.ss_sp);
        contexts[current_context_idx].state = DONE;
        memset(&contexts[current_context_idx].context, 0, sizeof(ucontext_t));
    }
    t_yield();
}
