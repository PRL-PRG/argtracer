library(tarchetypes)
library(targets)
library(tibble)

R_VANILLA <- "../../../R-vanilla"
R_DYNTRACE <- "../../../R-dyntrace"
R_VANILLA_GDB <- "../../../R-vanilla-gdb"
R_DYNTRACE_GDB <- "../../../R-dyntrace-gdb"

R_VMS <- tribble(
    ~name, ~path,
    "vanilla", R_VANILLA,
    "vanilla-gdb", R_VANILLA_GDB,
    "dyntrace", R_DYNTRACE,
    "dyntrace-gdb", R_DYNTRACE_GDB
)

# maximum time allowed for a program to run in seconds
TIMEOUT <- 15 * 60

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
                split_testthat = FALSE,
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
                keep_output = "on_error"
            ) %>%
                add_column(R = r_vms$name)
        },
        pattern = cross(r_vms, programs_files_)
    ),
    tar_target(
        run,
        left_join(programs_metadata, programs_run, by = "file")
    )
)
