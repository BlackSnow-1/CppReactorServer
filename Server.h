//
// Created by w on 2025/1/2.
//

#ifndef C__REACTORSERVER_SERVER_H
#define C__REACTORSERVER_SERVER_H


int initListenFd(unsigned short port);


int epollRun(int lfd);

// 和客户端建立连接
int acceptClient(int lfd, int epfd);

int recvHttpRequest(int cfd, int epfd);


#endif //C__REACTORSERVER_SERVER_H
