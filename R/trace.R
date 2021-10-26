#' @export
#' @importFrom instrumentr trace_code
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
#' @importFrom sxpdb open_db close_db
trace_file <- function(file, db_path=paste0(basename(file), ".sxpdb")) {
    if (dir.exists(db_path)) {
        unlink(db_path, recursive=T)
    }
    sxpdb::open_db(db_path, create = TRUE)

    tryCatch({
        code <- parse(file = file)
        code <- as.call(c(`{`, code))
        invisible(trace_args(code, quote = FALSE))
    }, error=function(e) {
        message("Tracing ", file, " failed: ", e$message)
    })

    sxpdb::close_db()
}
