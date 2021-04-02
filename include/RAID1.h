#ifndef RAID1_H
#define RAID1_H

#include "RAID.h"

class RAID1 : public RAID
{
    public:
        RAID1();
        virtual ~RAID1();
        virtual int initRAID();
        int getstate();
        virtual size_t read(int offset, void *buf, size_t count);
        virtual size_t write(int offset, void *buf, size_t count);

        int addressMapping(int offset, size_t count);

    protected:
        int slidesize;
        list<DISKADDR> diskaddrlist;
        list<DISKADDR> diskaddrlist_copy;

        Disk *getDisk(int diskno);

        size_t operate(int offset, void *buf, size_t count,int opflag);
        size_t operatedisk(int diskno,int offset, void *buf, size_t count,int opflag);



    private:
};


#endif // RAID1_H
