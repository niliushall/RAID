#include "FileDisk.h"

int FileDisk::disk_count = 0;
const string FileDisk::basedir = "F:\\";

FileDisk::FileDisk() {
    space = nullptr;
    type = "FileDisk";
    name = basedir + "FileDisk_" + to_string(disk_count);
    disk_count++;
}

FileDisk::~FileDisk() {
    ifstream readfile(name);
    if(readfile.good()) {
        readfile.close();
        // remove(name.c_str());    // 析构函数中删除磁盘文件
    }
}

bool FileDisk::emptyDisk() {
    ofstream outf(name, ios::out | ios::trunc | ios::binary);
    if(outf.bad()) {
        return false;
    }

    if(outf.is_open()) {
        string buf(disk_capacity, 0);
        outf << buf;
        outf.close();
        cout << disk_capacity << endl;
    }

    return true;
}

int FileDisk::initDisk(size_t cap) {
    disk_capacity = cap;
    state = DISKSTATE_NOTREADY;
    if(cap > DISK_SIZE) {
        cout << "Too large ramdisk(limit:10240)!" << endl;
        return state;
    }

    ofstream file(name, ios::out | ios::binary | ios::trunc);
    if(file.bad()) {
        cout << "File open error : " << name << endl;
        return -1;
    }
    cout << "create file : " << name << endl;

    if(emptyDisk()) {
        state = DISKSTATE_READY;
    }
    return state;
}

size_t FileDisk::read(int offset, void *buf, size_t count) {
    ifstream file_reader(name, ios::in | ios::binary);
    if(file_reader.is_open()) {
        file_reader.seekg(offset, ios::beg);
        file_reader.read(static_cast<char *>(buf), count);
    }
    return count;
}

size_t FileDisk::write(int offset, void *buf, size_t count) {
    if(checkparam(offset, buf, count) == -1) {
        return -1;
    }

    ofstream file_writer(name, ios::out | ios::binary);
    if(file_writer.is_open()) {
        file_writer.seekp(offset, ios::beg);
        file_writer.write(static_cast<char *>(buf), count);
    }
    return count;
}

int FileDisk::checkparam(int offset, void *buf, size_t count) {
    if (state != DISKSTATE_READY) {
        cout << "Disk state is not ready(" << state << ")!" << endl;
        return -1;
    }

    if (offset < 0 || offset > disk_capacity) {
        cout << "Invalid offset(" << offset << ") and capacity(" << disk_capacity << ")!" << endl;
        return -1;
    }

    if (count < 0 || offset + count > disk_capacity) {
        cout << "Invalid offset(" << offset << ") and count(" << count << "}!" << endl;
        return -1;
    }

    if (buf == nullptr) {
        cout << "Invalid buffer(NULL)!" << endl;
        return -1;
    }

    return 0;
}


