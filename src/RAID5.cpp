#include "RAID.h"
//#include <bits/stl_algo.h>
#include <string.h>
#include "RAID5.h"
#include <stdio.h>


int getstate();

RAID5::RAID5() : slidesize(SLIDESIZE) {
    set_name("raid5");
}

RAID5::~RAID5() {
}

int RAID5::rebuildRAID() {
    int ret;
    char *old_data = NULL;
    if (SUCCESS != (ret = alloc_read_disk_data((size_t) ((disklist.size() - 1) * DISK_SIZE), old_data))) {
    } else {

        if (SUCCESS != (ret = read_all_data(old_data))) {
        }
    }
    return write(0, old_data, (disklist.size() - 1) * DISK_SIZE);
}

int RAID5::initRAID() {
    state = RAIDSTATE_NOTREADY;
    if (disklist.size() < 2) {
        cout << "No enough disks(" << disklist.size() << ")" << endl;
        return state;
    }

    /*检测大小是否一致*/
    list<Disk *>::iterator iter;
    size_t diskcapacity = (*disklist.begin())->getcapacity();
    for (iter = disklist.begin(); iter != disklist.end(); iter++) {
        if (diskcapacity != (*iter)->getcapacity()) {
            cout << "Disk(" << (*iter)->getname() << ") size is not same with disk 0, please check again!" << endl;
            return state;
        }
        if ((*iter)->getstate() != DISKSTATE_READY) {
            cout << "Disk(" << (*iter)->getname() << ") not ready, please check again!" << endl;
            return state;
        }
    }
    capacity = diskcapacity * (disklist.size() - 1);/* we can assume that one disk is used to store checksum*/
    return 0;
}

int RAID5::getstate() {
    state = RAIDSTATE_READY;
    list<Disk *>::iterator iter;
    for (iter = disklist.begin(); iter != disklist.end(); iter++) {
        if ((*iter)->getstate() != DISKSTATE_READY) {
            state = RAIDSTATE_FAILED;
            cout << "Disk(" << (*iter)->getname() << ") in RAID5(" << getname() << ") is not ready, please check again!"
                 << endl;
            break;
        }
    }
    return state;
}

/*容忍一个盘出现问题,两个或者两个以上将视为FAILED*/
int RAID5::check_raid_state() {
    int state = READY;
    list<Disk *>::iterator iter;
    int disk_no = 0;
    fail_disks.clear();
    for (iter = disklist.begin(); iter != disklist.end(); iter++) {
        if ((*iter)->getstate() != DISKSTATE_READY) {
            fail_disks.push_back(disk_no);
        }
        disk_no++;
    }
    if (fail_disks.size() > 1) {
        setstate(FAILED);
        state = FAILED;
    }
    return state;
}


size_t RAID5::read(int offset, void *buf, size_t count) {
    return operate(offset, buf, count, OP_READ);
}

size_t RAID5::write(int offset, void *buf, size_t count) {
    return operate(offset, buf, count, OP_WRITE);
}

