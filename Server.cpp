//
// Created by w on 2025/1/2.
//

#include "Server.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <csignal>
#include <cassert>
#include <sys/sendfile.h>
#include <dirent.h>

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
                recvHttpRequest(fd, epfd);
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
    size_t len{}, total{};
    char buf[4096] = {0};
    char tmp[1024] = {0};
    while ((len = recv(cfd, tmp, sizeof tmp, 0)) > 0) {
        if (total + len < ARR_SIZE(buf)) {
            memcpy(buf + total, tmp, len);
        }
        total += len;
    }

    // 判断数据是否被接受完毕
    if (len == -1 && errno == EAGAIN) {
        //解析请求行
    } else if (len == 0) {
        // 客户端断开了连接
        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, nullptr);
        close(cfd);
    } else {
        std::cerr << "Error" << std::endl;
    }

    return 0;
}

int parseRequestLine(const char *line, int cfd) {
    // 解析请求行 get /xxx/1.jpg http/1.1
    char method[12];
    char path[1024];

    sscanf(line, "%[^ ] %[^ ]", method, path);

    if (strcasecmp(method, "get") != 0) {
        return -1;
    }

    // 处理客户端请求的静态资源(目录或文件)
    char *file{};
    if (strcmp(path, "/") == 0) {
        file = "./";
    } else {
        file = path + 1l;
    }

    // 获取文件属性
    struct stat st{};
    int ret = stat(file, &st);
    if (ret == -1) {
        // 文件不存在 -- 回复404
        sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        sendFile("404.html", cfd);
        return 0l;
    }

    // 判断文件类型
    if (S_ISDIR(st.st_mode)) {
        // 把这个目录中的内容发送给客户端
        sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        sendDir(file, cfd);
    } else {
        // 把文件的内容发送给客户端
        sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
        sendFile("404.html", cfd);
    }

    return 0;
}

// 发送文件
int sendFile(const char *fileName, int cfd) {
    //都一部分发一部分


    //1. 打开文件
    int fd = open(fileName, O_RDONLY);
    assert(fd > 0);
#if 0
    while (true) {
        char buf[1024];
        auto len = read(fd, buf, sizeof buf);
        if (len > 0) {
            send(cfd, buf, len, 0);
            usleep(10); // 这非常重要
        } else if (len == 0) {
            break;
        } else {
            std::cerr << "read " << std::endl;
        }
    }
#else

    auto size = lseek(fd, 0, SEEK_END);
    sendfile(cfd, fd, nullptr, size);
#endif

    return 0;
}

int sendHeadMsg(int cfd, int status, const char *descr, const char *type, int length) {
    // 状态行
    char buf[4096] = {0};

    sprintf(buf, "http/1.1 %d %s\r\n", status, descr);

    // 响应头
    sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
    sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);

    // 空行

    send(cfd, buf, strlen(buf), 0);
    return 0;
}

const char *getFileType(const char *name) {
    // a.jpg a.mp4 a.html
    // 自右向左查找 '.' 字符，如不存在返回null

    const char *dot = strrchr(name, '.');

    if (dot == nullptr) {
        return "text/plain; charset=utf-8"; //纯文本
    }

    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
        return "text/html; charset=utf-8";
    }

    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) {
        return "image/jpeg";
    }

    if (strcmp(dot, "gif") == 0) {
        return "image/gif";
    }

    if (strcmp(dot, ".mp4") == 0) {
        return "video/mp4";
    }

    if (strcmp(dot, ".png") == 0) {
        return "image/png";
    }

    if (strcmp(dot, ".css") == 0) {
        return "text/css";
    }

    if (strcmp(dot, ".au") == 0) {
        return "audio/basic";
    }

    if (strcmp(dot, ".wav") == 0) {
        return "audio/wav";
    }

    if (strcmp(dot, ".avi") == 0) {
        return "video/x-msvideo";
    }

    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0) {
        return "video/quicktime";
    }

    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0) {
        return "video/mpeg";
    }

    if (strcmp(dot, ".ogg") == 0) {
        return "application/ogg";
    }

    if (strcmp(dot, ".pac") == 0) {
        return "application/x-ns-proxy-autoconfig";
    }

    return "text/plain; charset=utf-8";
}


int sendDir(const char *dirName, int cfd) {
    char buf[4096] = {};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName, getFileType("."));

    struct dirent **namelist;

    int num = scandir(dirName, &namelist, nullptr, alphasort);

    for (int i = 0; i < num; i++) {
        // 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
        char *name = namelist[i]->d_name;
        struct stat st{};

        char subPath[1024] = {};
        sprintf(subPath, "%s/%s", dirName, name);

        stat(name, &st);

        if (S_ISDIR(st.st_mode)) {
            // a标签 <a href=""> name </a>

            // href 后有斜线访问目录 没有访问文件
            sprintf(buf + strlen(buf),
                    "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        } else {
            sprintf(buf + strlen(buf),
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }

        send(cfd, buf, strlen(buf), 0);
        memset(buf, 0, sizeof buf);
        free(namelist[i]);
    }

    // 闭合外标签
    sprintf(buf, "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);

    free(namelist);

    return 0;
}
