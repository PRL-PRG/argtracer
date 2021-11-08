#' @export
#' @importFrom instrumentr trace_code
trace_args <- function(code,
                       environment = parent.frame(),
                       quote = TRUE,
                       db = db) {
    argtracer <- .Call(C_argtracer_create, db = db)

    if(quote) {
        code <- substitute(code)
    }

    invisible(trace_code(argtracer, code, environment = environment, quote = FALSE))
}

#' @export
#' @importFrom sxpdb open_db close_db
trace_file <- function(file, db_path=paste0(basename(file), ".sxpdb")) {
    db <- sxpdb::open_db(db_path)

    tryCatch({
        code <- parse(file = file)
        code <- as.call(c(`{`, code))
        invisible(trace_args(code, quote = FALSE, db = db))
    }, error=function(e) {
        message("Tracing ", file, " failed: ", e$message)
    })

    sxpdb::close_db()
}
