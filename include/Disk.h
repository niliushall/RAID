#ifndef DISK_H
#define DISK_H

#include "Storage.h"
#include "RAID.h"

#define DISK_SIZE 10240

#define DISKSTATE_STARTFLAG       0    // had not alloc the memory
#define DISKSTATE_NOTREADY        1    // had alloc the memory
#define DISKSTATE_READY           2
#define DISKSTATE_FAILED          3
#define DISKSTATE_HUNG            4
#define DISKSTATE_ENDFLAG         5


class RAID;

class Disk : public Storage {
public:
    Disk();
    virtual int initDisk(size_t capacity) {return DISKSTATE_NOTREADY;}
    virtual ~Disk();
    virtual bool isvalidstate(int s);
    virtual bool emptyDisk(){return false;}
    size_t getcapacity();
    void setstate(int s);

protected:
    size_t disk_capacity;   // capacity of single disk
    RAID *group;

    static int count;
};

#endif // DISK_H
