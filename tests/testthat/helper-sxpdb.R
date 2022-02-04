with_temp_db <- function(code, envir=parent.frame()) {
  local_env <- new.env(parent = envir)
  temp_dir <- tempfile(fileext = ".sxpdb")
  db <- sxpdb::open_db(temp_dir, write = TRUE)

  assign("db", db, envir = local_env)

  on.exit({
    sxpdb::close_db(db)
    unlink(temp_dir, recursive = TRUE)
  })

  eval(code, envir = local_env)
}

test_with_db <- function(name, code) {
    code <- substitute(code)
    test_that(name, with_temp_db(code))
}
