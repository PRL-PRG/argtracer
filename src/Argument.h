#ifndef ARGTRACER_ARGUMENT_H
#define ARGTRACER_ARGUMENT_H

#include <string>

class Argument {
  public:
    Argument(int arg_id,
             int call_id,
             int fun_id,
             int call_env_id,
             const std::string& arg_name,
             int formal_pos,
             int dot_pos,
             int default_arg,
             int vararg,
             int missing,
             const std::string& arg_type,
             const std::string& expr_type,
             const std::string& val_type)
             // int preforced)
        : arg_id_(arg_id)
        , call_id_(call_id)
        , fun_id_(fun_id)
        , call_env_id_(call_env_id)
        , arg_name_(arg_name)
        , formal_pos_(formal_pos)
        , dot_pos_(dot_pos)
        // , force_pos_(NA_INTEGER)
        // , actual_pos_(NA_INTEGER)
        , default_arg_(default_arg)
        , vararg_(vararg)
        , missing_(missing)
        , arg_type_(arg_type)
        , expr_type_(expr_type)
        , val_type_(val_type) {}

    int get_id() {
        return arg_id_;
    }

    int get_call_id() {
        return call_id_;
    }

    int get_fun_id() {
        return fun_id_;
    }

    int get_formal_pos() {
        return formal_pos_;
    }

    // void set_force_position(int force_pos) {
    //   force_pos_ = force_pos;
    // }
  
    void to_sexp(int index,
                 SEXP r_arg_id,
                 SEXP r_call_id,
                 SEXP r_fun_id,
                 SEXP r_call_env_id,
                 SEXP r_arg_name,
                 SEXP r_formal_pos,
                 SEXP r_dot_pos,
                 // SEXP r_force_pos,
                 // SEXP r_actual_pos,
                 SEXP r_default_arg,
                 SEXP r_vararg,
                 SEXP r_missing,
                 SEXP r_arg_type,
                 SEXP r_expr_type,
                 SEXP r_val_type) {
        SET_INTEGER_ELT(r_arg_id, index, arg_id_);
        SET_INTEGER_ELT(r_call_id, index, call_id_);
        SET_INTEGER_ELT(r_fun_id, index, fun_id_);
        SET_INTEGER_ELT(r_call_env_id, index, call_env_id_);
        SET_STRING_ELT(r_arg_name, index, make_char(arg_name_));
        SET_INTEGER_ELT(r_formal_pos, index, formal_pos_);
        SET_INTEGER_ELT(r_dot_pos, index, dot_pos_);
        // SET_INTEGER_ELT(r_force_pos, index, force_pos_);
        // SET_INTEGER_ELT(r_actual_pos, index, actual_pos_);
        SET_LOGICAL_ELT(r_default_arg, index, default_arg_);
        SET_LOGICAL_ELT(r_vararg, index, vararg_);
        SET_LOGICAL_ELT(r_missing, index, missing_);
        SET_STRING_ELT(r_arg_type, index, make_char(arg_type_));
        SET_STRING_ELT(r_expr_type, index, make_char(expr_type_));
        SET_STRING_ELT(r_val_type, index, make_char(val_type_));
    }

  private:
    int arg_id_;
    int call_id_;
    int fun_id_;
    int call_env_id_;
    std::string arg_name_;
    int formal_pos_;
    int dot_pos_;
    // int force_pos_;
    // int actual_pos_;
    int default_arg_;
    int vararg_;
    int missing_;
    std::string arg_type_;
    std::string expr_type_;
    std::string val_type_;
    // int preforced_;
    // int cap_force_;
    // int cap_meta_;
    // int cap_lookup_;
    // int escaped_;
    // int esc_force_;
    // int esc_meta_;
    // int esc_lookup_;
    // int con_force_;
    // int con_lookup_;
    // int force_depth_;
    // int meta_depth_;
    // int comp_pos_;
    // std::string event_seq_;
    // std::string self_effect_seq_;
    // std::string effect_seq_;
    // std::vector<std::pair<std::string, int>> self_ref_seq_;
    // std::vector<std::pair<std::string, int>> ref_seq_;
    // int parent_fun_id_;
    // int parent_formal_pos_;
    // int parent_call_id_;
    // int parent_arg_id_;

    // void add_event_(char event) {
    //     event_seq_.push_back(event);
    // }

    // void add_effect_(std::string& effect_seq, const char type) {
    //     effect_seq.push_back(type);
    // }

    // void add_ref_(std::vector<std::pair<std::string, int>>& ref_seq,
    //               const std::string& name) {
    //     if (ref_seq.empty()) {
    //         ref_seq.push_back({name, 1});
    //     }

    //     else if (ref_seq.back().first == name) {
    //         ++ref_seq.back().second;
    //     }

    //     else {
    //         ref_seq.push_back({name, 1});
    //     }
    // }
};

#endif /* ARGTRACER_ARGUMENT_H */
