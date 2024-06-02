#ifndef NACHOS_USERPROG_CORE_MAP_HH
#define NACHOS_USERPROG_CORE_MAP_HH

#include "address_space.hh"
#include "lib/bitmap.hh"

class CoreMap {
   public:
    CoreMap();
    ~CoreMap();

    int Find(AddressSpace *space, unsigned vpn);
    unsigned CountClear() const;
    void Mark(unsigned frame, AddressSpace *space, unsigned vpn);
    void Clear(unsigned frame);
    bool Test(unsigned frame) const;

    AddressSpace *GetSpace(unsigned frame) const;
    unsigned GetVPN(unsigned frame) const;

   private:
    struct CoreMapEntry {
        AddressSpace *space;
        unsigned vpn;
    };

    Bitmap *bitmap;
    CoreMapEntry *entries;
};

#endif
