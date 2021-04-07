#include "Disk.h"

int Disk::count = 0;

Disk::Disk() {
    stringstream ss;
    type = "Disk";
    ss << type << Disk::count++;
    ss >> name;
    state = DISKSTATE_NOTREADY;
    capacity = 0;
}

Disk::~Disk() {

}

void Disk::setstate(int s) {
    if (!isvalidstate(s))
        return;

    if (DISKSTATE_FAILED == s) {
        if (!emptyDisk()) {
            cout << "Can not empty disk, set state failed!" << endl;
            return;
        }
    }
    state = s;
    return;
}

bool Disk::isvalidstate(int s) {
    if (s <= DISKSTATE_STARTFLAG || s >= DISKSTATE_ENDFLAG)
        return false;

    return true;
}

size_t Disk::getcapacity() {
    return disk_capacity;
}
