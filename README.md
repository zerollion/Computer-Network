# Tcp-project Computer-Network

Collection of projects writteen for computer network class

A simple tcp server and client simulation

1. Assume radius of SelfAnnihilate is 0.5;
2. client socket number is their ID;
3. client will self elimiate every 100 time tick, and re-spawn as soon as they relize that they are not exist;
4. robot that no longer exist has coordinate -1.0;
5. server now can carry 5 client, setting by listen function, in line 395 of tcp-server.cpp;
6. According to Dr.GauthierDickey, our server do not need to support player leaves game, therefore player leaving game(player socket close) will cause server crash
7. clients will update ther position as soon as they recieve ServerMapUpdate message

extra
1. every player can only move at most 0.5 length in one step;
2. chain reaction by recursive call*

*chain reaction is added, compiled and pass the test on 2 client, 

usage:
server: ./tcp-server $port_number
client: ./tcp-client $server_ip $server_port
