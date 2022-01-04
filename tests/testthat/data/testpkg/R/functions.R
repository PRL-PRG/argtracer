f_simple <- function(x, y) {
    x + y
}

f_simple_2_of_3 <- function(x, y, z) {
    x + z
}

f_calling_external <- function(x) {
    utils::head(x)
}

f_varargs <- function(...) {
    c(...)
}

f_missing <- function(x, y) {
    if (missing(y)) x else x + y
}

f_warning <- function(x, y) {
    if (x) {
        warning("f_warning")
        y
    } else {
        y
    }
}

f_error <- function(x, y) {
    if (x) {
        stop("f_error")
    } else {
        y
    }
}

f_return <- function(x, y) {
    if (x) {
        return(y+1L)
    } else {
        0L
    }
}
