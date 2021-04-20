#include "RAID.h"
#include <cstring>
#include "RAID6.h"

int getstate();

RAID6::RAID6() : slidesize(SLIDESIZE) {
    name = "RAID6";
    generate_GF_table();
}

RAID6::~RAID6() {

}

uint8_t RAID6::gf_pow(uint8_t exp) { return GFILOG[exp]; }

void RAID6::generate_GF_table() {
    int ILOG_data;

    GFILOG[0] = 1;
    for (int i = 1; i < 256; i++) {
        ILOG_data = GFILOG[i - 1] * 2;
        if (ILOG_data >= 256) {
            ILOG_data ^= 285;
        }
        GFILOG[i] = ILOG_data;
        GFLOG[GFILOG[i]] = i;
    }
}

int RAID6::gf_mul(int a, int b) {
    if (a == b) {
        return 0;
    }
    if (b == 0) {
        return a;
    }

    return GFILOG[(GFLOG[a] ^ GFLOG[b]) % 255];
}

int RAID6::gf_div(int a, int b) {
    if (a == b) {
        return 0;
    }

    if (b == 0) {
        return a;
    }

    return GFILOG[((GFLOG[a] ^ GFLOG[b]) + 255) % 255];
}

int RAID6::initRAID() {
    state = RAIDSTATE_NOTREADY;
    if (disklist.size() < 4) {
        cout << "No enough disks(" << disklist.size() << ")" << endl;
        return state;
    }

    // 检测大小是否一致
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
    capacity = diskcapacity * (disklist.size() - 2);// we can assume that two disk is used to store checksum

    for (int i = 0; i < disklist.size() - 2; i++) {
        gfParameter.push_back(rand() % 255 + 1);
    }

    return 0;
}

int RAID6::getstate() {
    state = RAIDSTATE_READY;
    list<Disk *>::iterator iter;
    for (iter = disklist.begin(); iter != disklist.end(); iter++) {
        if ((*iter)->getstate() != DISKSTATE_READY) {
            state = RAIDSTATE_FAILED;
            cout << "Disk(" << (*iter)->getname() << ") in RAID6(" << getname() << ") is not ready, please check again!"
                 << endl;
            break;
        }
    }
    return state;
}

// 容忍两个盘出现问题,三个或者三个以上将视为FAILED
int RAID6::check_raid_state() {
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
    if (fail_disks.size() > 2) {
        setstate(FAILED);
        state = FAILED;
    }
    return state;
}

size_t RAID6::read(int offset, void *buf, size_t count) {
    return operate(offset, buf, count, OP_READ);
}

size_t RAID6::write(int offset, void *buf, size_t count) {
    return operate(offset, buf, count, OP_WRITE);
}

/*
* offset [in] offset of raid5 data(not include checksum data)
*	len		 [in] length of data you want to write or read
*/
int RAID6::addressMapping(int offset, size_t len) {
    if (offset < 0 || (size_t) (offset + len) > capacity) {
        cout << "Invalid parameters.offset(" << offset << "),capacity(" << capacity << ")!" << endl;
        return -1;
    }

    cout << "AddrMapping: offset(" << offset << "),len(" << len << ")" << endl;

    diskaddrlist.clear();

    DISKADDR diskaddr;
    int disknum = disklist.size() - 2;
    int slideno = offset / slidesize; //the no of slide, start from 0 层
    int layer_no = slideno / disknum;
    diskaddr.layerno = layer_no;
    int offset1 = slideno / disknum * slidesize + offset % slidesize;
    int current_diskno = slideno % disknum;//当前的磁盘号

    diskaddr.diskno = current_diskno;
    diskaddr.offset = offset1;

    if (slidesize - offset % slidesize >= (int) len)//write to the last disk and only use the one disk
    {
        diskaddr.len = len;
        diskaddrlist.push_back(diskaddr);
    } else {
        int towrite = len;
        diskaddr.len = slidesize - offset % slidesize;
        towrite -= diskaddr.len;
        diskaddrlist.push_back(diskaddr);

        while (towrite > 0) {
            slideno++;
            layer_no = slideno / disknum;
            diskaddr.layerno = layer_no;
            current_diskno = slideno % disknum;//当前的磁盘号

            diskaddr.diskno = current_diskno;
            diskaddr.offset = slideno / disknum * slidesize;//当前磁盘的偏移量
            if (towrite < slidesize)//写数据到写最后一块（有剩余）
                diskaddr.len = towrite;
            else
                diskaddr.len = slidesize;
            towrite -= diskaddr.len;
            diskaddrlist.push_back(diskaddr);
        }
    }
    list<DISKADDR>::iterator iter;

    for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
        DISKADDR *ptr = &(*iter);
        cout << "AddrMapping: diskno(" << ptr->diskno << "),offset(" << ptr->offset << "),len(" << ptr->len << ")."
             << endl;
    }

    return diskaddrlist.size();
}

