library(tarchetypes)
library(targets)
library(tibble)

R_VANILLA <- "../../../R-vanilla"
R_DYNTRACE <- "../../../R-dyntrace"
R_VANILLA_GDB <- "../../../R-vanilla-gdb"
R_DYNTRACE_GDB <- "../../../R-dyntrace-gdb"

R_VMS <- tribble(
    ~name, ~path,
    "vanilla", R_VANILLA#,
    # "vanilla-gdb", R_VANILLA_GDB,
    # "dyntrace", R_DYNTRACE,
    # "dyntrace-gdb", R_DYNTRACE_GDB
)

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

out_dir <- "out"
if (!dir.exists(out_dir)) {
    dir.create(out_dir)
}

lib_dir <- file.path(out_dir, "library")
pkg_zip_dir <- file.path(out_dir, "packages", "zip")
pkg_dir <- file.path(out_dir, "packages", "extracted")
programs_dir <- file.path(out_dir, "programs")

tar_option_set(
    packages = c("runr", "readr", "covr", "magrittr", "dplyr", "stringr"),
)

list(
    tar_target(
        packages_file,
        "packages.txt",
        format = "file"
    ),
    tar_target(
        packages_to_install,
        unique(trimws(read_lines(packages_file)))
    ),
    tar_target(
        packages_bin,
        {
            runr::install_cran_packages(
                packages_to_install,
                lib_dir,
                r_home = R_VANILLA
            ) %>% pull(dir)
        },
        format = "file"
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
        format = "file"
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
    tar_target(r_vms, R_VMS),
    tar_target(
        programs_run,
        {
            runr::run_r_file(
                programs_files_,
                timeout = TIMEOUT,
                r_home = r_vms$path,
                lib_path = lib_dir,
                keep_output = "on_error",
                r_envir = R_ENVIR
            ) %>%
                add_column(R = r_vms$name)
        },
        pattern = cross(r_vms, programs_files_)
    ),
    tar_target(
        run,
        left_join(programs_metadata, programs_run, by = "file")
    ),
    # install different versions of the package using devtools
    # each into new directory and then use that one together with the lib_dir
    tar_target(
        programs_trace,
        {
            tmp_db <- tempfile(fileext = ".sxpdb")
            file <- normalizePath(programs_files_)
            withr::defer(unlink(tmp_db, recursive = TRUE))
            callr::r(
                function(file, db) {
                    res <- argtracer::trace_file(file, db)
                    print(res)
                    res
                },
                list(file, tmp_db),
                libpath = normalizePath(lib_dir),
                arch = normalizePath(file.path(R_DYNTRACE, "bin", "R"), mustWork = TRUE),
                show = TRUE,
                wd = dirname(file),
                env = R_ENVIR,
                timeout = TIMEOUT
            )
        },
        pattern = map(programs_files_)
    )
)
