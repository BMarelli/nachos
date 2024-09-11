#include "thread_test_rwlock.hh"

#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <vector>

#include "rwlock.hh"
#include "system.hh"

static RWLock *rwLock;

#define NUM_RUNS 10
#define NUM_THREADS 10
#define MIN_WORK_AMOUNT 1
#define MAX_WORK_AMOUNT 5

enum EventType {
    EVENT_TYPE_THREAD_STARTED = 0,
    EVENT_TYPE_LOCK_ACQUIRED = 1,
    EVENT_TYPE_WORK_STARTED = 2,
    EVENT_TYPE_WORK_PROGRESSED = 3,
    EVENT_TYPE_WORK_FINISHED = 4,
    EVENT_TYPE_LOCK_RELEASED = 5,
    EVENT_TYPE_THREAD_FINISHED = 6,
};

enum ThreadType {
    THREAD_TYPE_READER = 0,
    THREAD_TYPE_WRITER = 1,
    THREAD_TYPE_READER_WRITER = 2,

    NUM_THREAD_TYPES = 3,
};

enum LockType {
    LOCK_TYPE_READ = 0,
    LOCK_TYPE_WRITE = 1,
};

static bool IsReader(ThreadType threadType) { return threadType == THREAD_TYPE_READER || threadType == THREAD_TYPE_READER_WRITER; }

static bool IsWriter(ThreadType threadType) { return threadType == THREAD_TYPE_WRITER || threadType == THREAD_TYPE_READER_WRITER; }

struct Event {
    EventType eventType;
    ThreadType threadType;
    long threadId;
    void *data;
};

static std::vector<Event> eventLog;
static std::vector<Thread *> threads;

static void LogEvent(EventType eventType, ThreadType threadType, long threadId, void *data = nullptr) {
    Event event = {eventType, threadType, threadId, data};
    eventLog.push_back(event);
}

static void DoWork(ThreadType threadType, long id) {
    long workAmount = rand() % (MAX_WORK_AMOUNT - MIN_WORK_AMOUNT + 1) + MIN_WORK_AMOUNT;

    LogEvent(EVENT_TYPE_WORK_STARTED, threadType, id, (void *)workAmount);

    for (long i = 1; i <= workAmount; i++) {
        LogEvent(EVENT_TYPE_WORK_PROGRESSED, threadType, id, (void *)i);
        currentThread->Yield();
    }

    LogEvent(EVENT_TYPE_WORK_FINISHED, threadType, id);
}

static void Worker(void *arg) {
    long id = (long)arg;

    ThreadType threadType;
    switch (rand() % NUM_THREAD_TYPES) {
        case 0:
            threadType = THREAD_TYPE_READER;
            break;
        case 1:
            threadType = THREAD_TYPE_WRITER;
            break;
        case 2:
            threadType = THREAD_TYPE_READER_WRITER;
            break;
    }

    LogEvent(EVENT_TYPE_THREAD_STARTED, threadType, id);

    if (IsWriter(threadType)) {
        rwLock->AcquireWrite();

        LogEvent(EVENT_TYPE_LOCK_ACQUIRED, threadType, id, (void *)LOCK_TYPE_WRITE);
    }

    if (IsReader(threadType)) {
        rwLock->AcquireRead();

        LogEvent(EVENT_TYPE_LOCK_ACQUIRED, threadType, id, (void *)LOCK_TYPE_READ);
    }

    DoWork(threadType, id);

    if (IsReader(threadType)) {
        rwLock->ReleaseRead();

        LogEvent(EVENT_TYPE_LOCK_RELEASED, threadType, id, (void *)LOCK_TYPE_READ);
    }

    if (IsWriter(threadType)) {
        rwLock->ReleaseWrite();

        LogEvent(EVENT_TYPE_LOCK_RELEASED, threadType, id, (void *)LOCK_TYPE_WRITE);
    }

    LogEvent(EVENT_TYPE_THREAD_FINISHED, threadType, id);
}