Disk *RAID6::getDisk(int diskno) {
    if (diskno > (int) disklist.size()) {
        return NULL;
    }
    auto iter = disklist.begin();
    for (int i = 0; i < diskno; i++)
        iter++;
    return *iter;
}

size_t RAID6::operate(int offset, void *buf, size_t count, int opflag) {
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
size_t RAID6::operatedisk(void *buf, int opflag, list<DISKADDR> &diskaddrlist) {
    int finished = 0;
    uint8_t *dataptr = static_cast<uint8_t *>(buf);
    uint8_t restore_data[SLIDESIZE] = {0};
    if (FAILED == check_raid_state())  /*RAID6 state check */
    {
        cout << "RAID6 current stat is FAILED" << endl;
        return -1;
    }

    switch (opflag) {
        case OP_READ_REBUILD: {
            for (auto &iter : diskaddrlist) {
                DISKADDR *ptr = &iter;
                if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                    return -1;
                } else {
                    finished += ptr->len;
                    cout << "Read  rebuild  disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                         << ptr->len << ")!" << endl;
                    dataptr = (uint8_t *) dataptr + ptr->len;
                }
            }
            return finished;
        }
        case OP_READ: {
            if (fail_disks.size() == 0) { // 磁盘正常
                for (auto iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                    DISKADDR *ptr = &(*iter);
                    if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                        return -1;
                    } else {
                        finished += ptr->len;
                        cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count(" << ptr->len
                             << ")!" << endl;

                        dataptr = (uint8_t *) dataptr + ptr->len;
                    }
                }
                return finished;
            } else if (fail_disks.size() == 1) { // 一个磁盘损坏
                int fail_disk_no = fail_disks.at(0);
                if (fail_disk_no == disklist.size() - 1 || fail_disk_no == disklist.size() - 2) {   // 只有一个校验盘损坏
                    for (auto &iter : diskaddrlist) {
                        DISKADDR *ptr = &iter;
                        if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;
                            dataptr = (uint8_t *) dataptr + ptr->len;
                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;
                        }
                    }
                    return finished;
                } else {    // 一个磁盘损坏：数据盘损坏
                    for (auto &iter : diskaddrlist) {
                        DISKADDR *ptr = &iter;
                        if (ptr->diskno == fail_disk_no) {  // 如果存取数据的盘坏掉，需要从其他盘来恢复
                            if (SUCCESS != raid_disk_restore_by_P(ptr, restore_data)) {
                                return -1;
                            } else {
                                memmove(dataptr, restore_data + (ptr->offset % SLIDESIZE), ptr->len);
                                finished += ptr->len;
                                cout << "restore data   disk(" << ptr->diskno << ") layer(" << ptr->layerno
                                     << ") offset(" << ptr->offset << "),count(" << ptr->len << ")!" << endl;
                                dataptr = (uint8_t *) dataptr + ptr->len;
                            }
                        } else if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;
                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;
                            dataptr = (uint8_t *) dataptr + ptr->len;
                        }
                    }
                    return finished;
                }
            } else {    // 两个磁盘损坏
                int fail_disk_no1 = fail_disks.at(0);
                int fail_disk_no2 = fail_disks.at(1);
                if (fail_disk_no1 > fail_disk_no2) {
                    swap(fail_disk_no1, fail_disk_no2);
                }
                if ((fail_disk_no1 == disklist.size() - 2) && (fail_disk_no2 == disklist.size() - 1)) {
                    for (auto &iter : diskaddrlist) {
                        DISKADDR *ptr = &iter;
                        if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;
                            dataptr = (uint8_t *) dataptr + ptr->len;
                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;
                        }
                    }
                    return finished;
                } else if (fail_disk_no2 == disklist.size() - 1) {  // 一个数据盘损坏，一个Q校验盘损坏
                    int fail_disk_no = fail_disk_no1;
                    for (auto &iter : diskaddrlist) {
                        DISKADDR *ptr = &iter;
                        if (ptr->diskno == fail_disk_no) {  // 数据恢复
                            if (raid_disk_restore_by_P(ptr, restore_data) != SUCCESS) {
                                return -1;
                            }

                            memmove(dataptr, restore_data + (ptr->offset % SLIDESIZE), ptr->len);
                            finished += ptr->len;
                            cout << "restore data   disk(" << ptr->diskno << ") layer(" << ptr->layerno
                                 << ") offset(" << ptr->offset << "),count(" << ptr->len << ")!" << endl;

                            dataptr = (uint8_t *) dataptr + ptr->len;
                        } else if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;
                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;
                            dataptr = (uint8_t *) dataptr + ptr->len;
                        }
                    }
                    return finished;
                } else if (fail_disk_no2 == disklist.size() - 2) {   // 一个数据盘损坏，一个P校验盘损坏
                    int fail_disk_no = fail_disk_no1;
                    for (auto &iter : diskaddrlist) {
                        DISKADDR *ptr = &iter;
                        if (ptr->diskno == fail_disk_no) {
                            if (raid_disk_restore_by_Q(ptr, restore_data) != SUCCESS) {
                                cout << "raid_disk_restore_by_Q error..." << endl;
                                return ERR;
                            }
                            memmove(dataptr, restore_data + (ptr->offset % SLIDESIZE), ptr->len);
                        } else {
                            if (getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                                cout << "getDisk error : " << ptr->diskno << ", offset = " << ptr->offset << ", len = "
                                     << ptr->len << endl;
                                return ERR;
                            }
                        }

                        finished += ptr->len;
                        cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                             << ptr->len << ")!" << endl;
                        dataptr += ptr->len;
                    }
                    return finished;
                } else {    // 两个数据盘损坏
                    for (auto &iter : diskaddrlist) {
                        DISKADDR *ptr = &iter;
                        if (ptr->diskno == fail_disk_no1 || ptr->diskno == fail_disk_no2) {
                            if (raid_disk_restore_by_P_Q(ptr, restore_data, fail_disk_no1, fail_disk_no2) != SUCCESS) {
                                cout << "raid_disk_restore_by_P_Q error..." << endl;
                                return ERR;
                            }
                            memmove(dataptr, restore_data + (ptr->offset % SLIDESIZE), ptr->len);
                        } else {
                            if (getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                                cout << "getDisk error : " << ptr->diskno << ", offset = " << ptr->offset << ", len = "
                                     << ptr->len << endl;
                                return ERR;
                            }
                        }
                    }

                    /*循环结束后，数据恢复，将所有盘的状态都置为READY*/
                    list<Disk *>::iterator it;
                    for (it = disklist.begin(); it != disklist.end(); it++) {
                        cout << 11 << endl;
                        if ((*it)->getstate() == DISKSTATE_HUNG) //确保被hung 的是正常的disk
                        {
                            cout << 11 << endl;
                            (*it)->setstate(DISKSTATE_READY);
                        }
                    }

                    /*循环结束后，所有数据均已恢复
                     * 下面的过程就是当作无损坏盘进行读取数据*/
                    list<DISKADDR>::iterator iter;
                    for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                        DISKADDR *ptr = &(*iter);
                        if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;
                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;
                            /*						for(int i=0;i<ptr->len;i++)
                                                {
                                                    printf("%X  ",((char*)dataptr)[i]);
                                                }
                                                cout<<endl;*/
                            dataptr = (uint8_t *) dataptr + ptr->len;
                        }
                    }
                    return SUCCESS;
                }
            }
        }

        case OP_WRITE: {
            list<DISKADDR>::iterator iter;

            for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                getDisk(iter->diskno)->write(iter->offset, dataptr, iter->len);
                finished += iter->len;
                dataptr = (uint8_t *) dataptr + iter->len;
                cout << "Write to disk(" << iter->diskno << ") offset(" << iter->offset << "),count(" << iter->len
                     << ")!" << endl;
            }
            write_check_sum_disk(diskaddrlist);

            return finished;
        }

        case OP_WRITE_CHECK_SUM: {
            list<DISKADDR>::iterator iter;
            for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                getDisk(iter->diskno)->write(iter->offset, dataptr, iter->len);

                finished += iter->len;
                dataptr = (uint8_t *) dataptr + iter->len;

                cout << "Write check sum to disk(" << iter->diskno << ") offset(" << iter->offset << "),count("
                     << iter->len << ")!" << endl;
            }
            return finished;
        }
        default:
            cout << "Invalid opflag(" << opflag << ")" << endl;
            return 0;
    }
}

