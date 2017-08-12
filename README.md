# dunefemb
Code to analyze data fromDUNE front end motherboards.

## Introduction
This repository holds code used to analyze the data produced in
the BNL DUNE tests of the LAr TPC motherboards to be used in the
protoDUNE detector.

## Setup
The code here makes use of tools and other code in dunetpc, the 
repository that holds the DUNE TPC simulation and reconstruction code.
For instructions on obtaining that code, see
https://dune.bnl.gov/wiki/DUNE_LAr_Software_Releases.
After seting up UPS and then (if needed) installing the desire version of dunetpc
(typically the latest), dunetpc is set up with a command something like
```
> setup dunetpc v06_46_00 -q e14:prof
```
If this is successful, you will see the dunetpc environment variables defined:
```
> env | grep DUNETPC
DUNETPC_DIR=/home/dladams/ups/dunetpc/v06_46_00
SETUP_DUNETPC=dunetpc v06_46_00 -f Linux64bit+2.6-2.12 -z /home/dladams/ups -q e14:prof
DUNETPC_INC=/home/dladams/ups/dunetpc/v06_46_00/include
DUNETPC_VERSION=v06_46_00
DUNETPC_CXXFLAGS=-O3 -g -gdwarf-2 -fno-omit-frame-pointer -std=c++14
DUNETPC_CFLAGS=-O3 -g -gdwarf-2 -fno-omit-frame-pointer
DUNETPC_LIB=/home/dladams/ups/dunetpc/v06_46_00/slf6.x86_64.e14.prof/lib
DUNETPC_FQ_DIR=/home/dladams/ups/dunetpc/v06_46_00/slf6.x86_64.e14.prof
```
