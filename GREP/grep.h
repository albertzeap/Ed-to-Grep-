#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
/* make BLKSIZE and LBSIZE 512 for smaller machines */
#define	BLKSIZE	4096
#define	NBLK	2047
#define	FNSIZE	128
#define	LBSIZE	4096
#define	ESIZE	256
#define	GBSIZE	256
#define	NBRA	5
#define	KSIZE	9
#define	CBRA	1
#define	CCHR	2
#define	CDOT	4
#define	CCL	6
#define	NCCL	8
#define	CDOL	10
#define	CEOF	11
#define	CKET	12
#define	CBACK	14
#define	CCIRC	15
#define	STAR	01
#define	READ	0
#define	WRITE	1

int	close(int);
char *getblock(unsigned int atl, int iof);
char *getline_(unsigned int tl);
int advance(char *lp, char *ep);
int append(int (*f)(void), unsigned int *a);
void blkio(int b, char *buf, long (*iofcn)(int, void*, unsigned long));
int cclass(char *set, int c, int af);
void commands(void);
void compile(int eof);
void error(char *s);
int execute(unsigned int *addr);
void exfile(void);
void filename(int comm);
int getchr(void);
int getfile(void);
int getnum(void);
void global(int k);
void init(void);
unsigned int *address(void);
void newline(void);
void print(void);
void putd(void);
int putline(void);
void quit(int n);

void readfile(const char *c); //Added 
void search(const char *c); //Added
void search_file(const char *filename, const char * searchfor); //Added

void setwide(void);
void squeeze(int i);