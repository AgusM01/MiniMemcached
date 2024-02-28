#!/bin/bash

N=0
while :; do
	K=$(((K+1) % 100))
	echo "PUT $K $N"
	N=$((N+1))
done | nc localhost 8888
