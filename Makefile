BEAR := $(shell command -v bear 2> /dev/null)

.PHONY: all build check clean document test install

all: install

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

install: clean
ifdef BEAR
	bear -- R CMD INSTALL .
else
	R CMD INSTALL .
endif
