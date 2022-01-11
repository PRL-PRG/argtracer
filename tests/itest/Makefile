R       ?= R
OUT_DIR ?= out

all: run

run:
	$(R) -e 'targets::tar_make_future(workers=parallel::detectCores())'

clean:
	rm -fr $(OUT_DIR) _targets
