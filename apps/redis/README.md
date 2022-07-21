% bash run.sh
% redis-cli -h 172.44.0.2 -p 6379
172.44.0.2:6379> PING
PONG

### Entry point
- main.c in libs/redis creates a thread and then calls main() in https://github.com/redis/redis/blob/unstable/src/server.c

