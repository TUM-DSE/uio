#!/bin/bash

sudo python3.9 ./measure_apps.py
sudo python3.9 ./measure_console.py
sudo python3.9 ./measure_memory.py
sudo python3.9 ./measure_image.py
sudo python3.9 ./measure_symsize.py

pytho3.9 ./graphs.py ./measurements/app-latest.tsv
pytho3.9 ./graphs.py ./measurements/console-latest.tsv
pytho3.9 ./graphs.py ./measurements/memory-latest.tsv
pytho3.9 ./graphs.py ./measurements/image-latest.tsv

