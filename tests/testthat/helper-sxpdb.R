with_tempdb <- function(name, code, envir=parent.frame()) {
  local_env <- new.env(parent=envir)
  temp_dir <- tempdir("sxpdb")
  db <- sxpdb::open_db(temp_dir)

  assign(name, db, envir=local_env)

  on.exit({
    sxpdb::close_db(db)
    unlink(temp_dir, recursive=TRUE)
  })

  eval(substitute(code), envir=local_env)
}
