#' @export
trace_code <- function(code,
                       quote = TRUE,
                       environment = parent.frame(),
                       db = db) {
    if (quote) {
        code <- substitute(code)
    }

    invisible(.Call(C_trace_code, code, environment))
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

    sxpdb::close_db(db)
}
