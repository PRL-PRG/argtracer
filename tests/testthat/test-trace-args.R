test_with_db("empty code produces empty db", {
    ret <- trace_code(db, code = {})

    expect_equal(ret$status, 0)
    expect_equal(ret$result, NULL)
    expect_equal(sxpdb::size_db(db), 0)
})

test_with_db("function not from package are ignored", {
    ret <- trace_code(db, code = {
        f <- function(x) x + 1
        f(1)
    })

    expect_equal(ret$status, 0)
    expect_equal(ret$result, 2)
    expect_equal(sxpdb::size_db(db), 0)
})

test_that("ignored functions", {
    db_path <- tempfile()
    on.exit(unlink(db_path, recursive = TRUE))

    ret <- callr::r_copycat(
        function(db_path) {
            db <- sxpdb::open_db(db_path)
            ret <- argtracer::trace_code(db, code = {
                library(tools)
            })
            sxpdb::close_db(db)
            ret
        },
        list(db_path = db_path)
    )

    expect_equal(ret$status, 0)
    expect_equal(ret$result[1], "tools")

    db <- sxpdb::open_db(db_path)
    expect_equal(sxpdb::size_db(db), 0)
})

test_with_db("basic test", {
    ret <- trace_code(db, code = {
        my_paste <- function(x, y) {
            paste(x, y)
        }

        my_paste(1, 2)
    })

    expect_equal(ret$status, 0)
    expect_equal(ret$result[1], "1 2")

    expect_equal(sxpdb::size_db(db), 6)

    origins <- do.call(rbind, sxpdb::view_origins_db(db))

    expect_equal(unique(origins$pkg), "base")
    expect_equal(
        sort(unique(origins$fun)),
        c("isTRUE", "paste")
    )

    expect_equal(
        sort(subset(origins, fun == "paste")$param),
        c("..1", "..2", "collapse", "recycle0", "return", "sep")
    )

    expect_equal(
        sort(subset(origins, fun == "isTRUE")$param),
        c("return", "x")
    )
})

test_with_db("error is OK", {
    ret <- trace_code(db, {
        stop("testing an error in tracing code")
    })

    expect_equal(ret$status, 1)
    expect_equal(ret$result, NULL)
})

test_with_db("warning is OK", {
    ret <- trace_code(db, {
        warning("testing a warning in tracing code")
        TRUE
    })

    expect_equal(ret$status, 0)
    expect_equal(ret$result, TRUE)
})

# test_that("db works with ...", {
#   db_path <- tempdir()
#   on.exit(unlink(db_path, recursive=TRUE))
#
#   db <- sxpdb::open_db(db_path)
#
#   r <- trace_args(code = {
#     my_add <- function(x, ...) {
#       xs <- list(...)
#       sum(c(x, xs))
#     }
#
#     my_add(1, 2, 3)
#   }, db = db)
#
#   browser()
#   expect_equal(sxpdb::size_db(db),  4)
#   sxpdb::close_db(db)
# })

