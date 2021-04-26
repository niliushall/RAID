#ifndef RAID_FILEDISK_H
#define RAID_FILEDISK_H

#include "Disk.h"
#include <fstream>

class FileDisk : public Disk {
public:
    FileDisk();
    ~FileDisk() override;
    bool emptyDisk() override;
    int initDisk(size_t capacity = DISK_SIZE) override;
    size_t read(int offset, void* buf, size_t count) override;
    size_t write(int offset, void* buf, size_t count) override;

private:
    static const string basedir;
    static int disk_count;
    void* space;        // initial address of disk

private:
    int checkparam(int offset, void* buf, size_t count);
};


#endif //RAID_FILEDISK_H
