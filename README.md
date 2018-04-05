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
After setting up UPS and then (if needed) installing the desire version of dunetpc
(typically the latest), dunetpc is set up with a command something like
```
> setup dunetpc v06_72_00 -q e15:prof
```
If this is successful, you will see the dunetpc environment variables defined:
```
> env | grep DUNETPC
DUNETPC_DIR=/home/dladams/ups/dunetpc/v06_72_00
SETUP_DUNETPC=dunetpc v06_72_00 -f Linux64bit+2.6-2.12 -z /home/dladams/ups -q e15:prof
DUNETPC_INC=/home/dladams/ups/dunetpc/v06_72_00/include
DUNETPC_VERSION=v06_72_00
DUNETPC_CXXFLAGS=-O3 -g -gdwarf-2 -fno-omit-frame-pointer -std=c++14
DUNETPC_CFLAGS=-O3 -g -gdwarf-2 -fno-omit-frame-pointer
DUNETPC_LIB=/home/dladams/ups/dunetpc/v06_72_00/slf6.x86_64.e14.prof/lib
DUNETPC_FQ_DIR=/home/dladams/ups/dunetpc/v06_72_00/slf6.x86_64.e14.prof
```
Install the dunefemb package in a dedicated directory, go there, and isntall the subpackage:
```
> git clone https://github.com/dladams/dunefemb.git
> cd dunefemb
> git clone https://github.com/dladams/dunesupport.git
```

Edit the top block of mysetup.sh to reflect your local installation. Or better, copy
it to config.dat and make the changes there.

The use the command
```
> source myshell
```
to start a shell with dunefemb set up, and then start root:
```
dunefemb> root.exe
```
This will be slow the first time as it compiles the local sources.

## Data
You must have data files to analyze. The dunefemb code expects these to be installed
at ~/data/dune/femb, e.g.
```
dunefemb> l ~/data/dune/femb/wib_sbnd_v109_femb_protodune_v308/20170822T125317
total 80
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_check_current_test
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s0_extpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s0_intpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s1_extpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s1_intpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s2_extpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s2_extpulse_intclock
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s2_intpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s3_extpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g2_s3_intpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g3_s0_extpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g3_s0_intpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g3_s1_extpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g3_s1_intpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g3_s2_extpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g3_s2_intpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g3_s3_extpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_gainenc_test_g3_s3_intpulse
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_powercycle_test
4 drwxrwxr-x 2 dladams dladams 4096 Aug 22  2017 fembTest_summary
```

The wib directory name does not matter but he next is the timestamp which does.

You must also have ~/data/dune/femb/fembjson.dat which maps timestamps to FEMBs. A copy
of this with entries for the first 25 FEMBs is in the git repository.

## Analyzing data
If all goes well and you have data installed appropriately, then 
```
dunefemb> root.exe fta.C
```
should display a plot.

