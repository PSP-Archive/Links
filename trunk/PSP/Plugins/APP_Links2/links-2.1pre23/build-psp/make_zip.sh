#!/bin/sh
export FILENAME=links-2.1pre22_psp_r`cat ../svn_version.h|awk -F'"' '{print $2}'`_$1

echo filename: $FILENAME

shift
echo command: zip -r /tmp/$FILENAME $*

zip -r /tmp/$FILENAME $*
mv /tmp/$FILENAME .
