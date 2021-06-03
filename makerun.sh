#!/bin/sh

CURRENTPATH=`pwd`
CURRENTNAME=`basename $CURRENTPATH`

if [ $# -ne 1 ]
then
   echo "Must provide 3ds ip address!"
   exit 1
fi

make && $DEVKITPRO/tools/bin/3dslink -a $1 $CURRENTNAME.3dsx

