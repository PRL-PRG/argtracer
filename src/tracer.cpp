#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <unordered_map>

#include <Rdyntrace.h>

#include "constants.h"
#include "debug.h"
#include "tracer.h"
#include "utilities.h"

typedef std::array<char, 20> SxpHash;
typedef std::pair<SxpHash, std::string> Trace;
typedef SEXP (*AddValFun)(SEXP, SEXP);

static AddValFun add_val = NULL;

std::string BLACKLISTED_FUNS[] = {"lazyLoadDBfetch", "loadNamespace"};

std::unordered_map<SEXP, std::string> blacklisted_funs;
int blacklist_stack = 0;

class TracerState {
  private:
    std::unordered_map<SEXP, std::set<Trace>> traces_;
    SEXP db_;

  public:
    TracerState(SEXP db): db_(db){};

    void add_trace(SEXP clo, std::string param, SEXP val) {
        if (!add_val) {
            add_val = (AddValFun) R_GetCCallable("sxpdb", "add_val");
        }

        Debug("Recording %s\n", param.c_str());

        SEXP hash_raw = add_val(db_, val);
        if (hash_raw == R_NilValue) {
            return;
        }

        SxpHash hash;
        std::copy_n(RAW(hash_raw), Rf_length(hash_raw), hash.begin());

        auto trace = std::make_pair(hash, param);
        traces_[clo].insert(trace);
    }
};

#define HANDLE_IGNORE_FUNS_ENTRY(op) \
    do { \
      auto blacklist_fun = blacklisted_funs.find(op); \
      if (blacklist_fun != blacklisted_funs.end()) { \
          Debug("Entering ignored fun %s\n", blacklist_fun->second.c_str()); \
          blacklist_stack++; \
      } \
    } while(0)

#define HANDLE_IGNORE_FUNS_EXIT(op) \
    do { \
      auto blacklist_fun = blacklisted_funs.find(op); \
      if (blacklist_fun != blacklisted_funs.end()) { \
          Debug("Exiting ignored fun %s\n", blacklist_fun->second.c_str()); \
          blacklist_stack--; \
      } \
      if (blacklist_stack != 0) { \
          Debug("%d\n", blacklist_stack); \
          return; \
      } \
    } while(0)

void builtin_entry_callback(dyntracer_t* tracer,
                            SEXP call,
                            SEXP op,
                            SEXP args,
                            SEXP rho,
                            dyntrace_dispatch_t dispatch) {
    HANDLE_IGNORE_FUNS_ENTRY(op);
}

void builtin_exit_callback(dyntracer_t* tracer,
                           SEXP call,
                           SEXP op,
                           SEXP args,
                           SEXP rho,
                           dyntrace_dispatch_t dispatch,
                           SEXP result) {
    HANDLE_IGNORE_FUNS_EXIT(op);
}

void closure_entry_callback(dyntracer_t* tracer,
                            SEXP call,
                            SEXP op,
                            SEXP args,
                            SEXP rho,
                            dyntrace_dispatch_t dispatch) {
    HANDLE_IGNORE_FUNS_ENTRY(op);
}

// TODO: test ...
// TODO: test missing
// TODO: test exception <- what happens with the result?
void closure_exit_callback(dyntracer_t* tracer,
                           SEXP call,
                           SEXP op,
                           SEXP args,
                           SEXP rho,
                           dyntrace_dispatch_t dispatch,
                           SEXP result) {
    HANDLE_IGNORE_FUNS_EXIT(op);

    auto state = (TracerState*) dyntracer_get_data(tracer);

    Debug("Called %p\n", op);

    for (SEXP cons = FORMALS(op); cons != R_NilValue; cons = CDR(cons)) {
        SEXP param_name = TAG(cons);
        if (param_name != R_DotsSymbol) {
            SEXP param_val = Rf_findVarInFrame3(rho, param_name, TRUE);
            if (param_val == R_UnboundValue)
                continue;
            if (TYPEOF(param_val) == PROMSXP &&
                PRVALUE(param_val) == R_UnboundValue)
                continue;

            std::string param_name_str = CHAR(PRINTNAME(param_name));
            state->add_trace(op, param_name_str, param_val);
        } else {
            // TODO: fix dotdotdot
        }
    }

    state->add_trace(op, RETURN_PARAM_NAME, result);
}

// TODO: move to a better place
void initialize_globals() {
    if (!blacklisted_funs.empty()) {
      return;
    }

    for (int i = 0; i < sizeof(BLACKLISTED_FUNS) / sizeof(std::string); i++) {
        SEXP fun = Rf_findVarInFrame(R_BaseEnv,
                                     Rf_install(BLACKLISTED_FUNS[i].c_str()));

        if (TYPEOF(fun) == PROMSXP && PRVALUE(fun) != R_UnboundValue) {
            fun = PRVALUE(fun);
        }

        switch (TYPEOF(fun)) {
        case BUILTINSXP:
        case CLOSXP:
        case SPECIALSXP:
            blacklisted_funs.insert(std::make_pair(fun, BLACKLISTED_FUNS[i]));
            break;
        default:
            Rf_error("Unable to load blacklisted function %s\n",
                     BLACKLISTED_FUNS[i].c_str());
        }
    }
}

dyntracer_t* create_tracer(SEXP db) {
    dyntracer_t* tracer = dyntracer_create(NULL);
    dyntracer_set_builtin_exit_callback(tracer, &builtin_exit_callback);
    dyntracer_set_builtin_entry_callback(tracer, &builtin_entry_callback);
    dyntracer_set_closure_exit_callback(tracer, &closure_exit_callback);
    dyntracer_set_closure_entry_callback(tracer, &closure_entry_callback);

    TracerState* state = new TracerState(db);
    dyntracer_set_data(tracer, (void*) state);

    return tracer;
}

SEXP trace_code(SEXP db, SEXP code, SEXP rho) {
    dyntracer_t* tracer = create_tracer(db);

    initialize_globals();

    dyntrace_enable();
    dyntrace_result_t result = dyntrace_trace_code(tracer, code, rho);
    dyntrace_disable();

    return R_NilValue;
}
