#include <algorithm>
#include <array>
#include <deque>
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

typedef SEXP (*AddValOriginFun)(SEXP, SEXP, const char*, const char*,
                                const char*, uint64_t);

static AddValOriginFun add_val_origin = NULL;

const char* BLACKLISTED_FUNS[] = {"parent.frame",
                                  "sys.calls",
                                  "sys.frame",
                                  "sys.frames",
                                  "sys.function",
                                  "sys.nframe",
                                  "sys.on.exit",
                                  "sys.parent",
                                  "sys.parents",
                                  "sys.status",
                                  "::",
                                  ":::",
                                  "asNamespace",
                                  "asNamespace",
                                  "attachNamespace",
                                  "detach",
                                  "dyn.load",
                                  "dyn.unload",
                                  "getCallingDLL",
                                  "getHook",
                                  "getLoadedDLLs",
                                  "getNamespace",
                                  "getNamespaceExports",
                                  "getNamespaceImports",
                                  "getNamespaceInfo",
                                  "getNamespaceName",
                                  "getNamespaceUsers",
                                  "getNamespaceVersion",
                                  "isBaseNamespace",
                                  "isNamespace",
                                  "isNamespaceLoaded",
                                  "lazyLoadDBfetch",
                                  "library",
                                  "loadNamespace",
                                  "loadedNamespaces",
                                  "loadingNamespaceInfo",
                                  "namespaceExport",
                                  "namespaceImport",
                                  "namespaceImportClasses",
                                  "namespaceImportFrom",
                                  "namespaceImportMethods",
                                  "packageEvent",
                                  "packageHasNamespace",
                                  "packageNotFoundError",
                                  "packageStartupMessage",
                                  "parseNamespaceFile",
                                  "require",
                                  "requireNamespace",
                                  "setHook",
                                  "setNamespaceInfo",
                                  "unlockBinding",
                                  "unloadNamespace",
                                  "sys.call",
                                  "trace",
                                  "$.DLLInfo",
                                  "debug",
                                  "debugonce",
                                  "getCallingDLLe",
                                  "getDLLRegisteredRoutines",
                                  "getDLLRegisteredRoutines.DLLInfo",
                                  "getDLLRegisteredRoutines.character",
                                  "is.loaded",
                                  "isdebugged",
                                  "lazyLoad",
                                  "library.dynam",
                                  "library.dynam.unload",
                                  "lockBinding",
                                  "lockEnvironment",
                                  "undebug",
                                  "untrace"};

typedef struct {
    SEXP call, op, args, rho;
} Call;

typedef struct {
    void* context;
} Context;

typedef std::variant<Call, Context> Frame;
#define IS_CALL(f) (f.index() == 0)
#define IS_CNTX(f) (f.index() == 1)

// a map of resolved addresses of functions from BLACKLISTED_FUNS
static std::unordered_map<SEXP, std::string> blacklisted_funs;
static SEXP loadNamespaceFun = NULL;

class TracerState {
  private:
    // the SEXP database
    SEXP db_;
    // a depth of blacklisted functions
    int blacklist_stack_ = 0;
    // a call stack with both contexts and function calls
    std::deque<Frame> call_stack_;
    // a map of seen environments
    std::unordered_map<SEXP, std::string> envirs_;
    // a map of known functions
    std::unordered_map<SEXP, std::string> funs_;
    // a set of environments that shall be eventually loaded
    std::unordered_set<SEXP> pending_;
    // whether to record or not
    bool recording_ = true;
    // call_id
    uint64_t call_id_ = 0;

    inline void add_trace_val(const std::string& pkg_name,
                              const std::string& fun_name,
                              const std::string& param_name, SEXP val) {
        DEBUG("Recorded: %s::%s - %s\n", pkg_name.c_str(), fun_name.c_str(),
              param_name.c_str());

        if (recording_) {
            add_val_origin(db_, val, pkg_name.c_str(), fun_name.c_str(),
                           param_name.c_str(), call_id_);
        }
    }