static void AssertThreadEventsAreValid() {
    std::map<unsigned, std::vector<Event>> eventsByReaderThread;
    std::map<unsigned, std::vector<Event>> eventsByWriterThread;
    std::map<unsigned, std::vector<Event>> eventsByReaderWriterThread;
    for (size_t i = 0; i < eventLog.size(); ++i) {
        const Event &event = eventLog[i];

        switch (event.threadType) {
            case THREAD_TYPE_READER:
                eventsByReaderThread[event.threadId].push_back(event);
                break;
            case THREAD_TYPE_WRITER:
                eventsByWriterThread[event.threadId].push_back(event);
                break;
            case THREAD_TYPE_READER_WRITER:
                eventsByReaderWriterThread[event.threadId].push_back(event);
                break;
            default:
                break;
        }
    }

    for (auto &values : eventsByReaderThread) {
        auto &events = values.second;

        ASSERT(events[0].eventType == EVENT_TYPE_THREAD_STARTED);
        ASSERT(events[1].eventType == EVENT_TYPE_LOCK_ACQUIRED);
        ASSERT(events[2].eventType == EVENT_TYPE_WORK_STARTED);

        const size_t workAmount = (size_t)events[2].data;
        for (size_t i = 3; i < workAmount + 3; i++) {
            ASSERT(events[i].eventType == EVENT_TYPE_WORK_PROGRESSED);
        }

        ASSERT(events[events.size() - 3].eventType == EVENT_TYPE_WORK_FINISHED);
        ASSERT(events[events.size() - 2].eventType == EVENT_TYPE_LOCK_RELEASED);
        ASSERT(events[events.size() - 1].eventType == EVENT_TYPE_THREAD_FINISHED);

        ASSERT(events.size() == workAmount + 6);
    }

    for (auto &values : eventsByWriterThread) {
        auto &events = values.second;

        ASSERT(events[0].eventType == EVENT_TYPE_THREAD_STARTED);
        ASSERT(events[1].eventType == EVENT_TYPE_LOCK_ACQUIRED);
        ASSERT(events[2].eventType == EVENT_TYPE_WORK_STARTED);

        const size_t workAmount = (size_t)events[2].data;
        for (size_t i = 3; i < workAmount + 3; i++) {
            ASSERT(events[i].eventType == EVENT_TYPE_WORK_PROGRESSED);
        }

        ASSERT(events[events.size() - 3].eventType == EVENT_TYPE_WORK_FINISHED);
        ASSERT(events[events.size() - 2].eventType == EVENT_TYPE_LOCK_RELEASED);
        ASSERT(events[events.size() - 1].eventType == EVENT_TYPE_THREAD_FINISHED);

        ASSERT(events.size() == workAmount + 6);
    }

    for (auto &values : eventsByReaderWriterThread) {
        auto &events = values.second;

        ASSERT(events[0].eventType == EVENT_TYPE_THREAD_STARTED);
        ASSERT(events[1].eventType == EVENT_TYPE_LOCK_ACQUIRED && events[1].data == (void *)LOCK_TYPE_WRITE);
        ASSERT(events[2].eventType == EVENT_TYPE_LOCK_ACQUIRED && events[2].data == (void *)LOCK_TYPE_READ);
        ASSERT(events[3].eventType == EVENT_TYPE_WORK_STARTED);

        const size_t workAmount = (size_t)events[3].data;
        for (size_t i = 4; i < workAmount + 4; i++) {
            ASSERT(events[i].eventType == EVENT_TYPE_WORK_PROGRESSED);
        }

        size_t size = events.size();

        ASSERT(events[size - 4].eventType == EVENT_TYPE_WORK_FINISHED);
        ASSERT(events[size - 3].eventType == EVENT_TYPE_LOCK_RELEASED && events[events.size() - 3].data == (void *)LOCK_TYPE_READ);
        ASSERT(events[size - 2].eventType == EVENT_TYPE_LOCK_RELEASED && events[size - 2].data == (void *)LOCK_TYPE_WRITE);
        ASSERT(events[size - 1].eventType == EVENT_TYPE_THREAD_FINISHED);

        ASSERT(events.size() == workAmount + 8);
    }
}

static void AssertReadersAreNotInterruptedByWriters() {
    for (size_t i = 0; i < eventLog.size(); ++i) {
        const Event &event = eventLog[i];

        if (event.eventType != EVENT_TYPE_LOCK_ACQUIRED || event.threadType != THREAD_TYPE_READER) continue;

        for (size_t j = i + 1; j < eventLog.size(); ++j) {
            const Event &nextEvent = eventLog[j];

            if (nextEvent.eventType == EVENT_TYPE_LOCK_RELEASED && nextEvent.threadId == event.threadId) break;

            ASSERT(nextEvent.threadType != THREAD_TYPE_WRITER || nextEvent.eventType == EVENT_TYPE_THREAD_STARTED);
        }
    }
}