/*
* offset [in] offset of raid5 data(not include checksum data)
*	len		 [in] length of data you want to write or read
*/
int RAID5::addressMapping(int offset, size_t len) {
    if (offset < 0 || (size_t) (offset + len) > capacity) {
        cout << "Invalid parameters.offset(" << offset << "),capacity(" << capacity << ")!" << endl;
        return -1;
    }
    cout << "AddrMapping: offset(" << offset << "),len(" << len << ")" << endl;
    diskaddrlist.clear();

    DISKADDR diskaddr;
    int disknum = disklist.size();

    int slideno = offset / slidesize;                        /*块的id*/
    int layer_no = slideno / (disknum - 1);        /*块所在层数*/
    int current_checksum_diskno = layer_no % disknum;  /*校验盘id*/
    int disk_no = slideno % (disknum - 1);            /*所在磁盘id*/
    if (disk_no >= current_checksum_diskno)    /*if disk id  grear than current checksum disk id,we get next disk*/
    {
        disk_no++;
    }

    int offset1 = layer_no * slidesize + offset % slidesize;  /*在对应磁盘中的偏移*/
    diskaddr.diskno = disk_no;            /*数据所在的磁盘号*/
    diskaddr.offset = offset1;        /*设置在所在磁盘偏移*/

    /*一个磁盘中就能容下所有数据*/
    if (slidesize - offset % slidesize >= (int) len) {
        diskaddr.len = len;
        diskaddr.layerno = layer_no;
        diskaddrlist.push_back(diskaddr);
    } else {
        int towrite = len;
        diskaddr.len = slidesize - offset % slidesize;/*选中第一块磁盘长度*/
        diskaddr.layerno = layer_no;
        towrite -= diskaddr.len;
        diskaddrlist.push_back(diskaddr); /*添加第一块到diskaddrlist*/
        while (towrite > 0) {
            ++slideno;
            layer_no = slideno / (disknum - 1);        /*块所在层数*/
            current_checksum_diskno = layer_no % disknum;  /*校验盘id*/
            disk_no = slideno % (disknum - 1);            /*所在磁盘id*/
            if (disk_no >=
                current_checksum_diskno)    /*if disk id  grear than current checksum disk id,we get next disk*/
            {
                disk_no++;
            }
            offset1 = layer_no * slidesize;  /*在对应磁盘中的偏移*/
            diskaddr.diskno = disk_no;            /*数据所在的磁盘号*/
            diskaddr.offset = offset1;        /*设置在所在磁盘偏移*/
            diskaddr.layerno = layer_no;

            if (towrite < slidesize)//写数据到写最后一块（有剩余）
            {
                diskaddr.len = towrite;
            } else {
                diskaddr.len = slidesize;
            }
            towrite -= diskaddr.len;
            diskaddrlist.push_back(diskaddr);  /*添加数据磁盘信息到diskaddrlist*/
        }
    }
    list<DISKADDR>::iterator iter;
    for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
        DISKADDR *ptr = &(*iter);
        cout << "AddrMapping: diskno(" << ptr->diskno << ") layer(" << ptr->layerno << "),offset(" << ptr->offset
             << "),len(" << ptr->len << ")." << endl;
    }
    return diskaddrlist.size();
}

Disk *RAID5::getDisk(int diskno) {
    if (diskno > (int) disklist.size()) {
        return NULL;
    }
    list<Disk *>::iterator iter = disklist.begin();
    for (int i = 0; i < diskno; i++)
        iter++;
    return *iter;
}

size_t RAID5::operate(int offset, void *buf, size_t count, int opflag) {
    int ret;
    ret = addressMapping(offset, count);    /*map data into all disks, all informatioin  is stored in diskaddrlist*/
    if (ret < 0) {
        cout << "Invalid offset and count" << endl;
        return -1;
    }
    return operatedisk(buf, opflag, diskaddrlist);
}


