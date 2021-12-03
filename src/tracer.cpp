#include <algorithm>
#include <array>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <Rdyntrace.h>

#include "constants.h"
#include "debug.h"
#include "hashing.h"
#include "tracer.h"
#include "util.h"

typedef SEXP (*AddValFun)(SEXP, SEXP);
typedef SEXP (*AddOriginFun)(SEXP, const void*, const char*, const char*,
                             const char*);

static AddValFun add_val = NULL;
static AddOriginFun add_origin = NULL;

std::string BLACKLISTED_FUNS[] = {"lazyLoadDBfetch", "library", "loadNamespace",
                                  "require"};

typedef struct {
    SEXP call;
    SEXP op;
    SEXP args;
    SEXP rho;
} Call;

typedef struct {
    void* context;
} Context;

typedef std::variant<Call, Context> Frame;
#define IS_CALL(f) (f.index() == 0)
#define IS_CNTX(f) (f.index() == 1)

// a map of resolved addresses of functions from BLACKLISTED_FUNS
std::unordered_map<SEXP, std::string> blacklisted_funs;
// a depth of blacklisted functions
int blacklist_stack = 0;
// a call stack with both contexts and function calls
std::stack<Frame> call_stack;

class TracerState {
  private:
    int result_;
    std::unordered_map<SEXP, std::unordered_set<Trace>> traces_;
    SEXP db_;

  public:
    TracerState(SEXP db) : db_(db), result_(0){};

    int get_result() { return result_; }

    void set_result(int result) { result_ = result; }

    void add_trace(SEXP clo, std::string param, SEXP val) {
        Debug("Recording %s\n", param.c_str());

        SEXP hash_raw = add_val(db_, val);
        if (hash_raw == R_NilValue) {
            return;
        }

        SxpHash hash = XXH128_hashFromCanonical(
            reinterpret_cast<XXH128_canonical_t*>(RAW(hash_raw)));

        auto trace = std::make_pair(hash, param);
        traces_[clo].insert(trace);
    }

    void push_origins() {
        std::unordered_set<SEXP> seen_env;
        std::unordered_map<SEXP, FunOrigin> origins;

        for (auto& [clo, traces] : traces_) {
            if (TYPEOF(clo) == FREESXP) {
                Debug("Free");
                continue;
            }

            SEXP env = CLOENV(clo);

            auto seen = seen_env.insert(env);
            if (seen.second) {
                populate_namespace_map(env, origins);
            }

            auto origin = origins.find(clo);
            if (origin == origins.end()) {
                continue;
            }

            auto [pkg_name, fun_name] = origin->second;
            for (auto& [hash, param] : traces) {
                add_origin(db_, &hash, pkg_name.c_str(), fun_name.c_str(),
                           param.c_str());
            }
        }
    }
};

void call_entry(dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho) {
    Trace("%s--> %p (%d)\n", std::string(2 * frames.size(), ' ').c_str(), call,
          frames.size());
    call_stack.push(Frame(Call{call, op, args, rho}));

    auto blacklist_fun = blacklisted_funs.find(op);
    if (blacklist_fun != blacklisted_funs.end()) {
        Debug("Entering ignored fun %s\n", blacklist_fun->second.c_str());
        blacklist_stack++;
    }
}

void call_exit(dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho,
               SEXP result) {
    call_stack.pop();
    Trace("%s<-- %p (%d)\n", std::string(2 * frames.size(), ' ').c_str(), call,
          frames.size());

    auto blacklist_fun = blacklisted_funs.find(op);
    if (blacklist_fun != blacklisted_funs.end()) {
        --blacklist_stack;
        Debug("Exiting ignored fun %s (%d)\n", blacklist_fun->second.c_str(),
              blacklist_stack);
        return;
    }

    if (blacklist_stack != 0) {
        return;
    }

    auto type = TYPEOF(op);

    if (type != CLOSXP) {
        // for now
        return;
    }

    auto state = (TracerState*)dyntracer_get_data(tracer);

    Debug("Tracing %p\n", op);

    for (SEXP cons = FORMALS(op); cons != R_NilValue; cons = CDR(cons)) {
        SEXP param_name = TAG(cons);
        if (param_name != R_DotsSymbol) {
            SEXP param_val = Rf_findVarInFrame3(rho, param_name, TRUE);
            if (param_val == R_UnboundValue) {
                continue;
            }

            if (TYPEOF(param_val) == PROMSXP) {
                if (PRVALUE(param_val) == R_UnboundValue) {
                    continue;
                } else {
                    param_val = PRVALUE(param_val);
                }
            }

            std::string param_name_str = CHAR(PRINTNAME(param_name));
            state->add_trace(op, param_name_str, param_val);
        } else {
            // TODO: fix dotdotdot
        }
    }

    state->add_trace(op, RETURN_PARAM_NAME, result);
}