void RAID6::flag_init(bool *flag) {
    for (int i = 0; i < disklist.size() - 2; i++) {
        flag[i] = false;
    }
}

int RAID6::all_data_restored(bool *flag) {
    for (int i = 0; i < disklist.size() - 2; i++) {
        if (flag[i] == false)
            return false;
    }
    return true;
}

int RAID6::count_known_datas(bool *flag, int i, int _layer, int fail_disk_no1, int fail_disk_no2, int except_disk_no) {
    int fail_disk_count = 0;
    int disk_no = i;
    int layer_no;
    for (int j = 0; j < disklist.size() - 2; j++) {
        layer_no = _layer + j;
        if (flag[layer_no] == false && ((disk_no == fail_disk_no1) || (disk_no == fail_disk_no2))) {
            fail_disk_count++;
            except_disk_no = disk_no;
        }
        disk_no = (disk_no + 1) % (disklist.size() - 2);
    }
    return fail_disk_count;
}

/*生成P、Q校验数据，并写入到每一层的校验磁盘*/
int RAID6::write_check_sum_disk(list<DISKADDR> &addrlist) {
    int ret = SUCCESS;

    uint8_t *data_ptr = nullptr;
    uint8_t check_sum_P[SLIDESIZE] = {0};
    uint8_t check_sum_Q[SLIDESIZE] = {0};
    if (SUCCESS != (ret = alloc_read_disk_data((size_t) (disklist.size() - 2) * SLIDESIZE, data_ptr))) {
        return ERR;
    } else {
        for (int layers_iter = 0; SUCCESS == ret && layers_iter < DISK_SIZE / SLIDESIZE; layers_iter++) {
            ret = read_layer_data_for_check(layers_iter, data_ptr);
            if (ret == ERR) {
                return ERR;
            }

            ret = get_data_check_sum(data_ptr, (int) disklist.size() - 2, check_sum_P, check_sum_Q);
            if (ret == ERR) {
                return ERR;
            }

            ret = write_check_sum(layers_iter, (int) disklist.size() - 2, check_sum_P);
            if (ret == ERR) {
                return ERR;
            }

            ret = write_check_sum(layers_iter, (int) disklist.size() - 1, check_sum_Q);
            if (ret == ERR) {
                return ERR;
            }
        }
    }
    if (data_ptr != NULL) {
        free(data_ptr);
    }
    return ret;
}

