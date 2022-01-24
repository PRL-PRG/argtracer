devtools::load_all("data/testpkg")

test_with_db("test context jumps", {
    ret <- trace_code(db, {
        testpkg:::f_return(TRUE, 1L)
    })
    expect_equal(ret$status, 0L)
    expect_equal(ret$result, 2L)
    expect_equal(sxpdb::size_db(db), 3L)

    origins <- sxpdb::view_origins_db(db)

    expect_equal(unique(origins$pkg), "testpkg")
    expect_equal(unique(origins$fun), "f_return")
    expect_equal(sort(origins$param), c("return", "x", "y"))
})

test_with_db("empty code produces empty db", {
    ret <- trace_code(db, code = {})

    expect_equal(ret$status, 0L)
    expect_equal(ret$result, NULL)
    expect_equal(sxpdb::size_db(db), 0L)
})

test_with_db("function not from package are ignored", {
    ret <- trace_code(db, code = {
        f <- function(x) x + 1L
        f(1L)
    })

    expect_equal(ret$status, 0L)
    expect_equal(ret$result, 2L)
    expect_equal(sxpdb::size_db(db), 0L)
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

    expect_equal(ret$status, 0L)
    expect_equal(ret$result[1L], "tools")

    db <- sxpdb::open_db(db_path)
    expect_equal(sxpdb::size_db(db), 0L)
})

test_with_db("basic test", {
    ret <- trace_code(db, code = testpkg:::f_simple(1L, 2L))

    expect_equal(ret$status, 0L)
    expect_equal(ret$result, 3L)
    expect_equal(sxpdb::size_db(db), 3)

    origins <- sxpdb::view_origins_db(db)

    expect_equal(unique(origins$pkg), "testpkg")
    expect_equal(unique(origins$fun), "f_simple")
    expect_equal(sort(origins$param), c("return", "x", "y"))
})

test_with_db("skip unevaluated promises", {
    ret <- trace_code(db, code = testpkg:::f_simple_2_of_3(1L, { fail() }, 3L))

    expect_equal(ret$status, 0L)
    expect_equal(ret$result, 4L)
    expect_equal(sxpdb::size_db(db), 3)

    origins <- sxpdb::view_origins_db(db)

    expect_equal(unique(origins$pkg), "testpkg")
    expect_equal(unique(origins$fun), "f_simple_2_of_3")
    expect_equal(sort(origins$param), c("return", "x", "z"))
})

test_with_db("test varargs", {
    ret <- trace_code(db, code = testpkg:::f_varargs(1L, 2L, 3L))

    expect_equal(ret$status, 0L)
    expect_equal(ret$result, c(1L, 2L, 3L))
    expect_equal(sxpdb::size_db(db), 4)

    origins <- sxpdb::view_origins_db(db)

    expect_equal(unique(origins$pkg), "testpkg")
    expect_equal(unique(origins$fun), "f_varargs")
    expect_equal(sort(origins$param), c("..1", "..2", "..3", "return"))
})

test_with_db("error is OK", {
    ret <- trace_code(db, code = testpkg:::f_error(TRUE, 1L))

    expect_equal(ret$status, 1L)
    expect_equal(ret$result, NULL)

    origins <- sxpdb::view_origins_db(db)

    expect_equal(sum(origins$fun == "f_error"), 1L)
})

test_with_db("warning is OK", {
    ret <- trace_code(db, code = testpkg:::f_warning(TRUE, 1L))

    expect_equal(ret$status, 0L)
    expect_equal(ret$result, 1L)

    origins <- sxpdb::view_origins_db(db)

    # return, x, y
    expect_equal(sum(origins$fun == "f_warning"), 3L)
})
