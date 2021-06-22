#!/bin/bash

export PATH=$PATH:/home/Research/Downloads/SDRReradiation_AllPlatforms/NewSDRReradiation/NewSDRReradiation/bin/Debug_Linux
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/Research/Downloads/SDRReradiation_AllPlatforms/NewSDRReradiation/NewSDRReradiation/bin/Debug_Linux

./SDRReradiation -a -s 400000000 -e 500000000 -sr 2000000

