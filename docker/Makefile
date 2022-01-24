# saner makefile
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

CRAN_MIRROR := https://cran.r-project.org
DEPENDENCIES_TXT := dependencies.txt
DOCKER_IMAGE_NAME := prlprg/argtracer

# the $(2) can be in form ?sha=<branch> to fetch a branch
define GH_REPO_LAST_COMMIT
$(shell 2>/dev/null curl -s 'https://api.github.com/repos/$(1)/commits$(2)' | 2>/dev/null jq -r '.[0].sha' || echo 'HEAD')
endef

RUNR_COMMIT      ?= $(call GH_REPO_LAST_COMMIT,PRL-PRG/runr)
SXPDB_COMMIT     ?= $(call GH_REPO_LAST_COMMIT,PRL-PRG/sxpdb)
ARGTRACER_COMMIT ?= $(call GH_REPO_LAST_COMMIT,PRL-PRG/argtracer)

.PHONY: all
all: image

.PHONY: image
image: $(DEPENDENCIES_TXT)
	docker build \
		--build-arg CRAN_MIRROR=$(CRAN_MIRROR) \
		--build-arg DEPENDENCIES_TXT=$(DEPENDENCIES_TXT) \
		--build-arg RUNR_COMMIT=$(RUNR_COMMIT) \
		--build-arg SXPDB_COMMIT=$(SXPDB_COMMIT) \
		--build-arg ARGTRACER_COMMIT=$(ARGTRACER_COMMIT) \
    --rm \
    -t $(DOCKER_IMAGE_NAME) \
    .

.PHONY: upload
upload: image
	docker push $(DOCKER_IMAGE_NAME)
