FROM prlprg/r-dyntrace:r-4.0.2

ENV R_PROJECT_BASE_DIR="/R" \
    TERM=xterm-256color

ARG CRAN_MIRROR=https://cloud.r-project.org
# what R packages should be installed
ARG DEPENDENCIES_TXT=dependencies.txt
# where should be the source of these packages
ARG PKG_SRC_DIR=/CRAN/extracted

ADD $DEPENDENCIES_TXT /tmp/dependencies.txt
RUN $R_DIR/bin/R -e 'install.packages(readLines("/tmp/dependencies.txt"), dependencies=TRUE, repos=Sys.getenv("CRAN_MIRROR"), INSTALL_opts=c("--example", "--install-tests", "--with-keep.source", "--no-multiarch"), Ncpus=parallel::detectCores())'

# install tracer
ARG RUNR_COMMIT=HEAD
RUN $R_DIR/bin/R -e "devtools::install_github('PRL-PRG/runr@$RUNR_COMMIT', upgrade=FALSE)"
ARG SXPDB_COMMIT=HEAD
RUN $R_DIR/bin/R -e "devtools::install_github('PRL-PRG/sxpdb@$SXPDB_COMMIT', upgrade=FALSE)"

# download package sources
RUN mkdir -p $PKG_SRC_DIR && \
  $R_DIR/bin/R -e "for (x in readLines('/tmp/dependencies.txt')) runr::download_cran_package_source(x, repos='$CRAN_MIRROR', dest_dir='$PKG_SRC_DIR')"

# r user and group - this is the user that will be used to run the experiments
RUN useradd -u 1000 -m -U -G sudo -s /bin/bash r && \
  echo "%sudo ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers && \
  echo "Defaults secure_path=\"$PATH\"" >> /etc/sudoers && \
  touch /home/r/.sudo_as_admin_successful && \
  mkdir -m 700 /home/r/.ssh && \
  chown -R r:r $R_DIR/library $PKG_SRC_DIR


ADD entrypoint.sh /
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
