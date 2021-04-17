#include "RAID.h"
#include<time.h>

int RAID::count = 0;

RAID::RAID() {
    //ctor
    stringstream ss;
    type = "RAID";
    ss << type << RAID::count++;
    ss >> name;
}

RAID::~RAID() {
    //dtor
}

bool RAID::isvalidstate(int s) {
    if (s <= STARTFLAG || s >= ENDFLAG)
        return false;
    return true;
}

int RAID::getstate() {
    //should check disk states and calculate the state of the RAID.


    return state;

}

int RAID::addDisk(Disk *d) {
    if (d == nullptr)
        return 0;

    if (d->getstate() != DISKSTATE_READY) {
        cout << "Disk(" << d->getname() << ") is not ready,state(" << d->getstate() << ")!" << endl;
        return 0;
    }
    disklist.push_back(d);
    return 1;
}

int RAID::removeDisk(Disk *d) {
    list<Disk *>::iterator it;
    for (it = disklist.begin(); it != disklist.end(); it++) {
        if (d->getname() == (*it)->getname()) {
            it = disklist.erase(it);
            return 1;
        }
    }

    return 0;
}

int RAID::getPosition(Disk *disk) {
    int pos = -1;
    list<Disk *>::iterator it;
    for (it = disklist.begin(); it != disklist.end(); it++) {
        pos++;
        if (disk->getname() == (*it)->getname())
            return pos;
    }
    return -1;
}

Disk *RAID::getDisk(unsigned int pos) {
    if (pos > disklist.size())
        return NULL;

    unsigned int count = 0;

    list<Disk *>::iterator it;
    for (it = disklist.begin(); it != disklist.end(); it++) {
        if (count == pos)
            return *it;
        count++;
    }
    return NULL;
}

int RAID::replaceDisk(unsigned int pos, Disk *newdisk) {
    Disk *d = getDisk(pos);

    if (NULL != d)
        return replaceDisk(d, newdisk);
    return 0;

}

int RAID::replaceDisk(Disk *olddisk, Disk *newdisk) {
    list<Disk *>::iterator it;
    for (it = disklist.begin(); it != disklist.end(); it++) {
        if (olddisk->getname() == (*it)->getname()) {
            it = disklist.erase(it);
            disklist.insert(it, newdisk);
            return 1;
        }
    }

    return 0;
}

int RAID::printDiskList() {
    this->printinfo();
    list<Disk *>::iterator it;
    for (it = disklist.begin(); it != disklist.end(); it++) {
        (*it)->printinfo(5);
    }
    return 0;
}


//int RAID::initRAID()
//{
//  return 0;
//}


int RAID::rebuildRAID() {
    return 0;
}

int RAID::set_rand_disk_hung1() {
    int ret = SUCCESS;
    list<Disk *>::iterator it;
    int i;
    for (it = disklist.begin(), i = 0; i < disklist.size() - 2; it++, i++) {
    }

    cout << "hung disk : " << i << endl;
    (*it)->setstate(DISKSTATE_HUNG);
    return ret;
}

int RAID::set_rand_disk_hung() {
    int ret = SUCCESS;
    srand(time(NULL));
    int diskno = -1;
    list<Disk *>::iterator it;
    int count = 0;
    int time = 5;
    while (time-- > 0) {
        count = 0;
        diskno = rand() % (int) disklist.size();
        for (it = disklist.begin(); it != disklist.end(); it++) {
            if (count == diskno && (*it)->getstate() == DISKSTATE_READY) //确保被hung 的是正常的disk
            {
                cout << "hung disk : " << diskno << endl;
                (*it)->setstate(DISKSTATE_HUNG);
                break;
            }
            count++;
        }
        if (it != disklist.end()) //如果没有hung掉disk,则重复
        {
            break;
        }
    }
    if (time == 0) {
        ret = ERR;
    }
    return ret;
}

int RAID::get_disk_size() {
    return disklist.size();
}

void RAID::pritinfo() {
    printDiskList();
}

int RAID::check_raid_state() {
    return 0;
}