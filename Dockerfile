FROM alpine:latest

# <version> | 'latest'
RUN apk add --no-cache \
  make gcc libc-dev \
  && rm -rf /var/cache/apk/*

RUN mkdir -p /app/

COPY Makefile /app/
COPY mcrcon.1 /app/
COPY mcrcon.c /app/

WORKDIR /app/

RUN make
RUN make install

ENTRYPOINT [ "/app/mcrcon" ]