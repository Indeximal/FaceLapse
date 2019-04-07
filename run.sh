#!/bin/bash

set -e

date=${1:-$(date "+%Y%m%d")}

cp project.json backupdata/pre$date.json||echo "Couldn't backup project.json"

mkdir /tmp/facelapse
facelapse -d project.json -x -C transparent -o /tmp/facelapse $date/*

ffmpeg -framerate 15 -i '/tmp/facelapse/frame%05d.png' -pix_fmt yuv420p -vcodec libx264 parts/$date.mp4

mv facelapse.mp4 facelapseOld.mp4||echo "No old facelapse to backup"

echo "file '$date.mp4'" >> parts/list.txt
ffmpeg -f concat -i parts/list.txt -c copy facelapse.mp4

rm -rf /tmp/facelapse
echo "Done."