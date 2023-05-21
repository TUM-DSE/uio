#!/bin/bash

START=$SECONDS

sudo python3.9 ./measure_apps.py
END_APP=$SECONDS
sudo python3.9 ./measure_console.py
END_CONSOLE=$SECONDS
sudo python3.9 ./measure_memory.py
END_MEMORY=$SECONDS
sudo python3.9 ./measure_image.py
END_IMAGE=$SECONDS
sudo python3.9 ./measure_symsize.py
END_SYMSIZE=$SECONDS

python3.9 ./graphs.py ./measurements/app-latest.tsv
python3.9 ./graphs.py ./measurements/console-latest.tsv
python3.9 ./graphs.py ./measurements/memory-latest.tsv
python3.9 ./graphs.py ./measurements/image-latest.tsv

END=$SECONDS

echo "Total time: $((END-START)) seconds"
echo "App time: $((END_APP-START)) seconds"
echo "Console time: $((END_CONSOLE-END_APP)) seconds"
echo "Memory time: $((END_MEMORY-END_CONSOLE)) seconds"
echo "Image time: $((END_IMAGE-END_MEMORY)) seconds"
echo "Symsize time: $((END_SYMSIZE-END_IMAGE)) seconds"

