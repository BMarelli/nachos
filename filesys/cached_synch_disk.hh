#ifndef NACHOS_FILESYS_CACHED_SYNCH_DISK__HH
#define NACHOS_FILESYS_CACHED_SYNCH_DISK__HH

#include "synch_disk.hh"

class CachedSynchDisk : public SynchDisk {
   public:
    CachedSynchDisk(const char *name);
    ~CachedSynchDisk() override;

    void ReadSector(int sectorNumber, char *data) override;
    void WriteSector(int sectorNumber, const char *data) override;

   private:
    Lock *lock;

    struct CacheEntry {
        unsigned sector;
        char data[SECTOR_SIZE];
        bool dirty;
        bool valid;
        unsigned lastUsed;
    };

    static const int CACHE_SIZE = 64;
    CacheEntry cache[CACHE_SIZE];

    bool ReadCache(unsigned sector, char *data);
    void Cache(unsigned sector, const char *data, bool dirty);
    void Flush();

    unsigned FindVictim(unsigned sector);

    unsigned clock;
};

#endif
