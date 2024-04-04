#ifndef NACHOS_LIB_FUNCTION_PTR__HH
#define NACHOS_LIB_FUNCTION_PTR__HH

/// This declares the type `VoidFunctionPtr` to be a “pointer to a
/// function taking a pointer argument and returning nothing”.
///
/// With such a function pointer (say it is "func"), we can call it like
/// this:
///    func (arg);
/// or:
///    (*func) (arg);
///
/// This is used by `Thread::Fork` and for interrupt handlers, as well as a
/// couple of other places.
typedef void (*VoidFunctionPtr)(void *arg);

typedef void (*VoidNoArgFunctionPtr)();

#endif
