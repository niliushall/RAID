#ifndef RAID0_H
#define RAID0_H

#include "RAID.h"

class RAID0 : public RAID
{
    public:
        RAID0();
        virtual ~RAID0();
        virtual int initRAID();
        int getstate();
        virtual size_t read(int offset, void *buf, size_t count);
        virtual size_t write(int offset, void *buf, size_t count);

        int addressMapping(int offset, size_t count);

    protected:
        int slidesize;			 /*the size of block*/
        list<DISKADDR> diskaddrlist;

        Disk *getDisk(int diskno);

        size_t operate(int offset, void *buf, size_t count,int opflag);
        size_t operatedisk(int diskno,int offset, void *buf, size_t count,int opflag);
};

#endif // RAID0_H
