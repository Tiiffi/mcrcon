FROM gcc:10

# Default variables (see docker.env and README.md) - get latest mcrcon version
ARG MCRCON_HOST=localhost
ARG MCRCON_PORT=25575
ARG MCRCON_PASS

RUN apt-get update && apt-get -y install wget tar make musl-dev && rm -rf /var/lib/apt/lists/*

ADD ./* /tmp/

WORKDIR /tmp
RUN make && make install && rm -rf /tmp/*

WORKDIR /
ENTRYPOINT ["mcrcon"]