    void populate_namespace(SEXP env) {
        if (envirs_.find(env) != envirs_.end()) {
            DEBUG("Namespace: %p already loaded\n", env);
            return;
        }

        auto pkg_name = env_get_name(env);

        if (!pkg_name) {
            DEBUG("Environment: %p is not a namespace\n", env);
            return;
        }

        envirs_[env] = *pkg_name;

        auto values = PROTECT(env2list(env));
        auto names = PROTECT(Rf_getAttrib(values, R_NamesSymbol));

        DEBUG("Populating %p: %s\n", env, (*pkg_name).c_str());

        if (Rf_length(values) != Rf_length(names)) {
            Rf_warning("Lengths do not match in env2list. Not loading: %s\n",
                       (*pkg_name).c_str());
            return;
        }

        for (int i = 0; i < Rf_length(names); i++) {
            auto name = STRING_ELT(names, i);
            std::string fun_name = CHAR(name);

            auto fun = VECTOR_ELT(values, i);

            if (TYPEOF(fun) != CLOSXP) {
                TRACE("%s is not a closure\n", fun_name.c_str());
                continue;
            }

            TRACE("Loaded %s::%s\n", (*pkg_name).c_str(), fun_name.c_str());

            funs_[fun] = fun_name;
        }

        UNPROTECT(2);
    }

    inline void load_package_name(SEXP env) {
        auto env_key = envirs_.find(env);
        if (env_key != envirs_.end()) {
            return;
        }

        if (pending_.find(env) != pending_.end()) {
            populate_namespace(env);
            pending_.erase(env);
        }
    }

    void trace_builtin_call(SEXP op, SEXP args, SEXP rho, SEXP result) {
        DEBUG("Tracing builtin %p\n", op);
        ++call_id_;

        std::string fun_name = dyntrace_get_c_function_name(op);
        int i = 0;

        for (SEXP x = args; x != R_NilValue; x = CDR(x), ++i) {
            SEXP v = CAR(x);
            if (TYPEOF(v) == PROMSXP) {
                if (PRVALUE(v) != R_UnboundValue) {
                    v = PRVALUE(v);
                } else {
                    continue;
                }
            }

            std::string param_name = std::to_string(i);
            add_trace_val(BASE_PKG_NAME, fun_name, param_name, v);
        }

        if (result != R_UnboundValue) {
            add_trace_val(BASE_PKG_NAME, fun_name, RETURN_PARAM_NAME, result);
        }
    }

    void trace_closure_call(SEXP clo, SEXP rho, SEXP result) {
        DEBUG("Tracing %p\n", clo);
        ++call_id_;

        auto pkg_key = envirs_.find(CLOENV(clo));
        if (pkg_key == envirs_.end()) {
            DEBUG("%p: not from a namespace\n", clo);
            return;
        }
        auto pkg_name = pkg_key->second;

        auto fun_key = funs_.find(clo);
        if (fun_key == funs_.end()) {
            DEBUG("%p: not a named closure\n", clo);
            return;
        }
        auto fun_name = fun_key->second;

        for (SEXP cons = FORMALS(clo); cons != R_NilValue; cons = CDR(cons)) {
            SEXP param_tag = TAG(cons);

            if (param_tag == R_DotsSymbol) {
                auto dd_arg = arg_val(Rf_findVarInFrame3(rho, param_tag, TRUE));
                if (!dd_arg) {
                    continue;
                }

                int i = 1;
                for (SEXP dd = *dd_arg; dd != R_NilValue; dd = CDR(dd), i++) {
                    auto val = arg_val(CAR(dd));
                    if (!val) {
                        continue;
                    }
                    auto param_name = ".." + std::to_string(i);
                    add_trace_val(pkg_name, fun_name, param_name, *val);
                }
            } else {
                auto val = arg_val(Rf_findVarInFrame3(rho, param_tag, TRUE));
                if (!val) {
                    continue;
                }
                std::string param_name = CHAR(PRINTNAME(param_tag));
                add_trace_val(pkg_name, fun_name, param_name, *val);
            }
        }

        if (result != R_UnboundValue) {
            add_trace_val(pkg_name, fun_name, RETURN_PARAM_NAME, result);
        }
    }

