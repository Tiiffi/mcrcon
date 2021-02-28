#!/bin/bash

docker build -t tiiffi/mcrcon:latest .
docker run -it --rm --env-file docker.env tiiffi/mcrcon:latest
