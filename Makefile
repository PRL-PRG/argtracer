.PHONY: all build check document test

all: document build check install

build: document
	R CMD build .

check: build
	R CMD check argtracer*tar.gz

clean:
	-rm -f argtracer*tar.gz
	-rm -fr argtracer.Rcheck
	-rm -rf src/*.o src/*.so
	-rm -rf tests/testthat/test_db/*

document:
	R -e 'devtools::document()'

test:
	R -e 'devtools::test()'

lintr:
	R --slave -e "lintr::lint_package()"

install: clean
	R CMD INSTALL .
