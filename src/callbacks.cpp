// #include <cassert>
// #include "tracer.h"
// #include "callbacks.h"
// #include "utilities.h"
// #include "constants.h"
// #include <instrumentr/instrumentr.h>
// #include <vector>
// #include <stdio.h>
// #include <string.h>
//
// static int package_loading;
// static SEXP (*sxpdb_add_val_origin_)(SEXP sxpdb, SEXP val, 
//                                      const char* package_name, 
//                                      const char* function_name, 
//                                      const char* argument_name) = NULL;
// // SEXP ln_fun = Rf_findFun(Rf_install("loadNamespace"), R_BaseEnv);      Aviral says this could be problematic
//
// void closure_call_entry_callback(instrumentr_tracer_t tracer,
//                                  instrumentr_callback_t callback,
//                                  instrumentr_state_t state,
//                                  instrumentr_application_t application,
//                                  instrumentr_closure_t closure,
//                                  instrumentr_call_t call) {
//
//   SEXP fun = instrumentr_closure_get_sexp(closure);
//   const char* name = instrumentr_closure_get_name(closure);
//
//   if(name != NULL && !strcmp(name, "loadNamespace")) {
//     package_loading += 1;
//   }
// }
//
// // void builtin_call_exit_callback(instrumentr_tracer_t tracer,
// //                                 instrumentr_callback_t callback,
// //                                 instrumentr_state_t state,
// //                                 instrumentr_application_t application,
// //                                 instrumentr_builtin_t builtin,
// //                                 instrumentr_call_t call) {
// //
// //   // closure_call_exit_callback(tracer, callback, state, application, closure, call);
// //   const char* name = instrumentr_builtin_get_name(builtin);
// //   Rprintf("%s\n", name);
// // }
//
// void closure_call_exit_callback(instrumentr_tracer_t tracer,
//                                 instrumentr_callback_t callback,
//                                 instrumentr_state_t state,
//                                 instrumentr_application_t application,
//                                 instrumentr_closure_t closure,
//                                 instrumentr_call_t call) {
//
//   const char* fun_name = instrumentr_closure_get_name(closure);
//
//   instrumentr_environment_t env = instrumentr_closure_get_environment(closure);
//   const char* pkg_name = instrumentr_environment_get_name(env);
//   
//   if(fun_name != NULL && !strcmp(fun_name, "loadNamespace")) {
//     package_loading -= 1;
//     return;
//   }
//
//   if (package_loading) {
//     return;
//   }
//
//   if (!sxpdb_add_val_origin_) {
//     sxpdb_add_val_origin_ = (SEXP(*)(SEXP, SEXP, const char*, const char*, const char*)) R_GetCCallable("sxpdb", "add_val_origin_");
//   }
//
//   instrumentr_environment_t call_env = instrumentr_call_get_environment(call);
//   instrumentr_value_t formals = instrumentr_closure_get_formals(closure);
//
//   int position = 0;
//
//   SEXP db = PROTECT(instrumentr_state_lookup(state, "db", R_NilValue));
//
//   while (instrumentr_value_is_pairlist(formals)) {
//     instrumentr_pairlist_t pairlist = instrumentr_value_as_pairlist(formals);
//     instrumentr_value_t tagval = instrumentr_pairlist_get_tag(pairlist);
//     instrumentr_symbol_t nameval = instrumentr_value_as_symbol(tagval);
//     const char* param_name = instrumentr_char_get_element(instrumentr_symbol_get_element(nameval));
//
//     instrumentr_value_t argval =
//       instrumentr_environment_lookup(call_env, nameval);
//
//     // std::string arg_type = LAZR_NA_STRING;
//     if(instrumentr_value_is_promise(argval)) {
//       instrumentr_promise_t promise = instrumentr_value_as_promise(argval);
//
//       if (instrumentr_promise_is_forced(promise)) {
//         instrumentr_value_t value = instrumentr_promise_get_value(promise);
//         SEXP value_sexp = PROTECT(instrumentr_value_get_sexp(value));
//         sxpdb_add_val_origin_(db, value_sexp, pkg_name, fun_name, param_name);
//         UNPROTECT(1);
//       }
//     }
//
//     ++position;
//     formals = instrumentr_pairlist_get_cdr(pairlist);
//   }
//
//   bool has_result = instrumentr_call_has_result(call);
//
//   if (has_result) {
//     instrumentr_value_t value = instrumentr_call_get_result(call);
//     SEXP value_sexp = PROTECT(instrumentr_value_get_sexp(value));
//     sxpdb_add_val_origin_(db, value_sexp, pkg_name, fun_name, RETURN_PARAM_NAME);
//     UNPROTECT(1);
//   }
//
//   UNPROTECT(1); // db
// }
