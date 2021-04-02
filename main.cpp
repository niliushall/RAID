#include <iostream>
#include "Storage.h"
#include "RamDisk.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Client.h"
#include "RAID0.h"
#include "RAID1.h"
#include "RAID5.h"
#include "RAID6.h"
#include <windows.h>

using namespace std;

int checkmemory(void *buf1, void *buf2, size_t len, string s);

int checkOneRW(Client *c, int offset, void *buf, int len)
{
	void *buf2;
	c->write(offset, buf, len);

	buf2 = malloc(len);
	c->read(offset, buf2, len);


	if(memcmp(buf, buf2, len) == 0)
	{
		cout << "Exactly same!" << endl;
	}
	else
	{
		checkmemory(buf, buf2, 225, "Read/Write dismatch:");
	}

	free(buf2);

	return 0;
}

int randomhungdisk(Client *c,int hung_num)
{
    if(hung_num==1)
    {
        c->set_rand_disk_hung();
    }
    else if(hung_num==2)
    {
        c->set_rand_disk_hung1();
        c->set_rand_disk_hung();
    }
	return 0;
}

int randomCheck(Client *c, int checktime, int hung_disk_num)
{

	c->batchWrite(checktime);
    randomhungdisk(c,hung_disk_num);
	int ret = c->batchCheck(checktime);
	if(ret == checktime)
	{
	    cout<<endl;
	    cout<<"****************************"<<endl;
		cout<<"OK,all data is  correct!"<<endl;
		cout<<endl;
	}
	else
	{
        cout<<endl;
	    cout<<"**************************************************"<<endl;
	    cout<<"sorry,something is wrong!please check you program"<<endl;
	    cout<<endl;
	}
	return ret;
}


int main()
{
    cout << "hhhh!!!" << endl;



    /*int level = 0;
    RamDisk disk[10];
    disk[0].initDisk(10240);
    disk[1].initDisk(10240);
    disk[2].initDisk(10240);
    disk[3].initDisk(10240);
    RAID *r;
    //cout<<"please select the level of RAID(0, 1, 5 or 6):"<<endl;
    cout<<"the level of RAID is 6:"<<endl;
    //cin>>level;

    switch(level)
    {
    case 0:
        {r=(RAID0 *)new RAID0;break;}
    case 1:
        {r=(RAID1 *)new RAID1;break;}
    case 5:
        {r=(RAID5 *)new RAID5;break;}
    case 6:
        {r=(RAID6 *)new RAID6;break;}
    default:
        {cout<<"the level of RAID is not exist!"<<endl;
        return 0;}
    }

    r=(RAID6 *)new RAID6;
    (*r).addDisk(&disk[0]);
    (*r).addDisk(&disk[1]);
    (*r).addDisk(&disk[2]);
    (*r).addDisk(&disk[3]);
    (*r).initRAID();
    Client c(r);
    cout<<"****************check 1 start**************"<<endl;
    randomCheck(&c, 5, 0);
	Sleep(20000);
//    //若为RAID1,5,则容忍一个磁盘故障
//    randomCheck(&c, 5, 1);

//    //若为RAID6,则容忍两个磁盘故障
//    randomCheck(&c, 5, 2);

//    //若为RAID5,6,则增加一个磁盘
    disk[4].initDisk(10240);
    (*r).addDisk(&disk[4]);
    Client c1(r);
    cout<<"****************check  start**************"<<endl;
    randomCheck(&c, 5, 2);

    system("pause");
    return 0;*/
}