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
  si->lport = lport;
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
      printf("releasing lock at sysnet.c l 89\n");
      goto bad;
    }
    pos = pos->next;
  }
  printf("Creating socket: raddr = %d, lport = %d, rport = %d\n", si->raddr, si->lport, si->rport);
  si->next = sockets;
  sockets = si;
  printf("releasing lock at sysnet.c l 97\n");
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

void sockclose(struct sock *s) { 
  printf("Closing socket with port #: %d\n", s->lport);
  struct sock *y;
  struct sock *next; 
  acquire(&s->lock);

  // remove from sockets
  acquire(&lock);
  y = sockets;
  if (y->raddr == s->raddr && y->lport == s->lport && y->rport == s->rport) {
    sockets = y->next;
  } 

  else {
    while (y->next) {
      next = y->next;
      if (next->raddr == s->raddr && next->lport == s->lport && next->rport == s->rport) {
        y->next = next->next;
        break;
      }
    }
  }
  release(&lock);
  release(&s->lock);
  kfree((char*)s);
  // acquire(&s->lock);
  // printf("Closing socket");
  // printf(" with port #: %d\n", s->lport);
  // struct sock *y = sockets;
  // if (sockets == s) {
  //   sockets = sockets->next;
  //   kfree((char*)s);
  //   release(&s->lock);
  //   return;
  // }

  // while ((y->next != s) && y) {
  //   y = y->next;
  // }

  // kfree((char*)s);
  // release(&s->lock);
}
//socketwrite(struct sock *s, uint64 addr, int n)
// from *s, get rport; from rport, get remote_socket
// write to socket
// wakeup(rsock->mbufq);


void sockwrite(struct sock *s, uint64 addr, int n) {
  printf("\nSTARTING SOCKWRITE\n");
  printf("sockwrite(s=%x , addr=%x, n=%d\n", s,addr,n);

  // if the address is equal to 127.0.0.1
  if(s->raddr == 2130706433)
  {
    struct proc *pr = myproc();
    struct sock* sptr = sockets;
    struct sock* remote = 0;
    // allocating a new mbuf
    struct mbuf *m = mbufalloc(n + MBUF_DEFAULT_HEADROOM);

    // checking loop to search for a matching destination port
    while (sptr && sptr->lport != s->rport)
    {
      printf("checking sptr: sptr = %x, sptr->lport = %d\n", sptr, sptr->lport);
      sptr = sptr->next;
    }

    // if socket pointer exists, continue with transferring payload
    if(sptr)
    {
      printf("sptr->lport = %d\n", sptr->lport);
      remote = sptr;
      acquire(&remote->lock);
      void * memaddr;
      //printf("sockwrite:  mbuf head = %x\n", m->head);
      printf("mbuf alloc'ed!\n");
      memaddr = mbufput(m, n);
      printf("mbufput done\n");

      printf("&remote->rxq = %x\n", &remote->rxq);
      if (copyin(pr->pagetable, memaddr, addr, n) == -1) {
		  printf("Error copying data into kern space\n");
        exit(1);
        return;
      }

      // use mbufq_pushtail to push onto mbufq, so no longer an empty mbufq
      mbufq_pushtail(&remote->rxq, m);
      // need to wake up sleeping rxq
      wakeup((void*) &remote->rxq);
      release(&remote->lock);
    }

	 else{
		  printf("No destination socket with that port found\n");
	 }
  }
}

// called by protocol handler layer to deliver UDP packets
//socketread(struct sock *s, uint64 addr, int n)
//if(mbufq)

void sockread(struct sock *s, uint64 addr, int n) {
  struct mbuf *m;
  struct proc *pr = myproc();

  acquire(&s->lock);
  printf("\nSTARTING SOCKETREAD\n");

  // checking whether or not mbufq is empty
   while (mbufq_empty(&s->rxq)) {
     // if empty, sleep until something is enqueued
      printf("sleeping...\n");
      printf("&s->rxq: %x\n", &s->rxq);
     sleep(&s->rxq, &s->lock);
   }

   printf("now awake\n");
   printf("sockread:  sock port = %d,  addr = %x,  n = %d\n", s->lport, addr, n);
   printf("sockread:  mbuf = %x\n", &m);

   // need to pop top of mbuf from rxq
   m = mbufq_pophead(&s->rxq);
   printf("popped mbufq head\n");

   // use copyout to move payload into memory
   copyout(pr->pagetable, addr, m->head ,n);
   printf("copying memory out\n");

   // freeing the mbuf
   mbuffree(m);
   printf("mbuffree\n");
   release(&s->lock);
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
