#include "free_map.hh"

#include "filesys/file_header.hh"
#include "lib/assert.hh"

FreeMap::FreeMap() {
    valid = false;
    dirty = false;
    lock = new Lock();
    fileHeader = new FileHeader;
    rwLock = new RWLock();
    file = new OpenFile(FREE_MAP_SECTOR, rwLock, fileHeader);
    bitmap = new Bitmap(NUM_SECTORS);
}

void FreeMap::Format(Directory *directory) {
    ASSERT(!valid);

    Begin();

    Mark(FREE_MAP_SECTOR);
    Mark(DIRECTORY_SECTOR);

    fileHeader->Allocate(bitmap, FREE_MAP_FILE_SIZE);
    fileHeader->WriteBack(FREE_MAP_SECTOR);

    directory->Initialize(bitmap);

    valid = true;

    Commit();
}

void FreeMap::Load() {
    if (valid) return;

    Begin();

    fileHeader->FetchFrom(FREE_MAP_SECTOR);
    bitmap->FetchFrom(file);

    valid = true;

    Commit();
}

FreeMap::~FreeMap() {
    delete lock;
    delete fileHeader;
    delete rwLock;
    delete file;
    delete bitmap;
}

void FreeMap::Begin() {
    ASSERT(!lock->IsHeldByCurrentThread());

    lock->Acquire();
}

void FreeMap::Commit() {
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(valid);

    if (dirty) bitmap->WriteBack(file);

    dirty = false;

    lock->Release();
}

void FreeMap::Abort() {
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(valid);

    if (dirty) bitmap->FetchFrom(file);

    dirty = false;

    lock->Release();
}

unsigned FreeMap::Find() {
    ASSERT(valid);
    ASSERT(lock->IsHeldByCurrentThread());

    int frame = bitmap->Find();
    if (frame != -1) dirty = true;

    return frame;
}

void FreeMap::Mark(unsigned which) {
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(!bitmap->Test(which));

    bitmap->Mark(which);
    dirty = true;
}

bool FreeMap::Test(unsigned which) const {
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(valid);

    return bitmap->Test(which);
}

void FreeMap::Clear(unsigned which) {
    ASSERT(valid);
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(bitmap->Test(which));

    bitmap->Clear(which);

    dirty = true;
}

unsigned FreeMap::CountClear() const {
    ASSERT(valid);
    ASSERT(lock->IsHeldByCurrentThread());

    return bitmap->CountClear();
}

void FreeMap::Print() const {
    ASSERT(valid);
    ASSERT(lock->IsHeldByCurrentThread());

    fileHeader->Print("FreeMap");
    bitmap->Print();
}

bool FreeMap::Allocate(FileHeader *hdr, unsigned size) {
    ASSERT(valid);
    ASSERT(lock->IsHeldByCurrentThread());

    dirty = true;

    return hdr->Allocate(bitmap, size);
}

void FreeMap::Deallocate(FileHeader *hdr) {
    ASSERT(valid);
    ASSERT(lock->IsHeldByCurrentThread());

    dirty = true;

    hdr->Deallocate(bitmap);
}
