# Copyright (c) 2024 Stilsoft
# Distributed under the MIT License
# Author Vyhodcev Egor

#!/bin/bash

winpty docker run \
    --rm \
	-it \
	--name docker_test_paraos \
	--privileged \
	docker_test_paraos:1.0 
