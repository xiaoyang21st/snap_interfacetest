#!/bin/bash
export SNAP_TRACE=0xF
/afs/awd.austin.ibm.com/proj/p9/eclipz/c07/usr/xyangbj/c04/opencapi_1/snap/software/tools/snap_maint -vvv
/afs/awd.austin.ibm.com/proj/p9/eclipz/c07/usr/xyangbj/c04/opencapi_1/snap/actions/hls_interfacetest/sw/snap_interfacetest -t 2000
