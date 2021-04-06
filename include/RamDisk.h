#ifndef RAMDISK_H
#define RAMDISK_H

#include "Disk.h"


class RamDisk : public Disk
{
    public:
        RamDisk();
        virtual ~RamDisk();
        virtual int initDisk(int capacity = DISK_SIZE);
        virtual size_t read(int offset, void *buf, size_t count);
        virtual size_t write(int offset, void *buf, size_t count);
        virtual bool emptyDisk();

        void *space;  //the address of  disk

    protected:
        static int count;

        int checkparam(int offset, void *buf, size_t count);
    private:
};

#endif // RAMDISK_H
