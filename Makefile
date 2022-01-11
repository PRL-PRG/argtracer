BEAR := $(shell command -v bear 2> /dev/null)
R    ?= R

ifdef BEAR
	BEAR := $(BEAR) --
endif

.PHONY: all build check clean document test install

all: install

build: document
	$(R) CMD build .

check: build
	$(R) CMD check argtracer*tar.gz

clean:
	-rm -f argtracer*tar.gz
	-rm -fr argtracer.Rcheck
	-rm -rf src/*.o src/*.so
	-rm -rf tests/testthat/test_db/*

document:
	$(R) -e 'devtools::document()'

test:
	$(R) -e 'devtools::test()'

itest:
	$(MAKE) -C tests/itest

itest-docker:
	docker run \
	    -ti --rm \
        -v $$(pwd):/argtracer \
        -e TZ=/usr/share/zoneinfo/Europe/Prague \
        -e OUT_DIR=/tmp/out \
        -e LIB_DIR=/R/R-dyntrace/library \
        -e PKG_DIR=/CRAN/extracted \
        prlprg/argtracer \
        make -C /argtracer/tests/itest

install: clean
	$(BEAR) $(R) CMD INSTALL .
