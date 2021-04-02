#include "RAID0.h"

#include <math.h>

RAID0::RAID0():slidesize(256)
{
    //ctor
}

RAID0::~RAID0()
{
    //dtor
}

int RAID0::initRAID()
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

    capacity=diskcapacity*disklist.size();

    return 0;
}

int RAID0::getstate()
{
    state=RAIDSTATE_READY;
    list<Disk *>::iterator iter;
    for(iter = disklist.begin(); iter != disklist.end(); iter++)
    {
        if ((*iter)->getstate()!=DISKSTATE_READY)
        {
            state = RAIDSTATE_FAILED;
            cout<<"Disk("<<(*iter)->getname()<<") in RAID0("<<getname()<<") is not ready, please check again!"<<endl;
            break;
        }
    }
    return state;
}

int RAID0::addressMapping(int offset, size_t len)
{
    if (offset<0||(size_t)(offset+len)>capacity)
    {
        cout<<"Invalid parameters.offset("<<offset<<"),capacity("<<capacity<<")!"<<endl;
        return -1;
    }

    cout<<"AddrMapping: offset("<<offset<<"),len("<<len<<")"<<endl;

    diskaddrlist.clear();

    DISKADDR diskaddr;
    int disknum=disklist.size();

    int slideno=offset/slidesize; //the no of slide, start from 0
    int offset1=slideno/disknum*slidesize+offset%slidesize;  //在对应磁盘中的偏移

    diskaddr.diskno=slideno%disklist.size();  //数据所在的磁盘号
    diskaddr.offset=offset1;        //设置在所在磁盘偏移

    if (slidesize-offset%slidesize>=(int)len)//write to the last disk and only use the one disk
    {
        diskaddr.len=len;
        diskaddrlist.push_back(diskaddr);
    }
    else
    {
        int towrite=len;

        diskaddr.len=slidesize-offset%slidesize;//选中第一块磁盘长度
        towrite-=diskaddr.len;
        diskaddrlist.push_back(diskaddr); //添加第一块到diskaddrlist

        while(towrite>0)
        {
            diskaddr.diskno=(++slideno)%disknum;//写磁盘下一块
            diskaddr.offset=slideno/disknum*slidesize;//磁盘的偏移量
            if (towrite<slidesize)//写数据到写最后一块（有剩余）
                diskaddr.len=towrite;
            else
                diskaddr.len=slidesize;
            towrite-=diskaddr.len;
            diskaddrlist.push_back(diskaddr);  //添加数据磁盘信息到diskaddrlist
        }
    }
    list<DISKADDR>::iterator iter;

    for(iter=diskaddrlist.begin();iter!=diskaddrlist.end();iter++)
    {
        DISKADDR *ptr=&(*iter);
        cout<<"AddrMapping: diskno("<<ptr->diskno<<"),offset("<<ptr->offset<<"),len("<<ptr->len<<")."<<endl;
    }


    return diskaddrlist.size();
}

//get disk  which id  is diskno
Disk *RAID0::getDisk(int diskno)
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

size_t RAID0::operatedisk(int diskno,int offset, void *buf, size_t count,int opflag)
{
    switch (opflag)
    {
        case OP_READ:
            cout<<"Read from  disk("<<diskno<<") offset("<<offset<<"),count("<<count<<")!"<<endl;
        return getDisk(diskno)->read(offset,buf,count);
        case OP_WRITE:
            cout<<"Write to disk("<<diskno<<") offset("<<offset<<"),count("<<count<<")!"<<endl;
        return getDisk(diskno)->write(offset,buf,count);
        default:
            cout<<"Invalid opflag("<<opflag<<")"<<endl;
        return 0;
    }
}

size_t RAID0::operate(int offset, void *buf, size_t count,int opflag)
{
    int ret;

    if (getstate()==RAIDSTATE_FAILED)
    {
        cout<<"RAID state is failed, can not read or write!"<<endl;
        return -1;
    }

    ret=addressMapping(offset,count);//map data into all disks, all informatioin  is stored in diskaddrlist
    if (ret<0)
    {
        cout<<"Invalid offset and count"<<endl;
        return -1;
    }

    int finished=0;
    void *dataptr=buf;
    list<DISKADDR>::iterator iter;
    for(iter=diskaddrlist.begin();iter!=diskaddrlist.end();iter++)
    {
        DISKADDR *ptr=&(*iter);
        operatedisk(ptr->diskno,ptr->offset,dataptr,ptr->len,opflag);
        finished+=ptr->len;
        dataptr=(char *)dataptr+ptr->len;
    }
    return finished;
}

size_t RAID0::write(int offset, void *buf, size_t count)
{
    return operate(offset,buf,count,OP_WRITE);
}

size_t RAID0::read(int offset, void *buf, size_t count)
{
    return operate(offset,buf,count,OP_READ);
}
