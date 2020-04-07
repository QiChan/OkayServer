#!/bin/sh

bin=/usr/local/protobuf34/bin/protoc

#bin=echo

INNER_IMPORT="./"
PROTO=`ls $INNER_IMPORT/*.proto`

$bin --cpp_out=.. -I"$INNER_IMPORT" $PROTO
