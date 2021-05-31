if (TRUE) {

  test_that("trace my_add1", {
  start <- Sys.time()
  record::open_db("test_db/db_my_add", create = TRUE)

  r <- trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(1, 1)
  })

  expect_equal(record::size_db(),  2)
  record::close_db()
  end <- Sys.time()
  print(end - start)
  })


test_that("trace my_add2", {
  start <- Sys.time()
  record::open_db("test_db/db_my_add", create = FALSE)

  r <- trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(2, 3)
  })

  expect_equal(record::size_db(), 4)
  record:::report()
  record::close_db()
  end <- Sys.time()
  print(end - start)
})


test_that("trace all", {
  start <- Sys.time()
  record::open_db("test_db/db_all", create = TRUE)

  if_true <- function (x) {
    if (add_one(x) == 2) TRUE else FALSE
  }
  add_one <- function (x) { x + 1 }
  r <- trace_args(code = {
    my_fun <- function(x, y) {
      if (if_true(x)) y else x
    }
    my_fun (1,2) # collect 1, 2, TRUE
    my_fun (3,0) # collect 3, 4, FALSE          0 is not forced, so is not collected
  })

  expect_equal(record::size_db(), 6)
  record:::report()
  record::close_db()
  end <- Sys.time()
  print(end - start)
})


test_that("trace stringr::str_detect", {
  start <- Sys.time()
  record::open_db("test_db/db_str_detect", create = TRUE)

  r <- trace_args(code = {
    stringr::str_detect("AB", "A")
  })

  expect_equal(record::size_db(), 1213)
  record:::report()
  record::close_db()
  end <- Sys.time()
  print(end - start)
  ## "collected 6417 promises + return vals from stringr::str_detect"
})


  test_that("trace ggplot2::aes_all", {
  start <- Sys.time()
  record::open_db("test_db/db_aes_all", create = TRUE)

  r <- trace_args(code = {
    ggplot2::aes_all(names(mtcars))
    ggplot2::aes_all(c("x", "y", "col", "pch"))
  })

  expect_equal(record::size_db(), 9958)
  record:::report()
  record::close_db()
  end <- Sys.time()
  print(end - start)
  ## "collected 105799 promises + return vals from ggplot2::aes_all"  
})

} else {

test_that("trace my_add1", {
  start <- Sys.time()

  r <- trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(1, 1)
  })

  end <- Sys.time()
  print(end - start)
})


test_that("trace my_add2", {
  start <- Sys.time()

  r <- trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(2, 3)
  })

  end <- Sys.time()
  print(end - start)
})


test_that("trace all", {
  start <- Sys.time()

  if_true <- function (x) {
    if (add_one(x) == 2) TRUE else FALSE
  }
  add_one <- function (x) { x + 1 }
  r <- trace_args(code = {
    my_fun <- function(x, y) {
      if (if_true(x)) y else x
    }
    my_fun (1,2) # collect 1, 2, TRUE
    my_fun (3,0) # collect 3, 4, FALSE          0 is not forced, so is not collected
  })

  browser()
  end <- Sys.time()
  print(end - start)
})


test_that("trace stringr::str_detect", {
  start <- Sys.time()

  r <- trace_args(code = {
    stringr::str_detect("AB", "A")
  })

  end <- Sys.time()
  print(end - start)
  ## "collected 6417 promises + return vals from stringr::str_detect"
})


test_that("trace ggplot2::aes_all", {
  start <- Sys.time()

  r <- trace_args(code = {
    ggplot2::aes_all(names(mtcars))
    ## ggplot2::aes_all(c("x", "y", "col", "pch"))
  })

  end <- Sys.time()
  print(end - start)
  ## "collected 105799 promises + return vals from ggplot2::aes_all"  
})

}
