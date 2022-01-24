library(tarchetypes)
library(targets)
library(tibble)
library(future)
library(future.callr)

create_dir <- function(...) {
    p <- file.path(...)
    if (!dir.exists(p)) stopifnot(dir.create(p, recursive = TRUE))
    p
}

R_ENVIR <- c(
    callr::rcmd_safe_env(),
    "R_KEEP_PKG_SOURCE" = 1,
    "R_ENABLE_JIT" = 0,
    "R_COMPILE_PKGS" = 0,
    "R_DISABLE_BYTECODE" = 1
)

# maximum time allowed for a program to run in seconds
TIMEOUT <- 5 * 60
SPLIT_TESTTHAT_TESTS <- TRUE
OUT_DIR <- Sys.getenv("OUT_DIR", "out")
LIB_DIR <- Sys.getenv("LIB_DIR", file.path(OUT_DIR, "library"))
PKG_DIR <- Sys.getenv("PKG_DIR", file.path(OUT_DIR, "packages"))
PROGRAMS_DIR <- Sys.getenv("PROGRAMS_DIR", file.path(OUT_DIR, "programs"))

# packages to test
CORPUS <- c("dplyr")

tar_option_set(
    packages = c("DT", "devtools", "dplyr", "runr"),
)

tar_config_set(
    store = file.path(OUT_DIR, "_targets")
)

plan(callr)

print(OUT_DIR)
print(LIB_DIR)
print(PKG_DIR)
print(PROGRAMS_DIR)

lib_dir <- create_dir(LIB_DIR)
pkg_dir <- create_dir(PKG_DIR)
programs_dir <- create_dir(PROGRAMS_DIR)

list(
    tar_target(
        pkg_argtracer,
        {
            if (Sys.getenv("IN_DOCKER", "0") != "1") {
                withr::with_libpaths(lib_dir, devtools::install_local(path = "../..", upgrade = FALSE))
            }
            file.path(lib_dir, "argtracer")
        },
        format = "file",
        deployment = "main"
    ),
    tar_target(
        packages_bin,
        {
            runr::install_cran_packages(
                CORPUS,
                lib_dir,
                dependencies = TRUE,
                check = FALSE
            ) %>% pull(dir)
        },
        format = "file",
        deployment = "main"
    ),
    tar_target(
        packages,
        {
            package <- basename(packages_bin)
            version <- sapply(package, function(x) as.character(packageVersion(x, lib.loc = lib_dir)))
            tibble(package, version)
        }
    ),
    tar_target(
        packages_src,
        runr::download_cran_package_source(
            packages$package,
            packages$version,
            pkg_dir
        ),
        pattern = map(packages),
        format = "file",
        deployment = "main"
    ),
    tar_target(
        packages_bin_,
        packages_bin
    ),
    tar_target(
        programs_files,
        {
            output_dir <- file.path(programs_dir, packages$package)
            res <- runr::extract_package_code(
                packages$package,
                pkg_dir = packages_bin_,
                output_dir = output_dir,
                split_testthat = SPLIT_TESTTHAT_TESTS,
                compute_sloc = FALSE
            )
            file.path(output_dir, res$file)
        },
        format = "file",
        pattern = map(packages, packages_bin_)
    ),
    tar_target(
        programs_metadata,
        {
            output_dir <- file.path(programs_dir, packages$package)
            sloc <- cloc(output_dir, by_file = TRUE, r_only = TRUE) %>%
                select(file = filename, code)
            res <- tibble(file = programs_files, package = packages$package, type = basename(dirname(file)))
            left_join(res, sloc, by = "file")
        },
        pattern = map(packages)
    ),
    # dummy target to flatten program_files
    tar_target(programs_files_, programs_files),
    tar_target(
        programs_run,
        {
            runr::run_r_file(
                programs_files_,
                timeout = TIMEOUT,
                lib_path = lib_dir,
                keep_output = "on_error",
                r_envir = R_ENVIR
            )
        },
        pattern = map(programs_files_)
    ),
    tar_target(
        run,
        left_join(programs_metadata, programs_run, by = "file")
    ),
    tar_target(
        programs_trace,
        {
            pkg_argtracer
            tmp_db <- tempfile(fileext = ".sxpdb")
            file <- normalizePath(programs_files_)
            withr::defer(unlink(tmp_db, recursive = TRUE))
            tryCatch(
                callr::r(
                    function(...) argtracer::trace_file(...),
                    list(file, tmp_db),
                    libpath = normalizePath(lib_dir),
                    show = TRUE,
                    wd = dirname(file),
                    env = R_ENVIR,
                    timeout = TIMEOUT
                ),
                error = function(e) {
                    data.frame(status = -2, time = NA, file = file,
                               db_path = NA, db_size = NA, error = e$message)
                }
            )
        },
        pattern = map(programs_files_)
    ),
    tar_render(report, "report.Rmd")
)
