#!/bin/sh
cd $1
svn info . | echo "`grep \"Last Changed Rev:\" | awk '{ print $4 }'`" 
