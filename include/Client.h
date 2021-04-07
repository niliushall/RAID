#ifndef CLIENT_H
#define CLIENT_H

#include "Storage.h"

/*
 * 模拟应用程序的读写，通过随机写入数据并读出检验数据是否丢失
 */

class Client {
public:
    Client(Storage *s);

    virtual ~Client();

    int batchCheck(int time);

    int batchWrite(int time);

    int read(int offset, void *buf, size_t count);

    int write(int offset, void *buf, size_t count);

    int get_disk_num();

    void set_rand_disk_hung();

    void set_rand_disk_hung1();

    void prin_disk_info();


protected:
    Storage *target;  /*storage of raid*/
    int seed;

    int randomCheck(int no, int total, size_t capacity);

    int randomWrite(int no, int total, size_t capacity);


private:
};

#endif // CLIENT_H
