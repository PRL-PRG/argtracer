test_that("trace stringr::str_detect", {
  r <- trace_args(code = {
    stringr::str_detect("AB", "A")
  })
  browser()
  expect_true(length(r$output$arguments) != 0)
})

test_that("trace my_add", {
  r <- trace_args(code = {
    my_add <- function(x, y) {x + y}
    my_add(1L, 1L)
  })
  browser()
  expect_true(length(r$output$arguments) != 0)
})
