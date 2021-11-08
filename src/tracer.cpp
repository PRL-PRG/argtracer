#include "tracer.h"
#include "callbacks.h"
#include <instrumentr/instrumentr.h>


SEXP r_argtracer_create(SEXP db) {
    instrumentr_tracer_t tracer = instrumentr_tracer_create();

    instrumentr_callback_t callback;

    callback = instrumentr_callback_create_from_c_function(
        (void*) (closure_call_entry_callback), INSTRUMENTR_EVENT_CLOSURE_CALL_ENTRY);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (closure_call_exit_callback), INSTRUMENTR_EVENT_CLOSURE_CALL_EXIT);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    // callback = instrumentr_callback_create_from_c_function(
    //     (void*) (builtin_call_exit_callback), INSTRUMENTR_EVENT_BUILTIN_CALL_EXIT);
    // instrumentr_tracer_set_callback(tracer, callback);
    // instrumentr_object_release(callback);


    instrumentr_state_t state = instrumentr_tracer_get_state(tracer);

    instrumentr_state_insert(state, "db", db, 0);

    SEXP r_tracer = instrumentr_tracer_wrap(tracer);
    instrumentr_object_release(tracer);
    return r_tracer;
}
