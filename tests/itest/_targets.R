library(tarchetypes)
library(targets)
library(tibble)
library(future)
library(future.callr)

create_dir <- function(...) {
    p <- file.path(...)
    if (!dir.exists(p)) dir.create(p, recursive = TRUE)
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
OUT_DIR <- "out"
# packages to test
CORPUS <- c("dplyr")

tar_option_set(
    packages = c("DT", "devtools", "dplyr", "runr"),
)

plan(callr)

list(
    tar_target(lib_dir, create_dir(OUT_DIR, "library"), format="file"),
    tar_target(pkg_dir, create_dir(OUT_DIR, "packages", "extracted"), format="file"),
    tar_target(programs_dir, create_dir(OUT_DIR, "programs"), format="file"),
    tar_target(
        sxpdb_,
        {
            devtools::install_github("PRL-PRG/sxpdb", lib = lib_dir)
            file.path(lib_dir, "sxpdb")
        },
        format = "file"
    ),
    tar_target(
        argtracer_,
        {
            sxpdb_
            withr::with_libpaths(lib_dir, devtools::install_local(path = "../.."))
            file.path(lib_dir, "argtracer")
        },
        format = "file"
    ),
    tar_target(
        packages_bin,
        {
            runr::install_cran_packages(
                CORPUS,
                lib_dir,
                dependencies = c("Depends", "Imports", "LinkingTo"),
                check = TRUE
            ) %>% pull(dir)
        },
        format = "file",
        deployment = "main"
    ),
    tar_target(
        packages,
        {
            package <- basename(packages_bin)
            version <- sapply(package, function(x) as.character(packageVersion(x)))
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
        programs_files,
        {
            output_dir <- file.path(programs_dir, packages$package)
            res <- runr::extract_package_code(
                packages$package,
                pkg_dir = packages_src,
                output_dir = output_dir,
                split_testthat = SPLIT_TESTTHAT_TESTS,
                compute_sloc = FALSE
            )
            file.path(output_dir, res$file)
        },
        format = "file",
        pattern = map(packages, packages_src)
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
            argtracer_
            tmp_db <- tempfile(fileext = ".sxpdb")
            file <- normalizePath(programs_files_)
            withr::defer(unlink(tmp_db, recursive = TRUE))
            callr::r(
                function(...) argtracer::trace_file(...),
                list(file, tmp_db),
                libpath = normalizePath(lib_dir),
                show = TRUE,
                wd = dirname(file),
                env = R_ENVIR,
                timeout = TIMEOUT
            )
        },
        pattern = map(programs_files_)
    ),
    tar_render(report, "report.Rmd")
)
