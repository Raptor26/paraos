#!/bin/bash

winpty docker run \
    --rm \
	-it \
	--name docker_test_paraos \
	--privileged \
	docker_test_paraos:1.0 
