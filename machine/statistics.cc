/// Routines for managing statistics about Nachos performance.
///
/// DO NOT CHANGE -- these stats are maintained by the machine emulation.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "statistics.hh"

#include <stdio.h>

/// Initialize performance metrics to zero, at system startup.
Statistics::Statistics() {
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;

#ifdef USE_TLB
    numTlbHits = numTlbMisses = 0;  // Initialize TLB hit and miss counters
#endif

#ifdef SWAP
    numPagesSentToSwap = numPagesLoadedFromSwap = 0;
#endif

#ifdef DFS_TICKS_FIX
    tickResets = 0;
#endif
}

/// Print performance metrics, when we have finished everything at system
/// shutdown.
void Statistics::Print() {
#ifdef DFS_TICKS_FIX
    if (tickResets != 0) {
        printf(
            "WARNING: the tick counter was reset %lu times; the following"
            " statistics may be invalid.\n\n",
            tickResets);
    }
#endif
    printf("Ticks: total %lu, idle %lu, system %lu, user %lu\n", totalTicks, idleTicks, systemTicks, userTicks);
    printf("Disk I/O: reads %lu, writes %lu\n", numDiskReads, numDiskWrites);
    printf("Console I/O: reads %lu, writes %lu\n", numConsoleCharsRead, numConsoleCharsWritten);
    printf("Paging: faults %lu\n", numPageFaults);
#ifdef USE_TLB
    if (numTlbHits + numTlbMisses > 0) {
        // Given that each tlb miss is retried, numTlbHits is the number of true tlb hits
        // (on the first try) plus the number of tlb misses that were retried. Therefore,
        // we need to subtract the number of misses from the total number of tlb hits to
        // get the number of true hits.
        double tlbHitRatio = (double)(numTlbHits - numTlbMisses) / (double)numTlbHits;
        printf("TLB: hit ratio %4.2f%%\n", tlbHitRatio * 100.0);
    }
#endif

#ifdef SWAP
    printf("Swap: pages sent to swap %lu, pages loaded from swap %lu\n", numPagesSentToSwap, numPagesLoadedFromSwap);
#endif

    printf("Network I/O: packets received %lu, sent %lu\n", numPacketsRecvd, numPacketsSent);
}
