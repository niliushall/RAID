#include "Storage.h"

using namespace std;

list<Storage *> Storage::store;
int Storage::count = 0;

Storage::Storage() : type("Storage"), state(-1), capacity(0) {
    stringstream ss;
    ss << type << Storage::count++;
    ss >> name;//=type+random(1000)

    Storage::addStorage(this);
}

Storage::~Storage() {
    Storage::removeStorage(this);
    //dtor
}

void Storage::setstate(int s) {
    if (isvalidstate(s))
        state = s;
    else
        cout << "Try to set invalid state(" << s << ")" << getname();
    return;
}

int Storage::addStorage(Storage *s) {
    if (NULL == s)
        return 0;
    Storage::store.push_back(s);
    return 1;
}

int Storage::removeStorage(Storage *s) {
    list<Storage *>::iterator it;
    for (it = Storage::store.begin(); it != Storage::store.end(); it++) {
        if (s->getname() == (*it)->getname()) {
            it = Storage::store.erase(it);
            return 1;
        }
    }

    return 0;
}

int Storage::getStoreageList(list<Storage *> nl) {
    int count = 0;
    nl.clear();
    list<Storage *>::iterator it;
    for (it = Storage::store.begin(); it != Storage::store.end(); it++) {
        nl.push_back(*it);
        count++;
    }

    return count;
}

int Storage::printStorageList() {
    int count = 0;
    list<Storage *>::iterator it;
    for (it = Storage::store.begin(); it != Storage::store.end(); it++) {
        (*it)->printinfo();
        count++;
    }
    return count;
}


void Storage::printinfo(int tab) {
    cout.flags(ios::left);
    cout << setw(tab) << "" << setw(10) << type << setw(15) << name << setw(5) << state << endl;
}

size_t Storage::read(int offset, void *buf, size_t count) {
    return -1;
}

size_t Storage::write(int offset, void *buf, size_t count) {
    cout << "\n\n\nStorage::write\n\n\n" << endl;
    Sleep(10000);
    return -1;
}

bool Storage::isvalidstate(int s) {
    return false;
}

int Storage::get_disk_size() {
    return 0;
}

int Storage::set_rand_disk_hung1() {
    return 0;
}

int Storage::set_rand_disk_hung() {
    return 0;
}

void Storage::pritinfo() {
}