int RAID6::alloc_read_disk_data(size_t size, uint8_t *&ptr) {
    int ret = SUCCESS;
    ptr = nullptr;
    ptr = (uint8_t *) malloc(size);
    if (ptr == nullptr) {
        ret = ERR;
    }
    memset(ptr, 0, size);
    return ret;
}

/* @berif 读取 layno 层所有磁盘（不包括 except_disk_no 和 Q）内容, layno 层的所有的数据复制到 ptr 中
 * layno  [in]   需要读取数据block 层
 * except_disk_no  [in]  该层中不需要读取的disk_no, -1 表示 该层所有的磁盘都需要读取数据
 **/

int RAID6::read_layer_data(int layno, uint8_t *data_ptr, int except_disk_no) {
    int ret = SUCCESS;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    for (int i = 0; i < disklist.size() - 1; i++) {
        if (i != except_disk_no) {
            disk_addr.layerno = layno;
            disk_addr.diskno = i;
            disk_addr.len = SLIDESIZE;
            disk_addr.offset = SLIDESIZE * layno;
            disk_addr_list.push_back(disk_addr);
        }
    }
    int data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if (data_size != SLIDESIZE * ((disklist.size() - 2))) {
        ret = ERR;
    }
    return ret;
}

int RAID6::read_layer_data_for_inclined_check(int layno, int disk_no, uint8_t *data_ptr, int except_disk_no) {
    int ret = SUCCESS;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    int temp_disk_no = disk_no;
    for (int i = 0; i < disklist.size() - 2; i++) {
        if (except_disk_no != temp_disk_no) {
            disk_addr.layerno = layno + i;
            disk_addr.diskno = temp_disk_no;
            disk_addr.len = SLIDESIZE;
            disk_addr.offset = SLIDESIZE * disk_addr.layerno;
            disk_addr_list.push_back(disk_addr);
        }
        temp_disk_no = (temp_disk_no + 1) % (disklist.size() - 1);

    }
    disk_addr.layerno = layno;
    disk_addr.diskno = disk_no;
    disk_addr.len = SLIDESIZE;
    disk_addr.offset = SLIDESIZE * layno;
    disk_addr_list.push_back(disk_addr);

    int data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if (data_size != SLIDESIZE * ((disklist.size() - 2))) {
        ret = ERR;
    }
    return ret;
}

