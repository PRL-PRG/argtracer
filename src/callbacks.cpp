#include "tracer.h"
#include "callbacks.h"
#include "TracingState.h"
#include "utilities.h"
#include <instrumentr/instrumentr.h>
#include <vector>


static void(*p_add_val)(SEXP) = NULL;

void closure_call_exit_callback(instrumentr_tracer_t tracer,
                                instrumentr_callback_t callback,
                                instrumentr_state_t state,
                                instrumentr_application_t application,
                                instrumentr_closure_t closure,
                                instrumentr_call_t call) {
  if (!p_add_val) {
    p_add_val = (void(*)(SEXP)) R_GetCCallable("record", "add_val");
  }

  // TracingState& tracing_state = TracingState::lookup(state);

  instrumentr_environment_t call_env = instrumentr_call_get_environment(call);
  int position = 0;

  instrumentr_value_t formals = instrumentr_closure_get_formals(closure);

  while (instrumentr_value_is_pairlist(formals)) {
    instrumentr_pairlist_t pairlist =
      instrumentr_value_as_pairlist(formals);

    instrumentr_value_t tagval = instrumentr_pairlist_get_tag(pairlist);

    instrumentr_symbol_t nameval = instrumentr_value_as_symbol(tagval);

    const char* argument_name = instrumentr_char_get_element(instrumentr_symbol_get_element(nameval));

    instrumentr_value_t argval =
      instrumentr_environment_lookup(call_env, nameval);

    if(instrumentr_value_is_promise(argval)) {
      if (instrumentr_promise_is_forced((instrumentr_promise_t) argval)) {
        instrumentr_value_t value = instrumentr_promise_get_value((instrumentr_promise_t) argval);
        SEXP r_promise_val = instrumentr_value_get_sexp(value);
        p_add_val(r_promise_val);
      }
    }
    ++position;
    formals = instrumentr_pairlist_get_cdr(pairlist);
  }

  // /* handle calls */
  // CallTable& call_table = tracing_state.get_call_table();
  // int call_id = instrumentr_call_get_id(call);

  bool has_result = instrumentr_call_has_result(call);

  // std::string result_type = LAZR_NA_STRING;
  if (has_result) {
    instrumentr_value_t value = instrumentr_call_get_result(call);
    instrumentr_value_type_t val_type = instrumentr_value_get_type(value);
    // result_type = instrumentr_value_type_get_name(val_type);

    // linking to record:
    SEXP r_return_val = instrumentr_value_get_sexp(value);
    p_add_val(r_return_val);
  }

  // Call* call_data = call_table.lookup(call_id);

  // call_data->exit(result_type);
}


void tracing_entry_callback(instrumentr_tracer_t tracer,
                            instrumentr_callback_t callback,
                            instrumentr_state_t state) {
    TracingState::initialize(state);
}

void tracing_exit_callback(instrumentr_tracer_t tracer,
                           instrumentr_callback_t callback,
                           instrumentr_state_t state) {
    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    FunctionTable& fun_table = tracing_state.get_function_table();

    fun_table.infer_qualified_names(env_table);

    TracingState::finalize(state);
}
