start <- Sys.time()

my_add <- function(x, y) {x + y}
my_add(1, 1)

my_add <- function(x, y) {x + y}
my_add(2, 3)

stringr::str_detect("AB", "A")

ggplot2::aes_all(names(mtcars))
ggplot2::aes_all(c("x", "y", "col", "pch"))

end <- Sys.time()
end - start
