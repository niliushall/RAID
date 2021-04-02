#ifndef RAID_H
#define RAID_H

#include "Storage.h"
#include "Disk.h"

enum RAIDSTATE
{
 STARTFLAG,NOTREADY, READY, FAILED, DEGRAGE,ENDFLAG
};

#define RAIDSTATE_STARTFLAG   0
#define RAIDSTATE_NOTREADY    1
#define RAIDSTATE_READY       1
#define RAIDSTATE_FAILED      2
#define RAIDSTATE_DEGRAGE     3
#define RAIDSTATE_ENDFLAG     4


class Disk;

typedef struct {
    int diskno;
	int layerno;
    size_t offset;
    size_t len;

} DISKADDR;

class RAID : public Storage
{
    public:
        RAID();
        virtual ~RAID();
        virtual bool isvalidstate(int s);
	    virtual int getstate();

        int addDisk(Disk *d);
        int removeDisk(Disk *d);
        int replaceDisk(unsigned int no,Disk *d);
        int replaceDisk(Disk *olddisk,Disk *newdisk);

        Disk *getDisk(unsigned int pos);
        int getPosition(Disk *disk);
        virtual int initialize() {return RAIDSTATE_NOTREADY;}

        virtual int initRAID()=0;
		virtual  void pritinfo();
		virtual  int check_raid_state();

        int rebuildRAID();

        int printDiskList();

		void set_name(string raidname)	{ name=raidname;}
		int set_rand_disk_hung();
        int set_rand_disk_hung1();
		int get_disk_size();



    protected:
        static int count;
        list<Disk *> disklist;   /*rRAID disk list */
		vector<int> fail_disks;   /* state is failed  disk id list*/
};

#endif // RAID_H
