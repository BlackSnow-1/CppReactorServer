//
// Created by w on 2025/1/1.
//

#ifndef C__REACTORSERVER_CLIENT_H
#define C__REACTORSERVER_CLIENT_H


typedef int(*handleFunc) (void* arg);

enum FDEvent{
    Timeout = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};


#endif //C__REACTORSERVER_CLIENT_H
