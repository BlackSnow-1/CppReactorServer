//
// Created by w on 2025/1/2.
//

#include "Server.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>
#include <fcntl.h>
#include <cstring>

#define ARR_SIZE(arr) sizeof(arr)/sizeof(arr[0])


int initListenFd(unsigned short port) {

    //1、 创建监听的fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    if (lfd == -1l) {
        std::cerr << "Socket " << std::endl;
    }

    //2、 设置端口复用
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1) {
        std::cerr << "Set Sockopt" << std::endl;
    }

    //3、 绑定

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (struct sockaddr *) &addr, sizeof addr);

    if (ret == -1) {
        std::cerr << "bind" << std::endl;
        return -1;
    }

    //4、 设置监听
    ret = listen(lfd, 128l);

    if (ret == -1) {
        std::cerr << "listen" << std::endl;
        return -1;
    }
    return lfd;
}


int epollRun(int lfd) {

    // 1. 创建epoll 实例
    int epfd = epoll_create(1);

    if (epfd == -1) {
        std::cerr << "Epoll create" << std::endl;
        return -1;
    }

    //2. 上树
    struct epoll_event event{};
    event.data.fd = lfd;

    /* 监听的事件类型 */
    event.events = EPOLLIN;

    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &event);

    if (ret == -1) {
        std::cerr << "epoll ctl fail" << std::endl;
        return -1l;
    }

    //3. 检测
    struct epoll_event epollEvents[1024];

    while (true) {

        int num = epoll_wait(epfd, epollEvents, ARR_SIZE(epollEvents), -1l);

        for (int i = 0; i < num; i++) {
            int fd = epollEvents[i].data.fd;
            if (fd == lfd) {
                // 建立新连接
                acceptClient(lfd, epfd);
            } else {
                // 主要是接受对端数据

            }
        }

    }


    return 0l;
}


// 和客户端建立连接
int acceptClient(int lfd, int epfd) {

    // 1. 建立连接
    int cfd = accept(lfd, nullptr, nullptr);
    if (cfd == -1) {
        std::cerr << "Accept error" << std::endl;
        return -1;
    }

    //2. 设置非阻塞
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    //3. cfd 添加到epoll中
    struct epoll_event ev{};
    ev.data.fd = cfd;
    ev.events = EPOLLIN | EPOLLET;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1) {
        std::cerr << "epoll_ctl" << std::endl;
        return -1l;
    }

    return 0;
}

int recvHttpRequest(int cfd, int epfd) {

    int len{}, total{};
    char buf[4096] = {0};
    char tmp[1024] = {0};
    while ((len = recv(cfd, tmp, sizeof tmp, 0)) > 0) {

        if (total + len < ARR_SIZE(buf)) {
            memcpy(buf, tmp, len);
        }
        total += len;
    }

    return 0;
}