#ifndef RAID6_H
#define RAID6_H

#include "RAID.h"

#define  SLIDESIZE  256
class RAID6 : public RAID
{
    public:
        RAID6();
        virtual ~RAID6();
        virtual int initRAID();
        int check_raid_state();
        int getstate();
        virtual size_t read(int offset, void *buf, size_t count);
        virtual size_t write(int offset, void *buf, size_t count);

        int addressMapping(int offset, size_t count);
        int write_check_sum_disk(list<DISKADDR> & addrlist);
        int alloc_read_disk_data(size_t size,char * &ptr);
        int read_layer_data(int layno, char *data_ptr,int except_disk_no=-1);
        int read_layer_data_for_inclined_check(int layno,int disk_no,char *data_ptr,int except_disk_no);

        int read_layer_data_for_checkP(int layno,char *data_ptr);
        int read_layer_data_for_checkQ(int layno, char *data_ptr,int except_disk_no=-1);
        int get_data_check_sum(char *data_ptr,int data_disk_num,char *check_sum);
        void calculate_check_sum(char *char_list,int data_disk_num,char &check_sum);
        int write_check_sum(int layerno,char *check_sum);
        int write_check_sum_for_inclined_check(int layerno,int disk_no, char *check_sum);

        void flag_init(bool *flag);

        int raid_disk_restore(DISKADDR *file_disk_addr,char *restore_data);

        int all_data_restored(bool *flag);
        int count_known_datas(bool *falg,int i,int _layer,int fail_disk_no1,int fail_disk_no2,int except_disk_no);

    protected:
        int slidesize;
        list<DISKADDR> diskaddrlist;

        Disk *getDisk(int diskno);

        size_t operate(int offset, void *buf, size_t count,int opflag);
        size_t operatedisk( void *buf,int opflag,list<DISKADDR>& diskaddrlist);
};


#endif // RAID6_H

