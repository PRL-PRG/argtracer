start <- Sys.time()

test_that("trace my_add1", {
  ## record::open_db("test_db/db_my_add", create = TRUE)

  r <- trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(1, 1)
  })

  expect_true(length(r$output$arguments) != 0)
  ## expect_equal(record::size_db(),  2)
  ## record::close_db("test_db/db_my_add")
})

test_that("trace my_add2", {
  ## record::open_db("test_db/db_my_add", create = FALSE)

  r <- trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(2, 3)
  })
  expect_true(length(r$output$arguments) != 0)
  ## expect_equal(record::size_db(), 5)
  ## record::close_db("test_db/db_my_add")
})

test_that("trace stringr::str_detect", {
  ## record::open_db("test_db/db_str_detect", create = TRUE)

  r <- trace_args(code = {
    stringr::str_detect("AB", "A")
  })

  expect_true(length(r$output$arguments) != 0)
  ## print(paste0("collected ", record::size_db(), " values from stringr::str_detect"))
  ## record::close_db("test_db/db_str_detect")
  ## expect_true(length(r$output$arguments) != 0)
  ## "collected 6417 values from stringr::str_detect"
})


test_that("trace ggplot2::aes_all", {
  ## record::open_db("test_db/db_aes_all", create = TRUE)

  r <- trace_args(code = {
    ggplot2::aes_all(names(mtcars))
    ggplot2::aes_all(c("x", "y", "col", "pch"))
  })
  ## print(paste0("collected ", record::size_db(), " values from ggplot2::aes_all"))
  ## record::close_db("test_db/db_aes_all")
  expect_true(length(r$output$arguments) != 0)
  ## "collected 105799 values from ggplot2::aes_all"  
})

end <- Sys.time()
print(end - start)

