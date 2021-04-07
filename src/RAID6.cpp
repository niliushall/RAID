#include<stdio.h>
#include "RAID.h"
//#include <bits/stl_algo.h>
#include <string.h>
#include "RAID6.h"


int getstate();

RAID6::RAID6() : slidesize(SLIDESIZZE) {
    name = "RAID6";
}

RAID6::~RAID6() {

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
    cout << "wl-RAID6::write" << endl;
//    Sleep(10000);
    return operate(offset, buf, count, OP_WRITE);
}

/*
* offset [in] offset of raid5 data(not include checksum data)
*	len		 [in] length of data you want to write or read
*/
int RAID6::addressMapping(int offset, size_t len) {
    cout << "wl-RAID6::addressMapping" << endl;
//    Sleep(5000);
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
    list<Disk *>::iterator iter = disklist.begin();
    for (int i = 0; i < diskno; i++)
        iter++;
    return *iter;
}

size_t RAID6::operate(int offset, void *buf, size_t count, int opflag) {
    cout << "wl-RAID6::operate" << endl;
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
    cout << "wl-RAID6::operatedisk" << endl;
    int finished = 0;
    void *dataptr = buf;
    if (FAILED == check_raid_state())  /*RAID6 state check */
    {
        cout << "RAID6 current stat is FAILED" << endl;
        return -1;
    }

    cout << "wl-opflag : " << opflag << endl;
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
                    /*                  for(int i=0;i<ptr->len;i++)
                                  {
                                      printf("%X  ",((char*)dataptr)[i]);
                                  }
                                  cout<<endl;*/
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
                if (fail_disk_no == disklist.size() - 1 || fail_disk_no == disklist.size() - 2) {
                    list<DISKADDR>::iterator iter;
                    for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                        DISKADDR *ptr = &(*iter);
                        if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;
                            dataptr = (char *) dataptr + ptr->len;
                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;
                        }
                    }
                    return finished;

                } else {
                    list<DISKADDR>::iterator iter;
                    for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                        DISKADDR *ptr = &(*iter);
                        if (ptr->diskno == fail_disk_no)  /*如果存取数据的盘坏掉，需要从其他盘来恢复*/
                        {

                            char restore_data[SLIDESIZZE];
                            memset(restore_data, 0, SLIDESIZZE);
                            if (SUCCESS != raid_disk_restore(ptr, restore_data)) {
                                return -1;
                            } else {
                                memmove(dataptr, restore_data + (ptr->offset % SLIDESIZZE), ptr->len);
                                finished += ptr->len;

                                cout << "restore data   disk(" << ptr->diskno << ") layer(" << ptr->layerno
                                     << ") offset(" << ptr->offset << "),count(" << ptr->len << ")!" << endl;

                                dataptr = (char *) dataptr + ptr->len;
                            }
                        } else if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;

                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;

                            dataptr = (char *) dataptr + ptr->len;
                        }
                        //system("pause");
                    }
                    return finished;
                }

            } else {
                int fail_disk_no1 = fail_disks.at(0);
                int fail_disk_no2 = fail_disks.at(1);
                if ((fail_disk_no1 == disklist.size() - 2) && (fail_disk_no2 == disklist.size() - 1)) {
                    list<DISKADDR>::iterator iter;
                    for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                        DISKADDR *ptr = &(*iter);
                        if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;
                            dataptr = (char *) dataptr + ptr->len;
                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;
                        }
                    }
                    return finished;
                } else if (fail_disk_no2 == disklist.size() - 1) {
                    int fail_disk_no = fail_disk_no1;
                    list<DISKADDR>::iterator iter;
                    for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                        DISKADDR *ptr = &(*iter);
                        if (ptr->diskno == fail_disk_no)  /*如果存取数据的盘坏掉，需要从其他盘来恢复*/
                        {
                            char restore_data[SLIDESIZZE];
                            memset(restore_data, 0, SLIDESIZZE);
                            if (SUCCESS != raid_disk_restore(ptr, restore_data)) {
                                return -1;
                            } else {
                                memmove(dataptr, restore_data + (ptr->offset % SLIDESIZZE), ptr->len);
                                finished += ptr->len;
                                cout << "restore data   disk(" << ptr->diskno << ") layer(" << ptr->layerno
                                     << ") offset(" << ptr->offset << "),count(" << ptr->len << ")!" << endl;

                                dataptr = (char *) dataptr + ptr->len;
                            }
                        } else if ((size_t) getDisk(ptr->diskno)->read(ptr->offset, dataptr, ptr->len) != ptr->len) {
                            return -1;
                        } else {
                            finished += ptr->len;
                            cout << "Read   disk(" << ptr->diskno << ") offset(" << ptr->offset << "),count("
                                 << ptr->len << ")!" << endl;

                            dataptr = (char *) dataptr + ptr->len;
                        }
                        //system("pause");
                    }
                    return finished;
                } else             //当坏掉的盘中不包括第二个数据盘的时候的处理情况
                {
                    int ret = SUCCESS;
                    int fail_disk_count = 0;
                    int except_disk_no = 0;
                    bool *layer_done = new bool[disklist.size()];
                    char *data_ptr = NULL;
                    char check_sum[SLIDESIZZE];

                    /*先将所有的数据盘中的数据全部恢复至数据盘
                     *再将数据从数据盘中读出
                     *下面的循环就是恢复过程
                     *首先用斜校验恢复一个层中的其中一个坏盘的数据，另外一个用横校验恢复
                     *直至恢复所有数据到数据盘*/
                    for (size_t layer_no = 0;
                         layer_no < DISK_SIZE / SLIDESIZZE; layer_no = layer_no + disklist.size() - 2) {
                        flag_init(layer_done);
                        while (all_data_restored(layer_done) == false) {
                            for (int i = 0; i < disklist.size() - 2; i++) {
                                fail_disk_count = count_known_datas(layer_done, i, layer_no, fail_disk_no1,
                                                                    fail_disk_no2, except_disk_no);
                                if (fail_disk_count == 2) {
                                } else if (fail_disk_count == 1) {
                                    ret = alloc_read_disk_data((size_t) (disklist.size() - 2) * SLIDESIZZE, data_ptr);

                                    ret = read_layer_data_for_inclined_check(layer_no, i, data_ptr, except_disk_no);
                                    ret = get_data_check_sum(data_ptr, (int) disklist.size() - 2, check_sum);
                                    ret = write_check_sum_for_inclined_check(layer_no + i, except_disk_no, check_sum);

                                    //ret = alloc_read_disk_data((size_t) (disklist.size() - 2) * SLIDESIZZE, data_ptr);
                                    if (except_disk_no == fail_disk_no1) {
                                        ret = read_layer_data(layer_no + i, data_ptr,
                                                              fail_disk_no2);    /*read one layer data*/
                                        ret = get_data_check_sum(data_ptr, (int) disklist.size() - 2, check_sum);
                                        ret = write_check_sum(layer_no + i, check_sum);
                                    } else if (except_disk_no == fail_disk_no2) {
                                        ret = read_layer_data(layer_no + i, data_ptr,
                                                              fail_disk_no1);    /*read one layer data*/
                                        ret = get_data_check_sum(data_ptr, (int) disklist.size() - 2, check_sum);
                                        ret = write_check_sum(layer_no + i, check_sum);
                                    }
                                    layer_done[i] = true;
                                    if (data_ptr) {
                                        free(data_ptr);
                                    }

                                }

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
                            dataptr = (char *) dataptr + ptr->len;
                        }
                    }

                }
            }
        }

        case OP_WRITE: {
            list<DISKADDR>::iterator iter;
            for (iter = diskaddrlist.begin(); iter != diskaddrlist.end(); iter++) {
                getDisk(iter->diskno)->write(iter->offset, dataptr, iter->len);
                finished += iter->len;
                dataptr = (char *) dataptr + iter->len;
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

                cout << endl;
                finished += iter->len;
                dataptr = (char *) dataptr + iter->len;

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
    //std::vector<int> influenced_layers;
    //list<DISKADDR>::iterator iter;
    //for(iter=addrlist.begin();iter!=addrlist.end();iter++)
    //{
    //    influenced_layers.push_back(iter->layerno);
    //}
    //vector<int>::iterator new_end;
    //new_end = unique(influenced_layers.begin(), influenced_layers.end());    //"删除"相邻的重复元素
    //influenced_layers.erase(new_end, influenced_layers.end());  //删除（真正的删除）重复的元素
    char *data_ptr = NULL;
    char check_sum[SLIDESIZZE];
    if (SUCCESS != (ret = alloc_read_disk_data((size_t) (disklist.size() - 2) * SLIDESIZZE, data_ptr))) {
    } else {
        int check_sum_disk_no = disklist.size() - 1;
        for (size_t layers_iter = 0; SUCCESS == ret && layers_iter < DISK_SIZE / SLIDESIZZE; layers_iter++) {
            //int check_sum_disk_no = disklist.size()-2;
            //cout<<"layers_iter = "<<layers_iter<<endl;
            ret = read_layer_data_for_checkP(layers_iter, data_ptr);

            ret = get_data_check_sum(data_ptr, (int) disklist.size() - 2, check_sum);
            ret = write_check_sum(layers_iter, check_sum);
            //if(SUCCESS!=(ret=read_layer_data_for_checkP(layers_iter,data_ptr)))		/*read one layer data for check1*/
            //{
            //}

            /*get data check sum of one layer*/
            //else if(SUCCESS!=(ret=get_data_check_sum(data_ptr,(int)disklist.size()-2,check_sum)))
            //{
            //}
            //else if(SUCCESS!=(ret=write_check_sum(layers_iter,check_sum))) 	/*write one check sum block*/
            //{
            //}
        }
        for (size_t layers_iter = 0; SUCCESS == ret && layers_iter < DISK_SIZE / SLIDESIZZE; layers_iter++) {
            if (SUCCESS != (ret = read_layer_data_for_checkQ(layers_iter, data_ptr, check_sum_disk_no))) {
            } else if (SUCCESS != (ret = get_data_check_sum(data_ptr, (int) disklist.size() - 2, check_sum))) {
            } else if (SUCCESS != (ret = write_check_sum(layers_iter, check_sum)))    /*write one check sum block*/
            {
            }
        }
    }
    if (data_ptr != NULL) {
        free(data_ptr);
    }
    return ret;
}

int RAID6::alloc_read_disk_data(size_t size, char *&ptr) {
    int ret = SUCCESS;
    ptr = NULL;
    ptr = (char *) malloc(size);
    if (NULL == ptr) {
        ret = ERROR;
    }
    return ret;
}

/*@berif 读取layno层所有磁盘（不包括except_disk_no）内容,layno 层的所有的数据 到ptr中
* layno  [in]   需要读取数据block 层
* except_disk_no  [in]  该层中不需要读取的disk_no, -1 表示 该层所有的磁盘都需要读取数据
* */

int RAID6::read_layer_data(int layno, char *data_ptr, int except_disk_no) {
    int ret = SUCCESS;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    for (int i = 0; i < disklist.size() - 1; i++) {
        if (except_disk_no != i) {
            disk_addr.layerno = layno;
            disk_addr.diskno = i;
            disk_addr.len = SLIDESIZZE;
            disk_addr.offset = SLIDESIZZE * layno;
            disk_addr_list.push_back(disk_addr);
        }
    }
    int data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if (data_size != SLIDESIZZE * ((disklist.size() - 2))) {
        ret = ERROR;
    }
    return ret;
}

int RAID6::read_layer_data_for_inclined_check(int layno, int disk_no, char *data_ptr, int except_disk_no) {
    int ret = SUCCESS;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    int temp_disk_no = disk_no;
    for (int i = 0; i < disklist.size() - 2; i++) {
        if (except_disk_no != temp_disk_no) {
            disk_addr.layerno = layno + i;
            disk_addr.diskno = temp_disk_no;
            disk_addr.len = SLIDESIZZE;
            disk_addr.offset = SLIDESIZZE * disk_addr.layerno;
            disk_addr_list.push_back(disk_addr);
        }
        temp_disk_no = (temp_disk_no + 1) % (disklist.size() - 1);

    }
    disk_addr.layerno = layno;
    disk_addr.diskno = disk_no;
    disk_addr.len = SLIDESIZZE;
    disk_addr.offset = SLIDESIZZE * layno;
    disk_addr_list.push_back(disk_addr);

    int data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if (data_size != SLIDESIZZE * ((disklist.size() - 2))) {
        ret = ERROR;
    }
    return ret;

}

//生成校验盘P
int RAID6::read_layer_data_for_checkP(int layno, char *data_ptr) {
    int ret = SUCCESS;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    //int except_disk_no1=except_disk_no;
    //int except_disk_no2=except_disk_no+1;
    for (int i = 0; i < disklist.size() - 2; i++) {
        //if((except_disk_no1 != i)&&(except_disk_no2 != i))
        //{
        disk_addr.layerno = layno;
        disk_addr.diskno = i;
        disk_addr.len = SLIDESIZZE;
        disk_addr.offset = SLIDESIZZE * layno;
        disk_addr_list.push_back(disk_addr);
        //}
    }
    int data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
/*    for(int i=0;i<SLIDESIZZE*2;i++)
    {
        printf("%X  ",((char*)data_ptr)[i]);
    }
    cout<<endl;
    system("pause");*/
    if (data_size != SLIDESIZZE * ((disklist.size() - 2))) {
        ret = ERROR;
    }
    return ret;
}

//生成校验盘Q
int RAID6::read_layer_data_for_checkQ(int layno, char *data_ptr, int except_disk_no) {
    int ret = SUCCESS;
    int layer_no;
    int temp_diskno;
    list<DISKADDR> disk_addr_list;
    DISKADDR disk_addr;
    for (int i = 0; i < disklist.size() - 2; i++) {
        disk_addr.layerno = layno / disklist.size() * disklist.size() + i;
        layer_no = disk_addr.layerno;
        temp_diskno = layno % (disklist.size() - 1) + i;
        if (except_disk_no == temp_diskno)
            disk_addr.diskno = 0;
        else
            disk_addr.diskno = temp_diskno;
        disk_addr.offset = layer_no * SLIDESIZZE;
        disk_addr_list.push_back(disk_addr);
    }
    int data_size = operatedisk(data_ptr, OP_READ_REBUILD, disk_addr_list);
    if (data_size != SLIDESIZZE * ((disklist.size() - 2))) {
        ret = ERROR;
    }
    return ret;
}

/*
* @berif  根据data_ptr 生成check_sum 奇校验码
*  data_ptr  [in]   数据盘同一层block的全部的数据量
*  data_disk_num  [in]  数据盘数量，不包括校验，相当于 全部盘数量减2
*  check_sum  [out]  用于表示data_ptr生成的校验block
*/
int RAID6::get_data_check_sum(char *data_ptr, int data_disk_num, char *check_sum) {
    int ret = SUCCESS;
    char *char_check_sum = NULL;
    if (NULL == (char_check_sum = (char *) malloc(data_disk_num * sizeof(char)))) {
        ret = ERROR;
    }
    for (int char_index = 0; SUCCESS == ret && char_index < SLIDESIZZE; char_index++) {
        for (int k = 0; SUCCESS == ret && k < data_disk_num; k++) {
            char_check_sum[k] = data_ptr[k * SLIDESIZZE + char_index];
        }
        calculate_check_sum(char_check_sum, data_disk_num, check_sum[char_index]);//char_check_sum 当前的原始数据
    }
    if (char_check_sum) {
        free(char_check_sum);
    }
    return ret;
}

/*@berif 根据char_list 字符list，奇校验生成char的校验码 check_sum*/
void RAID6::calculate_check_sum(char *char_list, int data_disk_num, char &check_sum) {
    check_sum = 0;
    bool flag = false;
    for (char i = 0; i < 8; i++)/*使用奇校验*/
    {
        flag = false;
        char check = 1 << i;
        for (int k = 0; k < data_disk_num; k++) {
            if (check & char_list[k]) {
                flag = !flag;
            }
        }
        if (flag)/*如果1的数量为奇数个，flag 为true*/
        {
            check_sum = check_sum | check;
        }
    }
}

/*write one  check_sum  block to check_sum_disk*/
int RAID6::write_check_sum(int layerno, char *check_sum) {
    int ret = SUCCESS;
    int check_sum_disk_no = (int) disklist.size() - 2;
    list<DISKADDR> addrlist;
    DISKADDR diskaddr;
    diskaddr.layerno = layerno;
    diskaddr.diskno = check_sum_disk_no;
    diskaddr.len = SLIDESIZZE;
    diskaddr.offset = (size_t) layerno * SLIDESIZZE;
    addrlist.push_back(diskaddr);
    if (SLIDESIZZE != operatedisk(check_sum, OP_WRITE_CHECK_SUM, addrlist)) {
        ret = ERROR;
    }
    return ret;
}

int RAID6::write_check_sum_for_inclined_check(int layerno, int disk_no, char *check_sum) {
    int ret = SUCCESS;
    list<DISKADDR> addrlist;
    DISKADDR diskaddr;
    diskaddr.layerno = layerno;
    diskaddr.diskno = disk_no;
    diskaddr.len = SLIDESIZZE;
    diskaddr.offset = (size_t) layerno * SLIDESIZZE;
    addrlist.push_back(diskaddr);
    if (SLIDESIZZE != operatedisk(check_sum, OP_WRITE_CHECK_SUM, addrlist)) {
        ret = ERROR;
    }
    return ret;
}

int RAID6::raid_disk_restore(DISKADDR *file_disk_addr, char *restore_data) {
    int ret = SUCCESS;
    int layer_no = file_disk_addr->layerno;
    int fail_disk_no = file_disk_addr->diskno;
    char *data_ptr = NULL;
    if (SUCCESS != (ret = alloc_read_disk_data((size_t) (disklist.size() - 2) * SLIDESIZZE, data_ptr))) {
    }
    if (SUCCESS != (ret = read_layer_data(layer_no, data_ptr, fail_disk_no)))    /*read one layer data*/
    {
    } else if (SUCCESS != (ret = get_data_check_sum(data_ptr, (int) disklist.size() - 2, restore_data))) {
    }
    if (data_ptr) {
        free(data_ptr);
    }
    return ret;
}
