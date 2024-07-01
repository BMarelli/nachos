#include "open_file_manager.hh"

OpenFileManager::~OpenFileManager() {
    for (auto &openFile : openFiles) delete openFile.second.rwLock;
}

bool OpenFileManager::IsManaged(unsigned sector) { return openFiles.find(sector) != openFiles.end(); }

void OpenFileManager::Manage(unsigned sector, unsigned referenceCount, RWLock *rwLock) {
    ASSERT(!IsManaged(sector));

    openFiles[sector] = {referenceCount, rwLock};
}

void OpenFileManager::Unmanage(unsigned sector) {
    ASSERT(IsManaged(sector));

    delete openFiles[sector].rwLock;

    openFiles.erase(sector);
}

unsigned OpenFileManager::GetReferenceCount(unsigned sector) {
    auto it = openFiles.find(sector);

    if (it == openFiles.end()) return 0;

    return it->second.referenceCount;
}

unsigned OpenFileManager::IncrementReferenceCount(unsigned sector) {
    ASSERT(IsManaged(sector));

    openFiles[sector].referenceCount++;

    return openFiles[sector].referenceCount;
}

unsigned OpenFileManager::DecrementReferenceCount(unsigned sector) {
    ASSERT(IsManaged(sector));

    openFiles[sector].referenceCount--;

    return openFiles[sector].referenceCount;
}

RWLock *OpenFileManager::GetRWLock(unsigned sector) {
    ASSERT(IsManaged(sector));

    return openFiles[sector].rwLock;
}
