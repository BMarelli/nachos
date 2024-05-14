#include "priority.hh"

char const *PriorityToString(Priority priority) {
    switch (priority) {
        case PRIORITY_HIGH:
            return "HIGH";
        case PRIORITY_NORMAL:
            return "NORMAL";
        case PRIORITY_LOW:
            return "LOW";
        default:
            return "UNKNOWN";
    }
}
