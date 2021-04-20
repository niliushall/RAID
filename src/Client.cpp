#include "Client.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "RamDisk.h"
#include <stdio.h>

Client::Client(Storage *s) : target(s) {
    srand(time(NULL));
    seed = rand();
}

Client::~Client() {

}

int checkmemory(void *buf1, void *buf2, size_t len, string s) {
    cout << s << endl;
    string ret;
    char *ptr1 = (char *) buf1, *ptr2 = (char *) buf2;
    for (size_t i = 0; i < len; i++) {
        ret = (ptr1[i] == ptr2[i]) ? "" : " dismatch";
        printf("%d   v1:%3x, v2:%3x  %s\n", i, (uint8_t) ptr1[i], (uint8_t) ptr2[i], ret.c_str());
    }
    return 0;
}

int Client::read(int offset, void *buf, size_t len) {
    int ret = target->read(offset, buf, (size_t) len);
    if (-1 == ret) {
        cout << "Read from " << target->getname() << " failed.offset(" << offset << "),count(" << len << ")!" << endl;
    }

    return ret;
}

int Client::write(int offset, void *buf, size_t len) {
    int ret = target->write(offset, buf, (size_t) len);
    cout << "ret is " << ret << "\n\n";

    if (-1 == ret) {
        cout << "Write to " << target->getname() << " failed.offset(" << offset << "),len(" << len << ")!" << endl;

    }
    return ret;
}

int Client::randomWrite(int no, int total, size_t capacity) {
    int ret = 0;
    int step = capacity / total;

    int offset = rand() % step;
    int len = rand() % (step - offset);
    offset = offset + step * no;


    if (len == 0) return 0;
    void *buf = malloc(len);
    cout << no << " Write to " << target->getname() << ", offset(" << offset << ") size(" << len << ")!" << endl;

    memset(buf, 0, len);

    char *ptr = (char *) buf;
    char value;
    for (int i = 0; i < len; i++) {
        value = rand() % 128;
        ptr[i] = value;
    }

    ret = write(offset, buf, len);
    free(buf);
    return ret;
}

#define BUFSIZE 256

int Client::randomCheck(int no, int total, size_t capacity) {
    int ret;
    int step = capacity / total;

    char buffer1[BUFSIZE] = {0}, buffer2[BUFSIZE] = {0};

    int offset = rand() % step;
    int len = rand() % (step - offset);
    offset = offset + step * no;

    if (len == 0) return 0;

    cout << no << " Check at " << target->getname() << ",offset(" << offset << ") size(" << len << ")!" << endl;

    char *ptr1 = nullptr;
    char *ptr2 = nullptr;

    if (len <= BUFSIZE) {
        ptr1 = buffer1;
        ptr2 = buffer2;
    } else {
        ptr1 = (char *) malloc(len);
        ptr2 = (char *) malloc(len);
        if ((ptr1 == nullptr) || (ptr2 == nullptr)) {
            cout << "Malloc failed." << endl;
            return -1;
        }
    }

    memset(ptr1, 0, len);
    memset(ptr2, 0, len);

    char *ptr = ptr1;
    for (int i = 0; i < len; i++)
        ptr[i] = rand() % 128;

    ret = read(offset, ptr2, len);
    ret = memcmp(ptr1, ptr2, len);

    cout << "memcmp : ret = " << ret << endl;

    if (ret != 0) {
        checkmemory(ptr1, ptr2, len, "Dismatch details:");
    }

    if (len > BUFSIZE) {
        free(ptr1);
        free(ptr2);
    }

    return ret;
}

int Client::batchWrite(int time) {
    srand(seed);
    int capacity = target->getcapacity();
    int i;

    for (i = 0; i < time; i++) {
        if (-1 == randomWrite(i, time, capacity)) {
            cout << "Total write " << i << " times!" << endl;
            break;
        }
    }
    return i;
}

int Client::batchCheck(int time) {
    srand(seed);
    int capacity = target->getcapacity();
    int i;

    for (i = 0; i < time; i++) {
        if (0 != randomCheck(i, time, capacity)) {
            cout << "Invalid data found!" << endl;
            break;
        }
    }
    return i;
}


int Client::get_disk_num() {
    return target->get_disk_size();
}

void Client::set_rand_disk_hung() {
    target->set_rand_disk_hung();
}

void Client::set_rand_disk_hung1() {
    target->set_rand_disk_hung1();
}

void Client::prin_disk_info() {
    cout << "-------------------------------" << endl << endl;
    target->pritinfo();
    cout << endl << endl << "-------------------------------" << endl;
}
