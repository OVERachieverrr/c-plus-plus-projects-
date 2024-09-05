
# Chat System

This project implements a basic TCP server in C++. The server listens for incoming connections on a specified IP address and port, processes client requests, and main ojectiveis to exchange text and transfer images over the internet.

## NOTE

Here, we have used WXwidget to integrate graphics and used visual studio 2022 for our projects



## TEAM MEMBERS


- [NIRAJ BISTA](https://github.com/NirazB) -PUR080BEI025  -nirajbista25@gmail.com
- [LORESH KUNWAR](https://github.com/LoreshKunwar) -PUR080BEI022  -loresh.kunwar@gmail.com
- [SAWAR RAI](https://github.com/SawarRai) -PUR080BEI040  -raisawar2017@gmail.com
- [BIJAYA KHANAl](https://github.com/OVERachieverrr) -PUR080BEI015  -khanalbijaya21@gmail.com


 


## how to create projects 

1)open visual studio

2)create new projects

3)empty projects

4)In Source Files create file named "main.cpp"

### note:

similarly create projects for server and clients. You can create multiple clients projects or least two in visual studio. IF you checking using same laptop use localhost IP 127.0.0.1. If you are trying to connect two or more laptop use your IPv4 of network your are connected.Make sure all laptop are connected in same network and one more things, IP address of client should be modified.

## To setup WXwidget
follow the given links
```bash
https://www.youtube.com/watch?v=ONYW3hBbk-8&t=415s
```


### note:
you have to include headers files in properties as shown in given vedio link for every clients projects you have created.you also have setup wxwidget in LINUX.

## TO RUN IN LINUX
follow the given commands

for server
```bash
g++ server.cpp -o server -lpthread
```
```bash
./server
```

for clients

```bash
for client:  g++ wx-config --cxxflags client.cpp -o client `wx-config --libs
```
```bash
./client
```

## Screenshots

![App Screenshot](screenshots\windows\server.png)
![App Screenshot](screenshots\windows\Screenshot(88).png)
![App Screenshot](screenshots\windows\Screenshot(90).png)
![App Screenshot](screenshots\windows\Screenshot(89).png)
![App Screenshot](screenshots\windows\Screenshot(93).png)
![App Screenshot](screenshots\windows\Screenshot(94).png)
![App Screenshot](screenshots\windows\Screenshot(95).png)
![App Screenshot](screenshots\windows\Screenshot(96).png)
![App Screenshot](screenshots\linux\linux.jpg)
## Support

- if you have problem in setting up in windows contact @BIJAYA KHANAL.
- if you have problem in setting up in linux contact @NIRAJ BISTA