  public:
    TracerState(SEXP db) : db_(db){};

    void disable_recording() { recording_ = false; }

    void add_pending_namespace(SEXP env) { pending_.insert(env); }

    void call_entry(SEXP call, SEXP op, SEXP args, SEXP rho) {
        TRACE("%s--> %p (%lu)\n", INDENT(call_stack_.size()), call,
              call_stack_.size());

        call_stack_.push_back(Frame(Call{call, op, args, rho}));

        auto blacklist_fun = blacklisted_funs.find(op);
        if (blacklist_fun != blacklisted_funs.end()) {
            DEBUG("Entering ignored fun %s\n", blacklist_fun->second.c_str());
            blacklist_stack_++;
        } else {
            load_package_name(CLOENV(op));
        }
    }

    void call_exit(SEXP call, SEXP op, SEXP args, SEXP rho, SEXP result) {
        call_stack_.pop_back();

        if (op == loadNamespaceFun && TYPEOF(result) == ENVSXP) {
            add_pending_namespace(result);
        }

        TRACE("%s<-- %p (%lu)\n", INDENT(call_stack_.size()), call,
              call_stack_.size());

        auto blacklist_fun = blacklisted_funs.find(op);
        if (blacklist_fun != blacklisted_funs.end()) {
            blacklist_stack_--;

            DEBUG("Exiting ignored fun %s (%d)\n",
                  blacklist_fun->second.c_str(), blacklist_stack_);

            return;
        }

        if (blacklist_stack_ != 0) {
            return;
        }

        switch (TYPEOF(op)) {
        case CLOSXP:
            trace_closure_call(op, rho, result);
            break;
#ifdef ENABLE_BUILTINS
        case BUILTINSXP:
            trace_builtin_call(op, args, rho, result);
            break;
#endif
#ifdef ENABLE_SPECIALS
        case SPECIALSXP:
            trace_builtin_call(op, args, rho, result);
            break;
#endif
        }
    }

    void context_entry(void* pointer) {
        TRACE("%s==> %p (%lu)\n", INDENT(call_stack_.size()), pointer,
              call_stack_.size());

        call_stack_.push_back(Frame(Context{pointer}));
    }

    void context_exit(void* pointer) {
        TRACE("%s<== %p (%lu) [%p]\n", INDENT(call_stack_.size() - 1), pointer,
              call_stack_.size() - 1,
              std::get<Context>(call_stack_.back()).context);

        call_stack_.pop_back();
    }

    void context_jump(void* pointer, SEXP retval, int restart) {
        TRACE("%sJUMP BEGIN (%p)\n", INDENT(call_stack_.size()), pointer);

        while (!call_stack_.empty()) {
            const auto& f = call_stack_.back();
            if (IS_CALL(f)) {
                const auto& call = std::get<Call>(f);
                SEXP call_retval = R_UnboundValue;

                // jumo_to_top_ex calls R_jumpctxt with retval=NULL
                if (!restart && retval != NULL && call_stack_.size() >= 2) {
                    const auto& next = call_stack_[call_stack_.size() - 2];
                    if (IS_CNTX(next) &&
                        std::get<Context>(next).context == pointer) {
                        call_retval = retval;
                    }
                }

                call_exit(call.call, call.op, call.args, call.rho, call_retval);
            } else if (IS_CNTX(f)) {
                const auto& cntx = std::get<Context>(f);
                if (cntx.context == pointer) {
                    break;
                } else {
                    context_exit(cntx.context);
                }
            }
        }

        TRACE("%sJUMP END\n", INDENT(call_stack_.size()));
    }
};

