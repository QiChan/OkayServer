#!/bin/sh

bin=/usr/local/protobuf34/bin/protoc

#bin=echo

if [ -z "$1" ] ; then
	echo usage:
	echo ./start proto_dir
	exit
fi

INNER_IMPORT="./"
PUB_IMPORT="$1"
PROTO=`ls $INNER_IMPORT/*.proto $PUB_IMPORT/*.proto`


$bin --cpp_out=.. -I"$INNER_IMPORT" -I"$PUB_IMPORT" $PROTO


