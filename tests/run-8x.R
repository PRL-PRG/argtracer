#!/usr/bin/env Rscript

library(argtracer)

log <- data.frame(time = double(0))

for (i in 1:7) {
  start_tracing <- Sys.time()
  record::open_db(paste0("testthat/test_db/test", i), create=TRUE)

  trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(1, 1)
  })

  trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(2, 3)
  })

  trace_args(code = {
    stringr::str_detect("AB", "A")
  })

  trace_args(code = {
    ggplot2::aes_all(names(mtcars))
    ggplot2::aes_all(c("x", "y", "col", "pch"))
  })

  record::close_db(paste0("testthat/test_db/test", i))

  end_tracing <- Sys.time()
  log[i, ] <- end_tracing - start_tracing
}

write.csv(log, "time-log.csv", row.names=FALSE)
