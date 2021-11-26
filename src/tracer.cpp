#include <string>
#include <unordered_map>

#include <Rdyntrace.h>

#include "tracer.h"
#include "utilities.h"


typedef std::pair<std::string, std::string> FunOrigin;

class TracerState {
    private:
        std::unordered_map<SEXP, FunOrigin> funs_origins_;

        bool get_fun_origin_(SEXP clo, FunOrigin &origin, bool updated) {
            auto res = funs_origins_.find(clo);
            if (res != funs_origins_.end()) {
                origin = res->second;
                return true;
            } else if (!updated) {
                update_funs_origins(CLOENV(clo));
                return get_fun_origin_(clo, origin, true);
            } else {
                return false;
            }
        }
    public:
        bool get_fun_origin(SEXP clo, FunOrigin &origin) {
          return get_fun_origin_(clo, origin, false);
        }

        bool update_funs_origins(SEXP env) {
            auto package_opt = env_get_name(env);
            if (!package_opt) {
                return false;
            }
            auto package = package_opt.value();
            auto bindings = PROTECT(R_lsInternal3(env, TRUE, FALSE));
            
            for (int i=0; i<Rf_length(bindings); i++) {
                auto binding = STRING_ELT(bindings, i);
                auto fun = Rf_findVarInFrame3(env, Rf_installChar(binding), TRUE);
                auto has_value = -1;
                if (TYPEOF(fun) == PROMSXP) has_value = PRVALUE(fun) == R_UnboundValue;

                Rprintf("*** %s::%s %d %d %p ", package.c_str(), CHAR(binding), TYPEOF(fun), has_value, fun);
                if (fun == R_UnboundValue) {
                  Rprintf("not found\n");
                  continue;
                }
                if (TYPEOF(fun) != CLOSXP) {
                  Rprintf("not closure\n");
                  continue;
                }
                Rprintf("\n");

                std::string fun_name = CHAR(binding);
                auto origin = std::make_pair(package, fun_name);

                add_fun_origin(fun, origin);
            }

            UNPROTECT(1);
            return true;
            
        }

        void add_fun_origin(SEXP clo, const FunOrigin &origin) {
            Rprintf("Adding %s::%s\n", origin.first.c_str(), origin.second.c_str());
            funs_origins_.insert(std::make_pair(clo, origin));
        }

};

// hash of op  
// hash of each argument
// have a hashmap<sxp_hash, sxp>
// 
void closure_exit_callback(
        dyntracer_t* tracer,
        SEXP call,
        SEXP op,
        SEXP args,
        SEXP rho,
        dyntrace_dispatch_t dispatch,
        SEXP result) {
    TracerState *state = (TracerState *)dyntracer_get_data(tracer);
    FunOrigin origin;
    
    Rprintf("Called %p\n", op);
    if (state->get_fun_origin(op, origin)) {
      Rprintf("Called %s::%s\n", origin.first.c_str(), origin.second.c_str());
    } else {
    }
    

}

dyntracer_t *create_tracer() {
    TracerState *state = new TracerState();
    dyntracer_t *tracer = dyntracer_create(NULL);
    dyntracer_set_closure_exit_callback(tracer, &closure_exit_callback);
    dyntracer_set_data(tracer, (void *) state);
    return tracer;
}

SEXP trace_code(SEXP code, SEXP rho) {
    dyntracer_t *tracer = create_tracer();
    dyntrace_enable();
    dyntrace_result_t result = dyntrace_trace_code(tracer, code, rho);
    dyntrace_disable();
    return R_NilValue;
}

// SEXP r_argtracer_create(SEXP db) {
//     instrumentr_tracer_t tracer = instrumentr_tracer_create();
//
//     instrumentr_callback_t callback;
//
//     callback = instrumentr_callback_create_from_c_function(
//         (void*) (closure_call_entry_callback), INSTRUMENTR_EVENT_CLOSURE_CALL_ENTRY);
//     instrumentr_tracer_set_callback(tracer, callback);
//     instrumentr_object_release(callback);
//
//     callback = instrumentr_callback_create_from_c_function(
//         (void*) (closure_call_exit_callback), INSTRUMENTR_EVENT_CLOSURE_CALL_EXIT);
//     instrumentr_tracer_set_callback(tracer, callback);
//     instrumentr_object_release(callback);
//
//     // callback = instrumentr_callback_create_from_c_function(
//     //     (void*) (builtin_call_exit_callback), INSTRUMENTR_EVENT_BUILTIN_CALL_EXIT);
//     // instrumentr_tracer_set_callback(tracer, callback);
//     // instrumentr_object_release(callback);
//
//
//     instrumentr_state_t state = instrumentr_tracer_get_state(tracer);
//
//     instrumentr_state_insert(state, "db", db, 0);
//
//     SEXP r_tracer = instrumentr_tracer_wrap(tracer);
//     instrumentr_object_release(tracer);
//     return r_tracer;
// }