/*
* diskno  [in]  disk id
* offset  [in]  offset of disk
* buf     if read, buf is the space to store read data,if write ,buf is the space  to store write data
*/
size_t RAID5::operatedisk(void *buf, int opflag, list<DISKADDR> &diskaddrlist) {
    int finished = 0;
    void *dataptr = buf;
    if (FAILED == check_raid_state())  /*RAID5 state check */
    {
        cout << "RAID5 current stat is FAILED" << endl;
        return -1;
    }
    switch (opflag) {
        case OP_READ_REBUILD: {
            list<DISKADDR>::iterator iter;
            for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                DISKADDR *ptr = &(*iter);
                if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                    return -1;
                } else {
                    finished += ptr->len;
                    cout << "Read  rebuild  disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                         << ptr->len << ")!" << endl;
                    dataptr = (char *) dataptr + ptr->len;

                }
            }
            return finished;
        }
        case OP_READ: {
            if (fail_disks.size() == 0)/*没有一个磁盘不正常*/
            {
                list<DISKADDR>::iterator iter;
                for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                    DISKADDR *ptr = &(*iter);
                    if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                        return -1;
                    } else {
                        finished += ptr->len;
                        cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count(" << ptr->len
                             << ")!" << endl;
                        dataptr = (char *) dataptr + ptr->len;
                    }
                }
                return finished;
            } else if (fail_disks.size() == 1)/*有一个磁盘不正常*/
            {
                int fail_disk_no = fail_disks.at(0);
                list<DISKADDR>::iterator iter;
                for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                    DISKADDR *ptr = &(*iter);
                    if (ptr->diskno == fail_disk_no)  /*如果存取数据的盘坏掉，需要从其他盘来恢复*/
                    {
                        char restore_data[SLIDESIZE];
                        memset(restore_data, 0, SLIDESIZE);
                        if (SUCCESS != raid_disk_restore(ptr, restore_data)) {
                            return -1;
                        } else {
                            memmove(dataptr, restore_data + (ptr->offset % SLIDESIZE), ptr->len);
                            finished += ptr->len;

                            cout << "restore data   disk(" << ptr->diskno << ") layer(" << ptr->layerno << ") offset("
                                 << ptr->offset << "),count(" << ptr->len << ")!" << endl;
                            dataptr = (char *) dataptr + ptr->len;
                        }
                    } else if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                        return -1;
                    } else {
                        finished += ptr->len;

                        cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count(" << ptr->len
                             << ")!" << endl;
                        dataptr = (char *) dataptr + ptr->len;
                    }
                }
                return finished;
            }
        }
        case OP_WRITE: {
            list<DISKADDR>::iterator iter;
            for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                getDisk(iter->diskno)->write(iter->offset, dataptr, iter->len);
                finished += iter->len;
                cout << "Write to disk(" << iter->diskno << ") offset(" << iter->offset << "),count(" << iter->len
                     << ")!" << endl;
/*				for(int i=0;i<iter->len;i++)
				{
					printf("%X  ",((char*)dataptr)[i]);
				}
				cout<<endl;*/
                dataptr = (char *) dataptr + iter->len;
            }
            write_check_sum_disk(diskaddrlist);
            return finished;
        }
        case OP_WRITE_CHECK_SUM: {
            list<DISKADDR>::iterator iter;
            for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {

                getDisk(iter->diskno)->write(iter->offset, dataptr, iter->len);
                finished += iter->len;
/*				cout<<"check sum data:"<<endl;
				for(int i=0;i<iter->len;i++)
				{
					std::cout<<hex<<(int)((char*)(dataptr))[i]<<" ";
				}
				cout<<endl;*/
/*				cout<<"Write check sum to disk("<<iter->diskno<<") offset("<<iter->offset<<"),count("<<iter->len<<")!"<<endl;
				for(int i=0;i<iter->len;i++)
				{
					printf("%X  ",((char*)dataptr)[i]);
				}
				cout<<endl;*/
                dataptr = (char *) dataptr + iter->len;
            }
            return finished;
        }
        default:
            cout << "Invalid opflag(" << opflag << ")" << endl;
            return 0;
    }
}


int RAID5::write_check_sum_disk(list<DISKADDR> &addrlist) {
    int ret = SUCCESS;
    std::vector<int> influenced_layers;
    list<DISKADDR>::iterator iter;
    for (iter = addrlist.begin(); iter != addrlist.end(); iter++) {
        influenced_layers.push_back(iter->layerno);
    }
    vector<int>::iterator new_end;
    new_end = unique(influenced_layers.begin(), influenced_layers.end());
    influenced_layers.erase(new_end, influenced_layers.end());
    vector<int>::iterator layers_iter;
    char *data_ptr = NULL;
    char check_sum[SLIDESIZE];
    if (SUCCESS != (ret = alloc_read_disk_data((size_t) (disklist.size() - 1) * SLIDESIZE, data_ptr))) {
    } else {
        for (layers_iter = influenced_layers.begin();
             SUCCESS == ret && layers_iter != influenced_layers.end(); layers_iter++) {
            int check_sum_disk_no = (*layers_iter) % (int) disklist.size();
            if (SUCCESS !=
                (ret = read_layer_data(*layers_iter, data_ptr, check_sum_disk_no)))        /*read one layer data*/
            {
            }
                /*get data check sum of one layer*/
            else if (SUCCESS != (ret = get_data_check_sum(data_ptr, (int) disklist.size() - 1, check_sum))) {
            } else if (SUCCESS != (ret = write_check_sum(*layers_iter, check_sum)))    /*write one check sum block*/
            {
            }
        }
    }
    if (data_ptr != NULL) {
        free(data_ptr);
    }
    return ret;
}

int RAID5::alloc_read_disk_data(size_t size, char *&ptr) {
    int ret = SUCCESS;
    ptr = NULL;
    ptr = (char *) malloc(size);
    if (NULL == ptr) {
        ret = ERROR;
    }
    return ret;
}


