#!/bin/bash

CPU3=3
CPU4=4

HOST="172.44.0.2"
PORT=6379

[[ -z "${REPS}" ]] && REPS=10000
#[[ -z "${CONCURRENT_CONNS}" ]] && CONCURRENT_CONNS=10
[[ -z "${CONCURRENT_CONNS}" ]] && CONCURRENT_CONNS=30
#[[ -z "${PAYLOAD_SIZE}" ]] && PAYLOAD_SIZE=2
[[ -z "${PAYLOAD_SIZE}" ]] && PAYLOAD_SIZE=3
[[ -z "${KEEPALIVE}" ]] && KEEPALIVE=1
#[[ -z "${PIPELINING}" ]] && PIPELINING=1
[[ -z "${PIPELINING}" ]] && PIPELINING=16
[[ -z "${QUERIES}" ]] && QUERIES=get,set

taskset -c ${CPU3},${CPU4} redis-benchmark --csv -q \
    -n ${REPS} -c ${CONCURRENT_CONNS} \
    -h ${HOST} -p ${PORT} -d ${PAYLOAD_SIZE} \
    -k ${KEEPALIVE} -t ${QUERIES} \
    -P ${PIPELINING}
