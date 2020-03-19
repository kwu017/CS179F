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
  //struct spinlock sl_lock;
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

  // if (lport != 0) {
  //   si->lport = lport;
  // }
  // else {
  //   uint16 i = 8000;
  //   struct sock *s = sockets;
  //   while(s)
  //   {
  //     if(s->lport == i)
  //     {
  //       i++;
  //     }
  //   }
  //   si->lport = i;
  // }
  si->lport = lport;
  si->rport = rport;
  initlock(&si->lock, "sock");
  //memset(&si->lock, 0, sizeof(si->lock));
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

void sockclose(struct sock *s) { //fixed some things I THINK...BUT could be wrong
  printf("Closing socket");
  printf(" with port #: %d\n", s->lport);
  struct sock *pos, *next;
  acquire(&s->lock);

  printf("About to free outstanding mbufs\n");
  // free outstanding mbufs
  while(!mbufq_empty(&s->rxq)) {
    printf("Whee! We are freeing mbufs!!!\n");
    mbuffree(mbufq_pophead(&s->rxq));
  }
  printf("Finished freeing outstanding mbufs...\n");

  // remove from sockets
  acquire(&lock);
  pos = sockets;
  if (pos->raddr == s->raddr && pos->lport == s->lport && pos->rport == s->rport) {
    sockets = pos->next;
  } 
  else {
    while(pos->next) {
      next = pos->next;
      if (next->raddr == s->raddr && next->lport == s->lport && next->rport == s->rport) {
        pos->next = next->next;
        break;
      }
    }
  }
  release(&lock);

  // clean up
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
  printf("REACHED END OF CLOSE FUNCTION\n");
}
//socketwrite(struct sock *s, uint64 addr, int n)
// from *s, get rport; from rport, get remote_socket
// write to socket
// wakeup(rsock->mbufq);


int sockwrite(struct sock *s, uint64 addr, int n) {
  printf("sockwrite(s=%x , addr=%x, n=%d\n", s,addr,n);
  if(s->raddr == 2130706433)
  {
    //printf("Localhost! YAY!\n");
    struct proc *pr = myproc();
    struct sock* sptr = sockets;
    struct sock* remote = 0;
    struct mbuf *m = mbufalloc(MBUF_DEFAULT_HEADROOM);

    //acquire(&s->lock);
    //printf("lock aquired for sockwrite\n");
    while (sptr && sptr->lport != s->rport)
    {
      printf("checking sptr %x %d\n", sptr, sptr->lport);
      sptr = sptr->next;
    }

    if(sptr)
    {
      printf("lport = %d\n", sptr->lport);
      remote = sptr;
      //printf("start if statement\n");
      acquire(&remote->lock); //used to be remote
      //printf("lock aquired for sockwrite\n");
      void * memaddr;
      printf("Void pointers YAY!\n");
      //struct mbuf *m = mbufalloc(n + MBUF_DEFAULT_HEADROOM);
      printf("sockwrite:  mbuf head = %x\n", m->head);
      printf("mbuf alloc'ed!\n");
      memaddr = mbufput(m, n);
      printf("mbufput done\n");
      if (copyin(pr->pagetable, memaddr, addr, n) == -1) {
		printf("Error copying data into kern space\n");
        return -1;
      }
      //copyin(pr->pagetable, memaddr , addr, n);
      //mbufq_pushtail(&remote->rxq, m);
      printf("&remote->rxq = %x\n", &remote->rxq);
      //net_tx_udp(m, s->raddr ,s->lport, s->rport);
      //printf("HELLO THIS IS AFTER NET_TX_UDP FUNCTION\n");
      wakeup((void*) &remote->rxq);
      //release(&s->lock);
      release(&remote->lock);
      //printf("PLEASE RELEASE!!\n");
      return n;
    }
	else{
		printf("No destination socket with that port found\n");
	}
  }
  return n;
}

// void socksendto(uint32 raddr, uint16 lport, uint16 rport, char* memaddress, uint16 len) {
//   char temparray[2048]; //lol
//   if (raddr != 2130706433) { // 127.0.0.1
//     printf("ERROR :( THIS HASN'T BEEN IMPLEMENTED YET D:");
//     return;
//   }

//   else {
//     // printf("hello");
//     struct mbuf *m = mbufalloc(len + MBUF_DEFAULT_HEADROOM); // allocate new mbuf
//     struct proc *pr = myproc();
//     if (len>2048){}
//     copyin(pr->pagetable, temparray, (uint64) memaddress, len); //returns int
//     m->head = mbufput(m, len);
//     net_tx_udp(m, raddr, lport, rport); //returns int
//   }
// }

// called by protocol handler layer to deliver UDP packets
//socketread(struct sock *s, uint64 addr, int n)
//if(mbufq)

int sockread(struct sock *s, uint64 addr, int n) {
  struct mbuf *m;
  struct proc *pr = myproc();
  unsigned int i;

  acquire(&s->lock);
  printf("starting sockread\n");
   while (mbufq_empty(&s->rxq)) {
    printf("wassup this is loop\n");
    if(myproc()->killed){
      release(&s->lock);
      printf("nuuuu!!!\n");
      return -1;
    }
      printf("sleepy time\n");
     sleep(&s->rxq, &s->lock);
   }

   printf("sockread:  sock port = %d,  addr = %x,  n = %d\n", s->lport, addr, n);
   printf("sockread:  mbuf = %x\n", &m);
  //  unsigned int i = 0;
  //  void* ch;
   m = mbufq_pophead(&s->rxq);
   printf("popped mbufq head\n");
   //printf("sockread:  mbuf head = %x\n", m->head);
   printf("copying memory out\n");
   for(i = 0; i < n; i++){
    if(copyout(pr->pagetable, addr + i, m->head, 1) == -1) {
      break;
    }
    mbufpull(m, 1);
  }
   //copyout(pr->pagetable, addr, m->head ,n);
   printf("mbuffree\n");
   mbuffree(m);
   release(&s->lock);
   return i;
}

// void sockread(uint32 raddr, uint16 lport, uint16 rport, struct mbuf *m, uint16 len, char* memaddress) {
//   // struct mbuf *m;
//   struct proc *pr = myproc();
//   char temparray[2048]; // yes it's back
//   while (mbufq_empty(&sockets->rxq)) {
//     sleep(&m, &lock);
//   }

//   mbufq_pophead(&sockets->rxq);
//   copyout(pr->pagetable, (uint64) memaddress, temparray, len);
//   mbuffree(m);
// }

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
