#include "RAID1.h"

#include <math.h>

RAID1::RAID1():slidesize(256)
{
    //ctor
}


RAID1::~RAID1()
{
    //dtor
}

int RAID1::initRAID()
{
    state=RAIDSTATE_NOTREADY;

    if (disklist.size()<2)
    {
        cout<<"No enough disks("<<disklist.size()<<")"<<endl;
        return state;
    }

    list<Disk *>::iterator iter;
    size_t diskcapacity=(*disklist.begin())->getcapacity();
    for(iter = disklist.begin(); iter != disklist.end(); iter++)
    {
        if (diskcapacity!=(*iter)->getcapacity())
        {
            cout<<"Disk("<<(*iter)->getname()<<") size is not same with disk 0, please check again!"<<endl;
            return state;
        }
        if ((*iter)->getstate()!=DISKSTATE_READY)
        {
            cout<<"Disk("<<(*iter)->getname()<<") not ready, please check again!"<<endl;
            return state;
        }
    }

    capacity=diskcapacity*disklist.size()/2;

   return 0;
}

int RAID1::getstate()
{
    state=RAIDSTATE_READY;
    list<Disk *>::iterator iter;
    for(iter = disklist.begin(); iter != disklist.end(); iter++)
    {
        if ((*iter)->getstate()!=DISKSTATE_READY)
        {
            state = RAIDSTATE_FAILED;
            cout<<"Disk("<<(*iter)->getname()<<") in RAID1("<<getname()<<") is not ready, please check again!"<<endl;
            break;
        }
    }

    return state;
}

int RAID1::addressMapping(int offset, size_t len)
{
    if (offset<0||(size_t)(offset+len)>capacity)
    {
        cout<<"Invalid parameters.offset("<<offset<<"),capacity("<<capacity<<")!"<<endl;
        return -1;
    }

    cout<<"AddrMapping: offset("<<offset<<"),len("<<len<<")"<<endl;

    diskaddrlist.clear();
    diskaddrlist_copy.clear();

    DISKADDR diskaddr;
    DISKADDR diskaddr_copy;

    int disknum=disklist.size()/2;

    int slideno=offset/slidesize; //the no of slide, start from 0 层
    int offset1=slideno/disknum*slidesize+offset%slidesize;

    diskaddr.diskno=(slideno%disknum)*2;
    diskaddr.offset=offset1;

    diskaddr_copy.diskno=diskaddr.diskno+1;
    diskaddr_copy.offset=offset1;

    if (slidesize-offset%slidesize>=(int)len)//write to the last disk and only use the one disk
    {
        diskaddr.len=len;
        diskaddrlist.push_back(diskaddr);
        diskaddr_copy.len=len;
        diskaddrlist.push_back(diskaddr_copy);
    }
    else
    {
        int towrite=len;

        diskaddr.len=slidesize-offset%slidesize;
        diskaddr_copy.len=diskaddr.len;
        towrite-=diskaddr.len;
        diskaddrlist.push_back(diskaddr);
        diskaddrlist_copy.push_back(diskaddr_copy);


        while(towrite>0)
        {
            diskaddr.diskno=((++slideno)%disknum)*2;//写磁盘下一块
            diskaddr_copy.diskno=diskaddr.diskno+1;
            diskaddr.offset=slideno/disknum*slidesize;//磁盘的偏移量
            diskaddr_copy.offset=diskaddr.offset;
            if (towrite<slidesize)//写数据到写最后一块（有剩余）
            {
               diskaddr.len=towrite;
               diskaddr_copy.len=towrite;
            }

            else
            {
                diskaddr.len=slidesize;
                diskaddr_copy.len=slidesize;
            }
            towrite-=diskaddr.len;
            diskaddrlist.push_back(diskaddr);
            diskaddrlist_copy.push_back(diskaddr_copy);
        }
    }
    list<DISKADDR>::iterator iter;
    list<DISKADDR>::iterator iter_copy;

    for(iter=diskaddrlist.begin(),iter_copy=diskaddrlist_copy.begin();iter!=diskaddrlist.end(),iter_copy!=diskaddrlist_copy.end();iter++,iter_copy++)
    {
        DISKADDR *ptr=&(*iter);
        DISKADDR *ptr_copy=&(*iter_copy);
        cout<<"AddrMapping: diskno("<<ptr->diskno<<"),offset("<<ptr->offset<<"),len("<<ptr->len<<")."<<endl;
        cout<<"AddrMapping: diskno("<<ptr_copy->diskno<<"),offset("<<ptr_copy->offset<<"),len("<<ptr_copy->len<<")."<<endl;
    }


    return diskaddrlist.size(); //写了多少块数据，不包含备份数据
}

Disk *RAID1::getDisk(int diskno)
{
    if (diskno>(int)disklist.size())
    {
        return NULL;
    }

    list<Disk *>::iterator iter=disklist.begin();
    for(int i=0;i<diskno;i++)
        iter++;

    return *iter;
}

size_t RAID1::operatedisk(int diskno,int offset, void *buf, size_t count,int opflag)
{
    switch (opflag)
    {
    case OP_READ:
        cout<<"Read from  disk("<<diskno<<") offset("<<offset<<"),count("<<count<<")!"<<endl;
        return getDisk(diskno)->read(offset,buf,count); //此时的read和write是指磁盘操作。
    case OP_WRITE:
        cout<<"Write to disk("<<diskno<<") offset("<<offset<<"),count("<<count<<")!"<<endl;
        return getDisk(diskno)->write(offset,buf,count);
    default:
        cout<<"Invalid opflag("<<opflag<<")"<<endl;
        return 0;
    }
}

size_t RAID1::operate(int offset, void *buf, size_t count,int opflag)
{
    int ret;

    if (getstate()==RAIDSTATE_FAILED)
    {
        cout<<"RAID state is failed, can not read or write!"<<endl;
        return -1;
    }

    ret=addressMapping(offset,count);
    if (ret<0)
    {
        cout<<"Invalid offset and count"<<endl;
        return -1;
    }

    int finished=0;
    void *dataptr=buf;
    list<DISKADDR>::iterator iter;
    list<DISKADDR>::iterator iter_copy;
    for(iter=diskaddrlist.begin(),iter_copy=diskaddrlist_copy.begin();iter!=diskaddrlist.end(),iter_copy!=diskaddrlist_copy.end();iter++,iter_copy++)
    {
        DISKADDR *ptr=&(*iter);
        DISKADDR *ptr_copy=&(*iter_copy);
        operatedisk(ptr->diskno,ptr->offset,dataptr,ptr->len,opflag);
        operatedisk(ptr_copy->diskno,ptr_copy->offset,dataptr,ptr_copy->len,opflag);
        finished+=ptr->len;
        dataptr=(char *)dataptr+ptr->len;
    }

    return finished;
}

size_t RAID1::write(int offset, void *buf, size_t count)
{
    return operate(offset,buf,count,OP_WRITE);
}

size_t RAID1::read(int offset, void *buf, size_t count)
{
    return operate(offset,buf,count,OP_READ);
}
//read/write(RAID)->operate->operatedisk->read/write(disk)     operate->addressmapping
