//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

struct sock {
  struct sock *next; // the next socket in the list
  uint32 raddr;      // the remote IPv4 address
  uint16 lport;      // the local UDP port number
  uint16 rport;      // the remote UDP port number
  struct spinlock lock; // protects the rxq
  struct mbufq rxq;  // a queue of packets waiting to be received
};

static struct spinlock lock;
static struct sock *sockets;

void
sockinit(void)
{
  initlock(&lock, "socktbl");
}

// void
// sockbind(void)
// {
//   initlock(&lock, "socktbl");
// }

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
  struct sock *si, *pos;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  // initialize objects
  si->raddr = raddr;

  if (lport != 0) {
    si->lport = lport;
  }
  else {
    uint16 i = 8000;
    struct sock *s = sockets;
    while(s)
    {
      if(s->lport == i)
      {
        i++;
      }
    }
    s->lport = i;
  }
  si->rport = rport;
  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // sock bind stuff

  // add to list of sockets
  acquire(&lock);
  pos = sockets;
  while (pos) {
    if (pos->raddr == raddr &&
        pos->lport == lport &&
	pos->rport == rport) {
      release(&lock);
      goto bad;
    }
    pos = pos->next;
  }
  si->next = sockets;
  sockets = si;
  release(&lock);
  return 0;

bad:
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}

//
// Your code here.
//
// Add and wire in methods to handle closing, reading,
// and writing for network sockets.
//

void sockclose(struct sock *s) { //fixed some things I THINK...BUT could be wrong
  acquire(&s->lock);
  struct sock *y = sockets;
  if (sockets == s) {
    sockets = sockets->next;
    kfree((char*)s);
    release(&s->lock);
    return;
  }

  while ((y->next != s) && y) {
    y = y->next;
  }

  kfree((char*)s);
  release(&s->lock);
}

void socksendto(uint32 raddr, uint16 lport, uint16 rport, char* memaddress, uint16 len) {
  char temparray[2048]; //lol
  if (raddr != 2130706433) { // 127.0.0.1
    printf("ERROR :( THIS HASN'T BEEN IMPLEMENTED YET D:");
    return;
  }

  else {
    // printf("hello");
    struct mbuf *m = mbufalloc(len + MBUF_DEFAULT_HEADROOM); // allocate new mbuf
    struct proc *pr = myproc();
    copyin(pr->pagetable, temparray, (uint64) memaddress, len); //returns int
    m->head = mbufput(m, len);
    net_tx_udp(m, raddr, lport, rport); //returns int
  }
}

// called by protocol handler layer to deliver UDP packets

void sockread(uint32 raddr, uint16 lport, uint16 rport, struct mbuf *m, uint16 len, char* memaddress) {
  // struct mbuf *m;
  struct proc *pr = myproc();
  char temparray[2048]; // yes it's back
  while (mbufq_empty(&sockets->rxq)) {
    sleep(&m, &lock);
  }

  mbufq_pophead(&sockets->rxq);
  copyout(pr->pagetable, (uint64) memaddress, temparray, len);
  mbuffree(m);
}

void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
  //
  // Your code here.
  //
  // Find the socket that handles this mbuf and deliver it, waking
  // any sleeping reader. Free the mbuf if there are no sockets
  // registered to handle it.
  //
  mbuffree(m);
}
