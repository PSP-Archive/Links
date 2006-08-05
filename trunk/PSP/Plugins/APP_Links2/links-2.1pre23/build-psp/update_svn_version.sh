#!/bin/sh
cd $1
#pwd
svn update . > /dev/null && svn info . | echo "#define SVN_VERSION \"`grep \"Last Changed Rev:\" | awk '{ print $4 }'`\"" > svn_version.h
