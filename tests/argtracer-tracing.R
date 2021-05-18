library(argtracer)

start_tracing <- Sys.time()

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

end_tracing <- Sys.time()
end_tracing - start_tracing