// 读取某一层数据
int RAID6::read_layer_data_for_check(int layno, uint8_t *data_ptr) {
    int ret = SUCCESS;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;

    for (int i = 0; i < disklist.size() - 2; i++) {
        disk_addr.layerno = layno;
        disk_addr.diskno = i;
        disk_addr.len = SLIDESIZE;
        disk_addr.offset = SLIDESIZE * layno;
        disk_addr_list.push_back(disk_addr);
    }

    int data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if (data_size != SLIDESIZE * ((disklist.size() - 2))) {
        ret = ERR;
    }

    return ret;
}

/*
*  根据data_ptr 生成check_sum P+Q
*  data_ptr  [in]   数据盘同一层block的全部的数据量
*  data_disk_num  [in]  数据盘数量，不包括校验，相当于 全部盘数量减2
*  check_sum  [out]  用于表示data_ptr生成的校验block
*/
int RAID6::get_data_check_sum(const uint8_t *data_ptr, int data_disk_num, uint8_t *check_sum_P, uint8_t *check_sum_Q) {
    int ret = SUCCESS;
    uint8_t *char_check_sum = (uint8_t *) malloc(data_disk_num * sizeof(uint8_t));
    if (char_check_sum == nullptr) {
        ret = ERR;
    }
    for (int char_index = 0; SUCCESS == ret && char_index < SLIDESIZE; char_index++) {
        for (int k = 0; SUCCESS == ret && k < data_disk_num; k++) {
            char_check_sum[k] = data_ptr[k * SLIDESIZE + char_index];
        }
        calculate_check_sum_P(char_check_sum, data_disk_num, check_sum_P[char_index]);//char_check_sum 当前的原始数据
        calculate_check_sum_Q(char_check_sum, data_disk_num, check_sum_Q[char_index]);
    }
    if (char_check_sum) {
        free(char_check_sum);
    }
    return ret;
}

// 根据 char_list 字符，奇校验生成char的校验码 check_sum
void RAID6::calculate_check_sum_P(const uint8_t *char_list, int data_disk_num, uint8_t &check_sum) {
    check_sum = char_list[0];
    for (int i = 1; i < data_disk_num; i++) {
        check_sum ^= char_list[i];
    }
}

void RAID6::calculate_check_sum_Q(const uint8_t *char_list, int data_disk_num, uint8_t &check_sum) {
    check_sum = 0;
    for (int i = 0; i < data_disk_num; i++) {
        check_sum ^= gf_mul(char_list[i], gfParameter[i]);
    }
}

/*write one  check_sum  block to check_sum_disk*/
int RAID6::write_check_sum(int layerno, int diskno, uint8_t *check_sum) {
    list<DISKADDR> addrlist;
    DISKADDR diskaddr;
    diskaddr.layerno = layerno;
    diskaddr.diskno = diskno;
    diskaddr.len = SLIDESIZE;
    diskaddr.offset = (size_t) layerno * SLIDESIZE;
    addrlist.push_back(diskaddr);
    if (SLIDESIZE != operatedisk(check_sum, OP_WRITE_CHECK_SUM, addrlist)) {
        return ERR;
    }
    return SUCCESS;
}

