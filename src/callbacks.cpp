#include <cassert>
#include "tracer.h"
#include "callbacks.h"
#include "utilities.h"
#include <instrumentr/instrumentr.h>
#include <vector>
#include <stdio.h>
#include <string.h>

int package_loading;
static void(*p_add_val)(SEXP) = NULL;
// SEXP ln_fun = Rf_findFun(Rf_install("loadNamespace"), R_BaseEnv);      Aviral says this could be problematic

void closure_call_entry_callback(instrumentr_tracer_t tracer,
                                 instrumentr_callback_t callback,
                                 instrumentr_state_t state,
                                 instrumentr_application_t application,
                                 instrumentr_closure_t closure,
                                 instrumentr_call_t call) {

  SEXP fun = instrumentr_closure_get_sexp(closure);
  const char* name = instrumentr_closure_get_name(closure);
  // Rprintf("%s\n", name);

  if(name != NULL && !strcmp(name, "loadNamespace")) {
    package_loading += 1;
    // printf("+1!\n");
  }
}

void closure_call_exit_callback(instrumentr_tracer_t tracer,
                                instrumentr_callback_t callback,
                                instrumentr_state_t state,
                                instrumentr_application_t application,
                                instrumentr_closure_t closure,
                                instrumentr_call_t call) {

  SEXP fun = instrumentr_closure_get_sexp(closure);
  const char* name = instrumentr_closure_get_name(closure);
  // Rprintf("%s\n", name);

  if(name != NULL && !strcmp(name, "loadNamespace")) {
    package_loading -= 1;
    // printf("-1!\n");
    return;
  }

  if (package_loading) {
    return;
  }

  if (!p_add_val) {
    p_add_val = (void(*)(SEXP)) R_GetCCallable("record", "add_val");
  }

  instrumentr_environment_t call_env = instrumentr_call_get_environment(call);

  instrumentr_value_t formals = instrumentr_closure_get_formals(closure);

  int position = 0;

  while (instrumentr_value_is_pairlist(formals)) {
    instrumentr_pairlist_t pairlist =
      instrumentr_value_as_pairlist(formals);

    instrumentr_value_t tagval = instrumentr_pairlist_get_tag(pairlist);

    instrumentr_symbol_t nameval = instrumentr_value_as_symbol(tagval);

    const char* argument_name = instrumentr_char_get_element(instrumentr_symbol_get_element(nameval));

    instrumentr_value_t argval =
      instrumentr_environment_lookup(call_env, nameval);

    // std::string arg_type = LAZR_NA_STRING;
    if(instrumentr_value_is_promise(argval)) {
      instrumentr_promise_t promise = instrumentr_value_as_promise(argval);

      if (instrumentr_promise_is_forced(promise)) {
        instrumentr_value_t forced_val = instrumentr_promise_get_value(promise);
      // if (instrumentr_promise_is_forced((instrumentr_promise_t) argval)) {
      //   instrumentr_value_t value = instrumentr_promise_get_value((instrumentr_promise_t) argval);
        // instrumentr_value_type_t val_type = instrumentr_value_get_type(value);
        // arg_type = instrumentr_value_type_get_name(val_type);

        SEXP r_forced_val = PROTECT(instrumentr_value_get_sexp(forced_val));
        p_add_val(r_forced_val);
        // printf("%p", r_forced_val);

        UNPROTECT(1);
        // TODO
        // p_add_type(arg_type);
        // p_add_fun(name);
      }
    }
    ++position;
    formals = instrumentr_pairlist_get_cdr(pairlist);
  }

  bool has_result = instrumentr_call_has_result(call);

  // std::string ret_type = LAZR_NA_STRING;
  if (has_result) {
    instrumentr_value_t ret_val = instrumentr_call_get_result(call);
    // instrumentr_value_type_t val_type = instrumentr_value_get_type(value);
    // ret_type = instrumentr_value_type_get_name(val_type);

    // linking to record:
    SEXP r_ret_val = PROTECT(instrumentr_value_get_sexp(ret_val));
    p_add_val(r_ret_val);
    // printf("%p", r_ret_val);
    UNPROTECT(1);
    // TODO
    // p_add_type(ret_type);
    // p_add_fun(name);
  }
}
