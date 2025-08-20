#!/bin/sh

if [ $# -ne 2 ]; then 
   echo "Error: Only two arguments are accepted"
   echo "Usage: $0 <filesdir> <searchstr>"
   exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d "$filesdir" ]; then
   echo "Error: $filesdir does not represent a directory on the filesystem"
   exit 1
fi

file_count=$(find "$filesdir" -type f | wc -l)

matching_lines=$(find "$filesdir" -type f -exec grep -l "$searchstr" {} \; 2>/dev/null | xargs grep "$searchstr" 2>/dev/null | wc -l)

echo "The number of files are $file_count and the number of matching lines are $matching_lines" 
