#!/bin/bash

if $(cat /proc/cpuinfo | grep -i pku | grep -iq ospke); then
    echo "PKU suppoted"
else
    echo "PKU not suppoted"
fi
