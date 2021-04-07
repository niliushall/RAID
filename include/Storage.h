#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <windows.h>

#include <stdlib.h>

using namespace std;

#define OP_READ                  1
#define OP_WRITE                 2
#define OP_WRITE_CHECK_SUM      3
#define OP_READ_REBUILD         4

#define  SUCCESS 0
// #define  ERROR  -1



/*
 * 抽象的存储资源类，提供读写功能，具体过程由RAID和Disk类实现
 */

class Storage
{
    public:
        Storage();
        virtual ~Storage();
        virtual bool isvalidstate(int s);
        void setstate(int s);

        int getstate() {return state;}
        string getname(){return name;}
        string gettype() {return type;}

        virtual size_t getcapacity() {return capacity;} // 获取单个磁盘的容量,需要修改

        virtual size_t read(int offset, void *buf, size_t count);
        virtual size_t write(int offset, void *buf, size_t count);
        virtual int initialize() {return getstate();}
        virtual  int get_disk_size();
        virtual int set_rand_disk_hung();
        virtual int set_rand_disk_hung1();


        void printinfo(int tab=1);
        virtual void  pritinfo();

        static int getStoreageList(list<Storage *> nl);
        static int addStorage(Storage *s);
        static int printStorageList();
        static int removeStorage(Storage *s);

    protected:
        string type;
        int state;
        size_t capacity;	/*capacity of all disk(don't include chechsum space)*/
        string name;
        static int count;
        static list<Storage *> store;
};

typedef list<Storage *> STORAGELIST;

#endif // STORAGE_H