int RAID6::write_check_sum_for_inclined_check(int layerno, int disk_no, uint8_t *check_sum) {
    int ret = SUCCESS;
    list<DISKADDR> addrlist;
    DISKADDR diskaddr;
    diskaddr.layerno = layerno;
    diskaddr.diskno = disk_no;
    diskaddr.len = SLIDESIZE;
    diskaddr.offset = (size_t) layerno * SLIDESIZE;
    addrlist.push_back(diskaddr);
    if (SLIDESIZE != operatedisk(check_sum, OP_WRITE_CHECK_SUM, addrlist)) {
        ret = ERR;
    }
    return ret;
}

// 恢复 DISKADDR 指向空间的数据
int RAID6::raid_disk_restore_by_P(DISKADDR *fail_disk_addr, uint8_t *restore_data) {
    int layer_no = fail_disk_addr->layerno;
    int fail_disk_no = fail_disk_addr->diskno;
    uint8_t *data_ptr = nullptr;
    if (SUCCESS != alloc_read_disk_data((size_t) (disklist.size() - 2) * SLIDESIZE, data_ptr)) {
        return ERR;
    }

    if (SUCCESS != read_layer_data(layer_no, data_ptr, fail_disk_no)) { // 获取某一层数据，除fail_disk_no磁盘
        return ERR;
    }

    // 通过XOR恢复数据
    for (int i = 0; i < SLIDESIZE; i++) {
        restore_data[i] = data_ptr[i];
        for (int diskno = 1; diskno < disklist.size() - 2; diskno++) {
            restore_data[i] ^= data_ptr[SLIDESIZE * diskno + i];
        }
    }

    if (data_ptr) {
        free(data_ptr);
    }

    return SUCCESS;
}

// P损坏，用Q恢复损坏的数据盘
int RAID6::raid_disk_restore_by_Q(DISKADDR *fail_disk_addr, uint8_t *restore_data) {
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    vector<int> diskNo;             // 未损坏的数据盘编号
    int layer_no = fail_disk_addr->layerno;
    int fail_disk_no = fail_disk_addr->diskno;
    uint8_t *data_ptr = nullptr;    // 存储未损坏数据盘的数据
    uint8_t *data_Q = nullptr;
    if (alloc_read_disk_data((size_t) (disklist.size() - 3) * SLIDESIZE, data_ptr) != SUCCESS) {
        return ERR;
    }
    if (alloc_read_disk_data(SLIDESIZE, data_Q) != SUCCESS) {
        return ERR;
    }

    // 获取未损坏数据盘的数据
    for (int i = 0; i < disklist.size() - 2; i++) {
        if (i != fail_disk_no) {
            disk_addr.diskno = i;
            disk_addr.layerno = layer_no;
            disk_addr.len = SLIDESIZE;
            disk_addr.offset = SLIDESIZE * layer_no;
            disk_addr_list.push_back(disk_addr);
            diskNo.push_back(i);
        }
    }
    size_t data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if (data_size != (disklist.size() - 3) * SLIDESIZE) {
        cout << "operatedisk(data_ptr) error" << endl;
        return ERR;
    }

    // 获取Q检验盘数据
    disk_addr_list.clear();
    disk_addr.diskno = disklist.size() - 1;
    disk_addr.layerno = layer_no;
    disk_addr.len = SLIDESIZE;
    disk_addr.offset = SLIDESIZE * layer_no;
    disk_addr_list.push_back(disk_addr);

    data_size = operatedisk(data_Q, OP_READ_REBUILD, disk_addr_list);
    if (data_size != SLIDESIZE) {
        cout << "restore by Q, operatedisk error." << endl;
        return ERR;
    }

    // A = （Q + B*K2 + C*K3) / K1
    for (int i = 0; i < SLIDESIZE; i++) {
        restore_data[i] = data_Q[i];
        for (int j = 0; j < diskNo.size(); j++) {
            restore_data[i] ^= gf_mul(data_ptr[SLIDESIZE * j + i], gfParameter[diskNo[j]]);
        }
        restore_data[i] = gf_div(restore_data[i], gfParameter[fail_disk_no]);
    }

    if (data_ptr) {
        free(data_ptr);
    }
    if(data_Q) {
        free(data_Q);
    }

    return SUCCESS;
}

