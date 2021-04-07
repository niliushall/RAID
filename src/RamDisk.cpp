#include "RamDisk.h"
#include <cstdlib>
#include <string>

int RamDisk::count = 0;

RamDisk::RamDisk() : space(NULL) {
    type = "RamDisk";
    name = "RamDisk" + to_string(count);
    count++;
}

RamDisk::~RamDisk() {
    if (space) {
        free(space);
    }
}

bool RamDisk::emptyDisk() {
    memset(space, 0, capacity);
    return true;
}

int RamDisk::initDisk(size_t cap) {
    capacity = cap;
    state = DISKSTATE_NOTREADY;
    if (cap > 10240) {
        cout << "Too large ramdisk(limit:10240)!" << endl;
        return state;
    }
    space = malloc(cap);    // 分配内存（磁盘）空间
    if (NULL == space) {
        cout << "Malloc failed, create Ramdisk failed!" << endl;
        return state;
    }

    state = DISKSTATE_READY;
    return state;
}

int RamDisk::checkparam(int offset, void *buf, size_t count) {
    if (DISKSTATE_READY != state) {
        cout << "Disk state is not ready(" << state << ")!" << endl;
        return -1;
    }

    if (offset < 0 || offset > capacity) {
        cout << "Invalid offset(" << offset << ") and capacity(" << capacity << ")!" << endl;
        return -1;
    }

    if (count < 0 || offset + count > capacity) {
        cout << "Invalid offset(" << offset << ") and count(" << count << "}!" << endl;
        return -1;
    }

    if (NULL == buf) {
        cout << "Invalid buffer(NULL)!" << endl;
        return -1;
    }

    return 0;

}

size_t RamDisk::read(int offset, void *buf, size_t count) {
    if (checkparam(offset, buf, count) == -1) {
        return -1;
    } else {
        void *pos = (char *) space + offset;
        memcpy(buf, pos, count);
        return count;
    }
}

/*将buf一部分内容写到 disk 中*/
size_t RamDisk::write(int offset, void *buf, size_t count) {
    if (checkparam(offset, buf, count) == -1)
        return -1;

    void *pos = (char *) space + offset;

    memmove(pos, buf, count);
    return count;

}