void exit_callback(dyntracer_t* tracer, SEXP expression, SEXP environment,
                   SEXP result, int error) {
    auto state = (TracerState*)dyntracer_get_data(tracer);
    state->set_result(error);
}

void context_entry(dyntracer_t* tracer, void* pointer) {
    Trace("%s==> %p (%d)\n", std::string(2 * frames.size(), ' ').c_str(),
          pointer, frames.size());

    call_stack.push(Frame(Context{pointer}));
}

void context_exit(dyntracer_t* tracer, void* pointer) {
    Trace("%s<== %p (%d) [%p]\n",
          std::string(2 * (frames.size() - 1), ' ').c_str(), pointer,
          frames.size() - 1, std::get<Context>(frames.top()).context);

    call_stack.pop();
}

void context_jump_callback(dyntracer_t* tracer, void* pointer,
                           SEXP return_value, int restart) {
    Trace("%sJUMP BEGIN (%p)\n", std::string(2 * frames.size(), ' ').c_str(),
          pointer);

    while (!call_stack.empty()) {
        Frame f = call_stack.top();
        if (IS_CALL(f)) {
            auto call = std::get<Call>(f);
            call_exit(tracer, call.call, call.op, call.args, call.rho,
                      R_NilValue);
        } else if (IS_CNTX(f)) {
            auto cntx = std::get<Context>(f);
            if (cntx.context == pointer) {
                break;
            } else {
                context_exit(tracer, cntx.context);
            }
        }
    }

    Trace("%sJUMP END\n", std::string(2 * frames.size(), ' ').c_str());
}

// TODO: move to a better place
void initialize_globals() {
    if (!add_val) {
        add_val = (AddValFun)R_GetCCallable("sxpdb", "add_val");
    }
    if (!add_origin) {
        add_origin = (AddOriginFun)R_GetCCallable("sxpdb", "add_origin_");
    }

    if (blacklisted_funs.empty()) {
        for (int i = 0; i < sizeof(BLACKLISTED_FUNS) / sizeof(std::string);
             i++) {
            SEXP fun = Rf_findVarInFrame(
                R_BaseEnv, Rf_install(BLACKLISTED_FUNS[i].c_str()));

            if (TYPEOF(fun) == PROMSXP && PRVALUE(fun) != R_UnboundValue) {
                fun = PRVALUE(fun);
            }

            switch (TYPEOF(fun)) {
            case BUILTINSXP:
            case CLOSXP:
            case SPECIALSXP:
                blacklisted_funs.insert(
                    std::make_pair(fun, BLACKLISTED_FUNS[i]));
                break;
            default:
                Rf_error("Unable to load blacklisted function %s\n",
                         BLACKLISTED_FUNS[i].c_str());
            }
        }
    }
}

dyntracer_t* create_tracer() {
    dyntracer_t* tracer = dyntracer_create(NULL);

    dyntracer_set_builtin_entry_callback(
        tracer, [](dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho,
                   dyntrace_dispatch_t dispatch) {
            call_entry(tracer, call, op, args, rho);
        });
    dyntracer_set_special_entry_callback(
        tracer, [](dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho,
                   dyntrace_dispatch_t dispatch) {
            call_entry(tracer, call, op, args, rho);
        });
    dyntracer_set_closure_entry_callback(
        tracer, [](dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho,
                   dyntrace_dispatch_t dispatch) {
            call_entry(tracer, call, op, args, rho);
        });

    dyntracer_set_builtin_exit_callback(
        tracer, [](dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho,
                   dyntrace_dispatch_t dispatch, SEXP result) {
            call_exit(tracer, call, op, args, rho, result);
        });
    dyntracer_set_special_exit_callback(
        tracer, [](dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho,
                   dyntrace_dispatch_t dispatch, SEXP result) {
            call_exit(tracer, call, op, args, rho, result);
        });
    dyntracer_set_closure_exit_callback(
        tracer, [](dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho,
                   dyntrace_dispatch_t dispatch, SEXP result) {
            call_exit(tracer, call, op, args, rho, result);
        });

    dyntracer_set_context_entry_callback(tracer, &context_entry);
    dyntracer_set_context_exit_callback(tracer, &context_exit);
    dyntracer_set_context_jump_callback(tracer, &context_jump_callback);

    dyntracer_set_dyntrace_exit_callback(tracer, &exit_callback);

    return tracer;
}

SEXP trace_code(SEXP db, SEXP code, SEXP rho) {
    initialize_globals();

    dyntracer_t* tracer = create_tracer();

    TracerState state(db);
    dyntracer_set_data(tracer, (void*)&state);

    dyntrace_enable();
    dyntrace_result_t result = dyntrace_trace_code(tracer, code, rho);
    dyntrace_disable();

    state.push_origins();

    return Rf_ScalarInteger(state.get_result());
}
