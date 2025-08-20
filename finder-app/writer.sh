#!/bin/bash

if [ $# -ne 2 ]; then 
   echo "Error: $0 requires two arguments!"
   echo "Usage: ./writer.sh <writefile> <writestr>"
   exit 1
fi

writefile=$1
writestr=$2

directory_path=$(dirname "$writefile")

if [ ! -d "$directory_path" ]; then
   mkdir -p "$directory_path"
fi

echo "$writestr" > "$writefile"

echo "File $writefile has been created successfully with the content: $writestr"
