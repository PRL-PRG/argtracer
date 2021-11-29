#' @export
trace_code <- function(db,
                       code,
                       quote = TRUE,
                       environment = parent.frame()) {
    if (quote) {
        code <- substitute(code)
    }

    invisible(.Call(C_trace_code, db, code, environment))
}

#' @export
#' @importFrom sxpdb open_db close_db
trace_file <- function(file, db_path=paste0(basename(file), ".sxpdb")) {
    db <- sxpdb::open_db(db_path)

    tryCatch({
        code <- parse(file = file)
        code <- as.call(c(`{`, code))
        invisible(trace_code(db, code, quote = FALSE))
    }, error=function(e) {
        message("Tracing ", file, " failed: ", e$message)
    })

    sxpdb::close_db(db)
}
