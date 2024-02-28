!/bin/bash

while :; do
    ./guido PUT agustin < yo.txt
    ./guido GET agustin
    ./guido PUT agustin < lucho.txt
    ./guido GET agustin
done
