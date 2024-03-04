!/bin/bash

while :; do
    ./guido PUT agustin < yo.txt
    ./guido GET agustin
    ./guido PUT lucho < lucho.txt
    ./guido GET lucho
done
