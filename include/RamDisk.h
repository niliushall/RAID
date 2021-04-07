#ifndef RAMDISK_H
#define RAMDISK_H

#include "Disk.h"

class RamDisk : public Disk {
public:
    RamDisk();
    ~RamDisk() override;
    int initDisk(size_t capacity = DISK_SIZE) override;
    size_t read(int offset, void *buf, size_t count) override;
    size_t write(int offset, void *buf, size_t count) override;
    bool emptyDisk() override;

    void *space;  //the address of  disk

protected:
    static int count;
    int checkparam(int offset, void *buf, size_t count);
};

#endif // RAMDISK_H
