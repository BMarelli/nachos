#ifndef NACHOS_THREADS_PRIORITY_HH
#define NACHOS_THREADS_PRIORITY_HH

enum Priority {
    PRIORITY_HIGH = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_LOW = 2,

    NUM_PRIORITIES = 3
};

char const *PriorityToString(Priority priority);

#endif
