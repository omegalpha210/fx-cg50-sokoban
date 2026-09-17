#ifndef SOK_TEST_NATIVE_GINT_H
#define SOK_TEST_NATIVE_GINT_H
typedef struct gint_call_t {
    int (*function)(void *);
    void *argument;
} gint_call_t;
#define GINT_CALL(function_, argument_) ((gint_call_t){function_, argument_})
int gint_world_switch(gint_call_t call);
#endif
