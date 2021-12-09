#' @export
trace_code <- function(db, code, quote = TRUE,
                       environment = parent.frame()) {
    if (quote) {
        code <- substitute(code)
    }

    # sxpdb uses TRACE frag
    tracingState(on = FALSE)
    invisible(.Call(C_trace_code, db, code, environment))
}

#' @export
#' @importFrom sxpdb open_db close_db
trace_file <- function(file, db_path = paste0(basename(file), ".sxpdb")) {
    db <- sxpdb::open_db(db_path)

    time <- c("elapsed" = NA)
    status <- NA
    db_size <- NA
    error <- NA

    tryCatch(
        {
            code <- parse(file = file)
            code <- as.call(c(`{`, code))
            time <- system.time(status <- trace_code(db, code, quote = FALSE))
            db_size <- sxpdb::size_db(db)
        },
        error = function(e) {
            message("Tracing ", file, " failed: ", e$message)
            error <<- e$message
            status <<- -1
        }
    )

    time <- time["elapsed"]

    sxpdb::close_db(db)

    data.frame(status = status, time = time, file = file,
               db_path = db_path, db_size = db_size, error = error)
}

test_trace_file <- function(file) {
    db_path <- tempfile(fileext = ".sxpdb")
    cat("*** Temp DB path: ", db_path, "\n")
    trace_file(file, db_path)
}
