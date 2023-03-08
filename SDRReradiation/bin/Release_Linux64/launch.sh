#!/bin/bash

export PATH=$PATH:
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:

./SDRReradiation -a -s 400000000 -e 410000000 -sr 2000000

