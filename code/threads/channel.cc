#include "channel.hh"

Channel::Channel(char *debugName)
{
  name = debugName;
  lock4send = new Lock("lock4send");
  lock4receive = new Lock("lock4receive");
  semaphore4send = new Semaphore("semaphore4send", 0);
  semaphore4receive = new Semaphore("semaphore4receive", 0);
}

Channel::~Channel()
{
  delete lock4send;
  delete lock4receive;
  delete semaphore4send;
  delete semaphore4receive;
}

void
Channel::Send(int message)
{
  lock4send->Acquire();
  semaphore4send->P();
  buffer = message;
  semaphore4receive->V();
  lock4send->Release();
}

void
Channel::Receive(int *message)
{
  lock4receive->Acquire();
  semaphore4send->V();
  semaphore4receive->P();
  *message = buffer;
  lock4receive->Release();
}
