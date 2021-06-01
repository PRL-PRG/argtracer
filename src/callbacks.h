#ifndef ARGTRACER_CALLBACKS_H
#define ARGTRACER_CALLBACKS_H
#include <instrumentr/instrumentr.h>


void closure_call_entry_callback(instrumentr_tracer_t tracer,
                                 instrumentr_callback_t callback,
                                 instrumentr_state_t state,
                                 instrumentr_application_t application,
                                 instrumentr_closure_t closure,
                                 instrumentr_call_t call);

void closure_call_exit_callback(instrumentr_tracer_t tracer,
                                instrumentr_callback_t callback,
                                instrumentr_state_t state,
                                instrumentr_application_t application,
                                instrumentr_closure_t closure,
                                instrumentr_call_t call);

#endif /* ARGTRACER_CALLBACKS_H  */
