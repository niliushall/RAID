#include <iostream>
#include "Storage.h"
#include "RamDisk.h"
#include <cstdlib>
#include <string>
#include <ctime>
#include "Client.h"
#include "RAID0.h"
#include "RAID1.h"
#include "RAID5.h"
#include "RAID6.h"
#include <windows.h>
#include <fstream>
#include "FileDisk.h"

using namespace std;

int checkmemory(void *buf1, void *buf2, size_t len, string s);

int checkOneRW(Client *c, int offset, void *buf, int len) {
    void *buf2;
    c->write(offset, buf, len);

    buf2 = malloc(len);
    c->read(offset, buf2, len);


    if (memcmp(buf, buf2, len) == 0) {
        cout << "Exactly same!" << endl;
    } else {
        checkmemory(buf, buf2, 225, "Read/Write dismatch:");
    }

    free(buf2);

    return 0;
}

int randomhungdisk(Client *c, int hung_num) {
    if (hung_num == 1) {
        c->set_rand_disk_hung();
    } else if (hung_num == 2) {
        c->set_rand_disk_hung1();
        c->set_rand_disk_hung();
    }
    return 0;
}

int randomCheck(Client *c, int checktime, int hung_disk_num) {

    c->batchWrite(checktime);
    randomhungdisk(c, hung_disk_num);
    int ret = c->batchCheck(checktime);
    if (ret == checktime) {
        cout << endl;
        cout << "****************************" << endl;
        cout << "OK,all data is  correct!" << endl;
        cout << endl;
    } else {
        cout << endl;
        cout << "**************************************************" << endl;
        cout << "sorry,something is wrong!please check you program" << endl;
        cout << endl;
    }
    return ret;
}


/*int main() {
    int level = 0;
    int disk_num = 5;

    RamDisk disk[disk_num];
    for(int i = 0; i < disk_num; i++) {
        disk[i].initDisk();
    }

    RAID* r = (RAID6 *) new RAID6;
    for(int i = 0; i < disk_num; i++) {
        r->addDisk(&disk[i]);
    }
    r->initRAID();

    Client c(r);
    randomCheck(&c, 5, 2);

    system("pause");
    return 0;
}*/

/*
int main() {
    string file = "F:\\test.txt";
    ifstream readfile(file, ios::in);
    if(readfile.good()) {
        cout << "ÎÄ¼þ´æÔÚ\n";
        readfile.close();
    }

    if(remove(file.c_str()) == 0) {
        cout << "remove...\n";
        return 0;
    }
    cout << "É¾³ý´íÎó\n";
}*/

int main() {
    FileDisk disk[5];
    for(int i = 0; i < 5; i++) {
        disk[i].initDisk();
    }

    RAID* r = (RAID6*) new RAID6;
    for(int i = 0; i < 5; i++) {
        r->addDisk(&disk[i]);
    }
    r->initRAID();

    Client c(r);
    randomCheck(&c, 5, 2);

    system("pause");

    return 0;
}