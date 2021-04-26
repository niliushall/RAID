#ifndef RAID6_H
#define RAID6_H

#include "RAID.h"

#define  SLIDESIZE  256
class RAID6 : public RAID {
public:
    RAID6();
    ~RAID6();
    void generate_GF_table();
    int gf_mul(int a, int b);  // multiplication of Galois Field
    int gf_div(int a, int b);  // division of Galois Field
    uint8_t gf_pow(uint8_t exp);
    uint8_t gf_miv(uint8_t i);
    int initRAID() override;
    int check_raid_state() override;
    int getstate() override;
    size_t read(int offset, void *buf, size_t count) override;
    size_t write(int offset, void *buf, size_t count) override;

    int addressMapping(int offset, size_t count);
    int write_check_sum_disk(list<DISKADDR> & addrlist);
    int alloc_read_disk_data(size_t size,uint8_t * &ptr);
    int read_layer_data(int layno, uint8_t *data_ptr,int except_disk_no=-1);
    int read_layer_data_for_inclined_check(int layno,int disk_no,uint8_t *data_ptr,int except_disk_no);

    int read_layer_data_for_check(int layno,uint8_t *data_ptr);
    // int read_layer_data_for_checkQ(int layno, char *data_ptr,int except_disk_no=-1);
    int get_data_check_sum(const uint8_t *data_ptr,int data_disk_num,uint8_t* check_sum_P, uint8_t* check_sum_Q);
    static void calculate_check_sum_P(const uint8_t *char_list,int data_disk_num,uint8_t &check_sum);
    void calculate_check_sum_Q(const uint8_t *char_list,int data_disk_num,uint8_t &check_sum);
    int write_check_sum(int layerno, int diskno, uint8_t *check_sum);
    int write_check_sum_for_inclined_check(int layerno,int disk_no, uint8_t *check_sum);

    void flag_init(bool *flag);

    int raid_disk_restore_by_P(DISKADDR *fail_disk_addr, uint8_t *restore_data);
    int raid_disk_restore_by_Q(DISKADDR *fail_disk_addr, uint8_t *restore_data);
    int raid_disk_restore_by_P_Q(DISKADDR *fail_disk_addr, uint8_t *restore_data, int fail_disk_no1, int fail_disk_no2);

    int all_data_restored(bool *flag);
    int count_known_datas(bool *falg,int i,int _layer,int fail_disk_no1,int fail_disk_no2,int except_disk_no);

protected:
    int slidesize;
    list<DISKADDR> diskaddrlist;

    Disk *getDisk(int diskno);

    size_t operate(int offset, void *buf, size_t count,int opflag);
    size_t operatedisk( void *buf,int opflag,list<DISKADDR>& diskaddrlist);

private:
    int GFLOG[256];     // the positive logarithm table of Galois Field
    int GFILOG[256];    // the antilog table of Galois Field
    vector<int> gfParameter;
};

#endif // RAID6_H

