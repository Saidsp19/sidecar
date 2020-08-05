#!/bin/bash

CMD="${@}"
cd /home/${DOCKER_USER}
exec gosu ${DOCKER_USER} /bin/bash -il ${CMD:+ -c ${CMD}}