#   test_that("trace my_add2", {
#   start <- Sys.time()
#   record::open_db("test_db/db_my_add", create = FALSE)
#
#   r <- trace_args(code = {
#     my_add <- function(x, y) {x + y}
#     my_add(2, 3)
#   })
#
#   expect_equal(record::size_db(), 4)
#   record:::report()
#   record::close_db()
#   end <- Sys.time()
#   print(end - start)
# })
#
#
#   test_that("trace all", {
#     start <- Sys.time()
#     record::open_db("test_db/db_all", create = TRUE)
#
#     if_true <- function (x) {
#       if (add_one(x) == 2) TRUE else FALSE
#     }
#     add_one <- function (x) { x + 1 }
#
#     r <- trace_args(code = {
#       my_fun <- function(x, y) {
#         if (if_true(x)) y else x
#       }
#       my_fun (1,2) # collect 1, 2, TRUE
#       my_fun (3,0) # collect 3, 4, FALSE          0 is not forced, so is not collected
#     })
#
#     expect_equal(record::size_db(), 6)
#     record:::report()
#     record::close_db()
#     end <- Sys.time()
#     print(end - start)
# })
#
#   test_that("trace recursively", {
#     start <- Sys.time()
#     record::open_db("test_db/db_rec", create = TRUE)
#
#     add_one <- function (x) { x + 1 }
#     if_true <- function (x) {
#       add_one(100)                           # 100, 101
#       if (add_one(x) == 2) TRUE else FALSE   # 12, FALSE
#     }
#
#     r <- trace_args(code = {
#       my_fun <- function(x, y) {
#         add_one(0)                           # 0, 1
#         if (if_true(x)) y else x
#       }
#       my_fun(add_one(10),2)                  # 10, 11
#     })
#
#     expect_equal(record::size_db(), 8)
#     record:::report()
#     record::close_db()
#     end <- Sys.time()
#     print(end - start)
#   })
#
#
# test_that("trace stringr::str_detect", {
#   record::open_db("test_db/db_str_detect", create = TRUE)
#
#   r <- trace_args(code = {
#     stringr::str_detect("AB", "A")
#   })
#
#   expect_equal(record::size_db(), 16)
#   ## 1213
#   record:::report()
#   record::close_db()
# })
#
#
# test_that("trace ggplot2::aes_all", {
#   start <- Sys.time()
#   record::open_db("test_db/db_aes_all", create = TRUE)
#
#   r <- trace_args(code = {
#     ggplot2::aes_all(names(mtcars))
#     ggplot2::aes_all(c("x", "y", "col", "pch"))
#   })
#
#   expect_equal(record::size_db(), 85)
#   ## 9958
#   record:::report()
#   record::close_db()
#   end <- Sys.time()
#   print(end - start)
#   ## "collected 10599 promises + return vals from ggplot2::aes_all"
# })
#
#   test_that("check tracing package loading", {
#     record::open_db("test_db/db_pl", create = TRUE)
#
#     r <- trace_args(code = {
#       library(ggplot2)
#     })
#
#     expect_equal(record::size_db(), 292)
#     record::report()
#     record::close_db()
#   })
#
# } else {
#
# test_that("trace my_add1", {
#   start <- Sys.time()
#
#   r <- trace_args(code = {
#     my_add <- function(x, y) {x + y}
#     my_add(1, 1)
#   })
#
#   end <- Sys.time()
#   print(end - start)
# })
#
#
# test_that("trace my_add2", {
#   start <- Sys.time()
#
#   r <- trace_args(code = {
#     my_add <- function(x, y) {x + y}
#     my_add(2, 3)
#   })
#
#   end <- Sys.time()
#   print(end - start)
# })
#
#
# test_that("trace all", {
#   start <- Sys.time()
#
#   if_true <- function (x) {
#     if (add_one(x) == 2) TRUE else FALSE
#   }
#   add_one <- function (x) { x + 1 }
#   r <- trace_args(code = {
#     my_fun <- function(x, y) {
#       if (if_true(x)) y else x
#     }
#     my_fun (1,2) # collect 1, 2, TRUE
#     my_fun (3,0) # collect 3, 4, FALSE          0 is not forced, so is not collected
#   })
#
#   browser()
#   end <- Sys.time()
#   print(end - start)
# })
#
#
# test_that("trace stringr::str_detect", {
#   start <- Sys.time()
#
#   r <- trace_args(code = {
#     stringr::str_detect("AB", "A")
#   })
#
#   end <- Sys.time()
#   print(end - start)
#   ## "collected 6417 promises + return vals from stringr::str_detect"
# })
#
#
# test_that("trace ggplot2::aes_all", {
#   start <- Sys.time()
#
#   r <- trace_args(code = {
#     ggplot2::aes_all(names(mtcars))
#     ## ggplot2::aes_all(c("x", "y", "col", "pch"))
#   })
#
#   end <- Sys.time()
#   print(end - start)
#   ## "collected 105799 promises + return vals from ggplot2::aes_all"
# })
#
# }
