#include "cached_synch_disk.hh"

#include <cstring>

// #define FILESYS_DISK_CACHE_READ_AHEAD 1
// #define FILESYS_DISK_CACHE_WRITE_BEHIND 1

CachedSynchDisk::CachedSynchDisk(const char *name) : SynchDisk(name) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache[i].valid = false;
        cache[i].dirty = false;
    }

    clock = 0;
    lock = new Lock();
}

CachedSynchDisk::~CachedSynchDisk() {
    Flush();

    delete lock;
}

void CachedSynchDisk::ReadSector(int sectorNumber, char *data) {
    lock->Acquire();

    if (ReadCache(sectorNumber, data)) {
        lock->Release();

        return;
    }

    SynchDisk::ReadSector(sectorNumber, data);
    Cache(sectorNumber, data, false);

#ifdef FILESYS_DISK_CACHE_READ_AHEAD
    if (sectorNumber + 1 < NUM_SECTORS) {
        char readAheadData[SECTOR_SIZE];
        SynchDisk::ReadSector(sectorNumber + 1, readAheadData);  // not blocking
        Cache(sectorNumber + 1, readAheadData, false);
    }
#endif

    lock->Release();
}

void CachedSynchDisk::WriteSector(int sectorNumber, const char *data) {
    lock->Acquire();

#ifdef FILESYS_DISK_CACHE_WRITE_BEHIND
    Cache(sectorNumber, data, true);
#else
    SynchDisk::WriteSector(sectorNumber, data);
    Cache(sectorNumber, data, false);
#endif

    lock->Release();
}

bool CachedSynchDisk::ReadCache(unsigned sector, char *data) {
    for (unsigned i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].sector == sector) {
            memcpy(data, cache[i].data, SECTOR_SIZE);
            cache[i].lastUsed = clock++;

            return true;
        }
    }

    return false;
}

void CachedSynchDisk::Cache(unsigned sector, const char *data, bool dirty) {
    unsigned victim = FindVictim(sector);

    cache[victim].sector = sector;
    memcpy(cache[victim].data, data, SECTOR_SIZE);
    cache[victim].valid = true;
    cache[victim].dirty = dirty;
    cache[victim].lastUsed = clock++;
}

void CachedSynchDisk::Flush() {
    for (unsigned i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].dirty) {
            SynchDisk::WriteSector(cache[i].sector, cache[i].data);

            cache[i].dirty = false;
        }
    }
}

unsigned CachedSynchDisk::FindVictim(unsigned sector) {
    unsigned lru = 0;
    for (unsigned i = 1; i < CACHE_SIZE; i++) {
        if (!cache[i].valid || cache[i].sector == sector) return i;

        if (cache[i].lastUsed < cache[lru].lastUsed) lru = i;
    }

    if (cache[lru].dirty) SynchDisk::WriteSector(cache[lru].sector, cache[lru].data);

    return lru;
}
