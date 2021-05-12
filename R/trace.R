
#' @export
#' @importFrom instrumentr trace_code get_exec_stats
trace_args <- function(code,
                       environment = parent.frame(),
                       quote = TRUE) {
    argtracer <- .Call(C_argtracer_create)

    if(quote) {
        code <- substitute(code)
    }

    invisible(trace_code(argtracer, code, environment = environment, quote = FALSE))
}

#' @export
trace_file <- function(file, environment = parent.frame()) {
    code <- parse(file = file)

    code <- as.call(c(`{`, code))

    invisible(trace_expr(code, quote = FALSE))
}