int RAID5::read_layer_data(int layno, char *data_ptr, int except_disk_no) {
    int ret = SUCCESS;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    for (size_t i = 0; i < disklist.size(); i++) {
        if (except_disk_no != i) {
            disk_addr.layerno = layno;
            disk_addr.diskno = i;
            disk_addr.len = SLIDESIZE;
            disk_addr.offset = SLIDESIZE * layno;
            disk_addr_list.push_back(disk_addr);
        }
    }
    size_t data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if (data_size != SLIDESIZE * ((disklist.size() - 1))) {
        ret = ERROR;
    }
    return ret;
}

int RAID5::read_all_data(char *all_data) {
    int ret = SUCCESS;
    int check_sum;
    int disk_no;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    for (size_t i = 0; i < disklist.size() * DISK_SIZE / SLIDESIZE; i++) {
        check_sum = (i / disklist.size()) % disklist.size();
        disk_no = i % disklist.size();
        if (disk_no != check_sum) {
            disk_addr.layerno = i / disklist.size();
            disk_addr.diskno = i;
            disk_addr.len = SLIDESIZE;
            disk_addr.offset = SLIDESIZE * disk_addr.layerno;
            disk_addr_list.push_back(disk_addr);
        }
    }
    size_t data_size = operatedisk(all_data, OP_READ_REBUILD, disk_addr_list);
    if (data_size != DISK_SIZE * ((disklist.size() - 1))) {
        ret = ERROR;
    }
    return ret;
}

int RAID5::get_data_check_sum(char *data_ptr, int data_disk_num, char *check_sum) {
    int ret = SUCCESS;
    char *char_check_sum = NULL;
    if (NULL == (char_check_sum = (char *) malloc(data_disk_num * sizeof(char)))) {
        ret = ERROR;
    }
    for (int char_index = 0; SUCCESS == ret && char_index < SLIDESIZE; char_index++) {
        for (int k = 0; SUCCESS == ret && k < data_disk_num; k++) {
            char_check_sum[k] = data_ptr[k * SLIDESIZE + char_index];
        }
        calculate_check_sum(char_check_sum, data_disk_num, check_sum[char_index]);
    }
    if (char_check_sum) {
        free(char_check_sum);
    }
    return ret;
}

void RAID5::calculate_check_sum(char *char_list, int data_disk_num, char &check_sum) {
    check_sum = 0;
    bool flag = false;
    for (char i = 0; i < 8; i++) {
        flag = false;
        char check = 1 << i;
        for (int k = 0; k < data_disk_num; k++) {
            if (check & char_list[k]) {
                flag = !flag;
            }
        }
        if (flag) {
            check_sum = check_sum | check;
        }
    }
}

/*write one  check_sum  block to check_sum_disk*/
int RAID5::write_check_sum(int layerno, char *check_sum) {
    int ret = SUCCESS;
    int check_sum_disk_no = layerno % (int) disklist.size();
    list<DISKADDR> addrlist;
    DISKADDR diskaddr;
    diskaddr.layerno = layerno;
    diskaddr.diskno = check_sum_disk_no;
    diskaddr.len = SLIDESIZE;
    diskaddr.offset = (size_t) layerno * SLIDESIZE;
    addrlist.push_back(diskaddr);
    if (SLIDESIZE != operatedisk(check_sum, OP_WRITE_CHECK_SUM, addrlist)) {
        ret = ERROR;
    }
    return ret;
}


int RAID5::raid_disk_restore(DISKADDR *file_disk_addr, char *restore_data) {
    int ret = SUCCESS;
    int layer_no = file_disk_addr->layerno;
    int fail_disk_no = file_disk_addr->diskno;
    char *data_ptr = NULL;
    if (SUCCESS != (ret = alloc_read_disk_data((size_t) (disklist.size() - 1) * SLIDESIZE, data_ptr))) {
    }
    if (SUCCESS != (ret = read_layer_data(layer_no, data_ptr, fail_disk_no)))    /*read one layer data*/
    {
    } else if (SUCCESS != (ret = get_data_check_sum(data_ptr, (int) disklist.size() - 1, restore_data))) {
    }
    if (data_ptr) {
        free(data_ptr);
    }
    return ret;
}
