//
// Created by w on 2025/1/2.
//

#ifndef C__REACTORSERVER_SERVER_H
#define C__REACTORSERVER_SERVER_H


int initListenFd(unsigned short port);


int epollRun(int lfd);

// 和客户端建立连接
int acceptClient(int lfd, int epfd);

// 接受Http请求
int recvHttpRequest(int cfd, int epfd);

// 解析请求行
int parseRequestLine(const char *line, int cfd);

// 发送文件
int sendFile(const char *fileName, int cfd);

// 发送响应头（状态行 + 响应头）
int sendHeadMsg(int cfd, int status, const char *descr, const char *type, int length);

// 获取文件类型
const char *getFileType(const char *name);

// 发送目录
int sendDir(const char *dirName, int cfd);

#endif //C__REACTORSERVER_SERVER_H
