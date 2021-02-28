FROM gcc:10

# Default variables (see docker.env and README.md) - get latest mcrcon version
ENV MCRCON_BIN_URL=https://github.com/Tiiffi/mcrcon/releases/download/v0.7.1/mcrcon-0.7.1-linux-x86-64.tar.gz
ARG MCRCON_HOST=localhost
ARG MCRCON_PORT=25575
ARG MCRCON_PASS

RUN apt-get update && apt-get -y install wget tar make musl-dev && rm -rf /var/lib/apt/lists/*

ADD ./* /tmp/

WORKDIR /tmp
RUN make && make install && rm -rf /tmp/*

WORKDIR /
ENTRYPOINT ["mcrcon"]
