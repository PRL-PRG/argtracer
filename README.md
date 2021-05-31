
# argtracer

<!-- badges: start -->
<!-- badges: end -->

The goal of argtracer is to collect R-eal values that are arguments and return values of function calls in the executable code from a corpus of CRAN packages

## Installation

TBC

<!-- ``` r -->
<!-- install.packages("lazr") -->
<!-- ``` -->

## Example

This example traces the arguments and return values that are associated with the call to stringr::str_detectrecursively 

``` r
library(argtracer)

trace_args({
    stringr::str_detect("ab", "a")
})
```
