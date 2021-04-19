#ifndef NACHOS_CHANNELS__HH
#define NACHOS_CHANNELS__HH

#include "lock.hh"

class Channels
{
public:
  Channels(char *debugName);

  ~Channels();

  void Send(int message);

  void Receive(int *message);

private:
  char *name;

  int buffer;

  Lock *lock4send, *lock4receive;

  Semaphore *semaphore4send, *semaphore4receive;
};
#endif