void call_entry(dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho) {
    auto state = (TracerState*)dyntracer_get_data(tracer);

    dyntrace_disable();
    state->call_entry(call, op, args, rho);
    dyntrace_enable();
}

void call_exit(dyntracer_t* tracer, SEXP call, SEXP op, SEXP args, SEXP rho,
               SEXP result) {
    auto state = (TracerState*)dyntracer_get_data(tracer);

    dyntrace_disable();
    state->call_exit(call, op, args, rho, result);
    dyntrace_enable();
}

void context_entry(dyntracer_t* tracer, void* pointer) {
    auto state = (TracerState*)dyntracer_get_data(tracer);

    dyntrace_disable();
    state->context_entry(pointer);
    dyntrace_enable();
}

void context_exit(dyntracer_t* tracer, void* pointer) {
    auto state = (TracerState*)dyntracer_get_data(tracer);

    dyntrace_disable();
    state->context_exit(pointer);
    dyntrace_enable();
}

void context_jump_callback(dyntracer_t* tracer, void* pointer, SEXP retval,
                           int restart) {
    auto state = (TracerState*)dyntracer_get_data(tracer);

    dyntrace_disable();
    state->context_jump(pointer, retval, restart);
    dyntrace_enable();
}

void tracing_start(dyntracer_t* tracer, SEXP expression, SEXP environment) {
    dyntrace_disable();

    auto state = (TracerState*)dyntracer_get_data(tracer);

    auto nss = PROTECT(env2list(R_NamespaceRegistry));
    for (int i = 0; i < Rf_length(nss); i++) {
        state->add_pending_namespace(VECTOR_ELT(nss, i));
    }
    UNPROTECT(1);

    dyntrace_enable();
}

// TODO: move to a better place
void initialize_globals() {
    if (!add_val_origin) {
        add_val_origin =
            (AddValOriginFun)R_GetCCallable("sxpdb", "add_val_origin_");
    }

    if (!loadNamespaceFun) {
        loadNamespaceFun =
            get_or_load_binding(R_BaseEnv, Rf_install("loadNamespace"));

        if (TYPEOF(loadNamespaceFun) != CLOSXP) {
            Rf_error("Unable to get address of loadNamespace");
        }
    }

    if (blacklisted_funs.empty()) {
        for (int i = 0; i < sizeof(BLACKLISTED_FUNS) / sizeof(char*); i++) {
            SEXP fun =
                get_or_load_binding(R_BaseEnv, Rf_install(BLACKLISTED_FUNS[i]));

            switch (TYPEOF(fun)) {
            case BUILTINSXP:
            case CLOSXP:
            case SPECIALSXP:
                blacklisted_funs.insert(
                    std::make_pair(fun, BLACKLISTED_FUNS[i]));
                break;
            default:
                Rf_error("Unable to load blacklisted function %s\n",
                         BLACKLISTED_FUNS[i]);
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

    dyntracer_set_dyntrace_entry_callback(tracer, &tracing_start);

    return tracer;
}

SEXP trace_code(SEXP db, SEXP code, SEXP rho) {
    initialize_globals();

    dyntracer_t* tracer = create_tracer();

    TracerState state(db);

    char* no_recording = getenv("ARGTRACER_NO_RECORDING");
    if (no_recording != NULL) {
        state.disable_recording();
        Rprintf("Tracer will not record!\n");
    }

    dyntracer_set_data(tracer, (void*)&state);

    dyntrace_enable();
    dyntrace_result_t result = dyntrace_trace_code(tracer, code, rho);
    dyntrace_disable();

    const char* names[] = {"status", "result", ""};
    SEXP ret = PROTECT(Rf_mkNamed(VECSXP, names));
    SET_VECTOR_ELT(ret, 0, Rf_ScalarInteger(result.error_code));
    SET_VECTOR_ELT(ret, 1, result.error_code == 0 ? result.value : R_NilValue);
    UNPROTECT(1);

    return ret;
}
