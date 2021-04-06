#ifndef RAID5_H
#define RAID5_H

#include "RAID.h"
#include <algorithm>

#define  SLIDESIZZE  256

class RAID5 : public RAID
{
public:
	RAID5();
	virtual ~RAID5();
	virtual int initRAID();
	int getstate();
	int check_raid_state();
	virtual size_t read(int offset, void *buf, size_t count);
	virtual size_t write(int offset, void *buf, size_t count);

    int rebuildRAID();
	int addressMapping(int offset, size_t len);
	int write_check_sum_disk(list<DISKADDR> & addrlist);
	int alloc_read_disk_data(size_t size,char * &ptr);
	int read_layer_data(int layno,char *data_ptr,int except_disk_no=-1);
    int read_all_data(char *all_data);
	int get_data_check_sum(char *data_ptr,int data_disk_num,char *check_sum);
	void calculate_check_sum(char *char_list,int data_disk_num,char &check_sum);
	int write_check_sum(int layerno,char *check_sum);
	int raid_disk_restore(DISKADDR *file_disk_addr,char *restore_data);

protected:
	int slidesize;			 /*the size of block*/
	list<DISKADDR> diskaddrlist;

	Disk *getDisk(int diskno);
	size_t operate(int offset, void *buf, size_t count,int opflag);
	size_t operatedisk( void *buf,int opflag,list<DISKADDR>& diskaddrlist);



private:

};


#endif //SIMRAID_RAID5_H
