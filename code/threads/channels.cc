#include "channels.hh"

Channels::Channels(char *debugName)
{
  name = debugName;
  lock4send = new Lock("lock4send");
  lock4receive = new Lock("lock4receive");
  semaphore4send = new Semaphore("semaphore4send", 0);
  semaphore4receive = new Semaphore("semaphore4receive", 0);
  buffer = nullptr;
}

Channels::~Channels()
{
  delete lock4send;
  delete lock4receive;
  delete semaphore4send;
  delete semaphore4receive;
}

void 
Channels::Send(int message)
{
  lock4send->Acquire();
  semaphore4send->P();
  buffer = nullptr;
  *buffer = message;
  lock4send->Release();
  semaphore4receive->V();
  
}

void
Channels::Receive(int *message) 
{
  lock4receive->Acquire();
  semaphore4receive->P();
  buffer = message;
  semaphore4send->V();
  lock4receive->Release();
}
