FROM alpine:3

# Default variables (see docker.env and README.md) - get latest mcrcon version
ARG MCRCON_HOST=localhost
ARG MCRCON_PORT=25575
ARG MCRCON_PASS

RUN apk --update-cache --no-cache add build-base musl-dev

ADD ./* /tmp/

WORKDIR /tmp
RUN make && make install && rm -rf /tmp/*

WORKDIR /
ENTRYPOINT ["mcrcon"]
