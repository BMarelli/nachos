#ifndef NACHOS_CHANNEL__HH
#define NACHOS_CHANNEL__HH

#include "lock.hh"

class Channel
{
public:
  Channel(char *debugName);

  ~Channel();

  void Send(int message);

  void Receive(int *message);

private:
  char *name;

  int buffer;

  Lock *lock4send, *lock4receive;

  Semaphore *semaphore4send, *semaphore4receive;
};
#endif
