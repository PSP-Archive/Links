#!/bin/sh
export FILENAME=links-2.1pre23_psp_r`./get_svn_version.sh ..`_$1

echo filename: $FILENAME

shift
echo command: zip -r /tmp/$FILENAME $*

zip -r /tmp/$FILENAME $*
mv /tmp/$FILENAME .
