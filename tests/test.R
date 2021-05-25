library(argtracer)

start <- Sys.time()

record::open_db("test_db/db_aes_all", create = TRUE)
r <- trace_args(code = {
  ggplot2::aes_all(names(mtcars))
  ## ggplot2::aes_all(c("x", "y", "col", "pch"))
})

record:::report()
record::close_db()

end <- Sys.time()
print(end - start)
## expect_equal(record::size_db(), 9958)