static void AssertWritersAreNotInterrupted() {
    for (size_t i = 0; i < eventLog.size(); ++i) {
        const Event &event = eventLog[i];

        if (event.eventType != EVENT_TYPE_LOCK_ACQUIRED || event.threadType != THREAD_TYPE_WRITER) continue;

        for (size_t j = i + 1; j < eventLog.size(); ++j) {
            const Event &nextEvent = eventLog[j];

            if (nextEvent.eventType == EVENT_TYPE_LOCK_RELEASED && nextEvent.threadId == event.threadId) break;

            ASSERT(nextEvent.threadId == event.threadId || nextEvent.eventType == EVENT_TYPE_THREAD_STARTED);
        }
    }
}

static void AssertReaderWritersAreNotInterrupted() {
    for (size_t i = 0; i < eventLog.size(); ++i) {
        const Event &event = eventLog[i];

        if (event.eventType != EVENT_TYPE_LOCK_ACQUIRED || event.threadType != THREAD_TYPE_READER_WRITER) continue;

        for (size_t j = i + 1; j < eventLog.size(); ++j) {
            const Event &nextEvent = eventLog[j];

            if (nextEvent.eventType == EVENT_TYPE_LOCK_RELEASED && nextEvent.data == (void *)LOCK_TYPE_WRITE &&
                nextEvent.threadId == event.threadId)
                break;

            ASSERT(nextEvent.threadId == event.threadId || nextEvent.eventType == EVENT_TYPE_THREAD_STARTED);
        }
    }
}

static void ValidateEvents() {
    AssertThreadEventsAreValid();

    AssertReadersAreNotInterruptedByWriters();
    AssertWritersAreNotInterrupted();
    AssertReaderWritersAreNotInterrupted();
}

static void PrintEvents() {
    for (auto &event : eventLog) {
        const char *eventType;
        switch (event.eventType) {
            case EVENT_TYPE_THREAD_STARTED:
                eventType = "Thread started";
                break;
            case EVENT_TYPE_LOCK_ACQUIRED:
                if (event.data == (void *)LOCK_TYPE_READ) {
                    eventType = "Read lock acquired";
                } else {
                    eventType = "Write lock acquired";
                }
                break;
            case EVENT_TYPE_WORK_STARTED:
                eventType = "Work started";
                break;
            case EVENT_TYPE_WORK_PROGRESSED:
                eventType = "Work progressed";
                break;
            case EVENT_TYPE_WORK_FINISHED:
                eventType = "Work finished";
                break;
            case EVENT_TYPE_LOCK_RELEASED:
                if (event.data == (void *)LOCK_TYPE_READ) {
                    eventType = "Read lock released";
                } else {
                    eventType = "Write lock released";
                }
                break;
            case EVENT_TYPE_THREAD_FINISHED:
                eventType = "Thread finished";
                break;
            default:
                eventType = "Unknown";
                break;
        }

        const char *threadType;
        switch (event.threadType) {
            case THREAD_TYPE_READER:
                threadType = "reader";
                break;
            case THREAD_TYPE_WRITER:
                threadType = "writer";
                break;
            case THREAD_TYPE_READER_WRITER:
                threadType = "reader-writer";
                break;
            default:
                threadType = "unknown";
                break;
        }

        printf("(%s) %s: %ld", threadType, eventType, event.threadId);

        if (event.eventType == EVENT_TYPE_WORK_STARTED) {
            printf(" (work amount: %ld)", (long)event.data);
        } else if (event.eventType == EVENT_TYPE_WORK_PROGRESSED) {
            printf(" (progress: %ld)", (long)event.data);
        }

        printf("\n");
    }
}

void RWLockTest() {
    printf("Starting RWLock test suite\n");

    // NOTE: Disabling periodic yield to have explicit control over context switches.
    // This is needed to be able to validate the event log correctly.
    disablePeriodicYield = true;

    rwLock = new RWLock();

    for (int i = 0; i < NUM_RUNS; i++) {
        printf("\nRun %d\n", i + 1);

        eventLog.clear();
        threads.clear();

        for (long j = 0; j < NUM_THREADS; j++) {
            char name[32];
            sprintf(name, "(run %d/%d) thread %ld", i + 1, NUM_RUNS, j);

            Thread *thread = new Thread(name, true);
            threads.push_back(thread);

            thread->Fork(Worker, (void *)j);
        }

        for (Thread *t : threads) t->Join();

        PrintEvents();

        ValidateEvents();
    }

    delete rwLock;

    printf("\nRWLock test suite completed\n");
}