int RAID6::raid_disk_restore_by_P_Q(DISKADDR *fail_disk_addr, uint8_t *restore_data, int fail_disk_no1, int fail_disk_no2) {
    list<DISKADDR> disk_addr_list;
    vector<int> diskNo;     // 存储未损坏磁盘编号
    DISKADDR disk_addr;
    int layerNo = fail_disk_addr->layerno;
    int fail_disk_no = fail_disk_addr->diskno;
    uint8_t *data_ptr = nullptr, *data_P = nullptr, *data_Q = nullptr;

    if (alloc_read_disk_data((size_t) SLIDESIZE * (disklist.size() - 3), data_ptr) != SUCCESS) {
        return ERR;
    }
    if (alloc_read_disk_data((size_t) SLIDESIZE * (disklist.size() - 3), data_P) != SUCCESS) {
        return ERR;
    }
    if (alloc_read_disk_data((size_t) SLIDESIZE * (disklist.size() - 3), data_Q) != SUCCESS) {
        return ERR;
    }

    // 获取未损坏数据盘的数据
    for(int i = 0; i < disklist.size() - 2; i++) {
        if(i != fail_disk_no1 && i != fail_disk_no2) {
            disk_addr.diskno = i;
            disk_addr.layerno = layerNo;
            disk_addr.len = SLIDESIZE;
            disk_addr.offset = SLIDESIZE * layerNo;
            disk_addr_list.push_back(disk_addr);
            diskNo.push_back(i);
        }
    }
    size_t data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if(data_size != SLIDESIZE * (disklist.size() - 3)) {
        cout << "restore by P & Q, operatedisk(data_ptr) error..." << endl;
        return ERR;
    }

    // 获取P校验盘数据
    disk_addr.diskno = disklist.size() - 2;
    disk_addr.layerno = layerNo;
    disk_addr.len = SLIDESIZE;
    disk_addr.offset = SLIDESIZE * layerNo;

    disk_addr_list.clear();
    disk_addr_list.push_back(disk_addr);
    data_size = operatedisk(data_P, OP_READ_REBUILD, disk_addr_list);
    if(data_size != SLIDESIZE) {
        cout << "restore by P & Q, operatedisk(data_P) error..." << endl;
        return ERR;
    }

    // 获取Q校验盘数据
    disk_addr.diskno = disklist.size() - 1;
    disk_addr.layerno = layerNo;
    disk_addr.len = SLIDESIZE;
    disk_addr.offset = SLIDESIZE * layerNo;

    disk_addr_list.clear();
    disk_addr_list.push_back(disk_addr);
    data_size = operatedisk(data_Q, OP_READ_REBUILD, disk_addr_list);
    if(data_size != SLIDESIZE) {
        cout << "restore by P & Q, operatedisk(data_Q) error..." << endl;
        return ERR;
    }

    // 恢复数据，B = ( Q + P*K1 + C*K1 + C*K3) / (K1+K2)
    int another_faildisk_parameter = (fail_disk_no == fail_disk_no1) ? gfParameter[fail_disk_no2] : gfParameter[fail_disk_no1];
    for(int i = 0; i < SLIDESIZE; i++) {
        restore_data[i] = data_Q[i];
        for(int j = 0; j < disklist.size() - 2; j++) {
            restore_data[i] ^= gf_mul(data_ptr[SLIDESIZE * j + i], gfParameter[diskNo[j]]);
            restore_data[i] ^= gf_mul(data_ptr[SLIDESIZE * j + i], gfParameter[another_faildisk_parameter]);
        }
        restore_data[i] ^= gf_mul(data_P[i], another_faildisk_parameter);
        restore_data[i] = gf_div(restore_data[i], gfParameter[fail_disk_no1] ^ gfParameter[fail_disk_no2]);
    }

    free(data_ptr);
    free(data_P);
    free(data_Q);

    return SUCCESS;
}
