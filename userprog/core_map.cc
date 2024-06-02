#include "core_map.hh"

#include "mmu.hh"

CoreMap::CoreMap() {
    bitmap = new Bitmap(NUM_PHYS_PAGES);
    entries = new CoreMapEntry[NUM_PHYS_PAGES];
    for (unsigned i = 0; i < NUM_PHYS_PAGES; i++) {
        entries[i].space = nullptr;
        entries[i].vpn = -1;
    }
}

CoreMap::~CoreMap() {
    delete bitmap;
    delete[] entries;
}

int CoreMap::Find(AddressSpace *space, unsigned vpn) {
    int frame = bitmap->Find();
    if (frame != -1) {
        entries[frame].space = space;
        entries[frame].vpn = vpn;
    }

    return frame;
}

unsigned CoreMap::CountClear() const { return bitmap->CountClear(); }

void CoreMap::Mark(unsigned frame, AddressSpace *space, unsigned vpn) {
    bitmap->Mark(frame);
    entries[frame].space = space;
    entries[frame].vpn = vpn;
}

void CoreMap::Clear(unsigned frame) {
    bitmap->Clear(frame);
    entries[frame].space = nullptr;
    entries[frame].vpn = -1;
}

bool CoreMap::Test(unsigned frame) const { return bitmap->Test(frame); }

AddressSpace *CoreMap::GetSpace(unsigned frame) const { return entries[frame].space; }

unsigned CoreMap::GetVPN(unsigned frame) const { return entries[frame].vpn; }
