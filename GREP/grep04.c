//Before deleting putchr() and other comments
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

int  peekc, lastc, given, ninbuf, io, pflag;
int  vflag  = 1, oflag, listf, listn, col, tfile  = -1, tline, iblock  = -1, oblock  = -1, ichanged, nleft;
int  names[26], anymarks, nbra, subnewa, subolda, fchange, wrapp, bpagesize = 20;
unsigned nlall = 128;  unsigned int  *addr1, *addr2, *dot, *dol, *zero;

long  count;
char  Q[] = "", T[] = "TMP", savedfile[FNSIZE], file[FNSIZE], linebuf[LBSIZE], rhsbuf[LBSIZE/2], expbuf[ESIZE+4];
char  genbuf[LBSIZE], *nextip, *linebp, *globp, *mktemp(char *), tmpXXXXX[50] = "/tmp/eXXXXX";
char  *tfname, *loc1, *loc2, ibuff[BLKSIZE], obuff[BLKSIZE], WRERR[]  = "WRITE ERROR", *braslist[NBRA], *braelist[NBRA];
char  line[70];  char  *linp  = line;

long	lseek(int, long, int);
int	close(int);
// int	fork(void);
// int	wait(int *);

char *getblock(unsigned int atl, int iof);
char *getline_(unsigned int tl);
int advance(char *lp, char *ep);
int append(int (*f)(void), unsigned int *a);
int backref(int i, char *lp);
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
// void putchrr(int ac);
void putd(void);
int putline(void);
void quit(int n);
void setwide(void);
void squeeze(int i);

jmp_buf	savej;

int main(int argc, char *argv[]) {
	char *p1, *p2;
	argv++;
	while (argc > 1 && **argv=='-') {
		argv++;
		argc--;
	}
	if (oflag) {
		p1 = "/dev/stdout";
		p2 = savedfile;
		while (*p2++ = *p1++);
	}
	if (argc>1) {
		p1 = *argv;
		p2 = savedfile;
		while (*p2++ = *p1++)
			if (p2 >= &savedfile[sizeof(savedfile)]) p2--;
		globp = "r";
	}
	zero = (unsigned *)malloc(nlall*sizeof(unsigned));
	tfname = mkdtemp(tmpXXXXX);
	init();
	setjmp(savej);
	commands();
	quit(0);
	return 0;
}
unsigned int *address(void) {
	int c, sign = 1, opcnt = 0, nextopand = -1;
	unsigned int *a = dot, *b;
	do {
		do c = getchr(); while (c==' ' || c=='\t');
		if ('0'<=c && c<='9') {
			peekc = c;
			if (!opcnt) { a = zero; }
			a += sign*getnum();
		} else switch (c) {
		case '$': a = dol;
		case '.':
			if (opcnt){ error(Q); }
			break;
		case '\'':
			c = getchr();
			if (opcnt || c<'a' || 'z'<c) { error(Q); }
			a = zero;
			do a++; while (a<=dol && names[c-'a']!=(*a&~01));
			break;
		case '?': sign = -sign;
		case '/':
			compile(c);
			b = a;
			for (;;) {
				a += sign;
				if (a<=zero)    { a = dol;  }
				if (a>dol  )    { a = zero; }
				if (execute(a)) { break;    }
				if (a==b)       { error(Q); }
			}
			break;
		default:
			if (nextopand == opcnt) {
				a += sign;
				if (a<zero || dol<a) { continue; }       
			}
			if (c!='+' && c!='-' && c!='^') {
				peekc = c;
				if (opcnt==0) { a = 0; }
				return (a);
			}
			sign = 1;
			if (c!='+') { sign = -sign; }
			nextopand = ++opcnt;
			continue;
		}
		sign = 1;
		opcnt++;
	} while (zero<=a && a<=dol);
	error(Q);
	/*NOTREACHED*/
	return 0;
}
int advance(char *lp, char *ep) {
	char *curlp;
	int i;
	for (;;) switch (*ep++) {
	case CCHR:
		if (*ep++ == *lp++) { continue; }
		return(0);
	case CDOT:
		if (*lp++) { continue; }
		return(0);
	case CDOL:
		if (*lp==0) { continue; }
		return(0);
	case CEOF:
		loc2 = lp;
		return(1);
	case CCL:
		if (cclass(ep, *lp++, 1)) {
			ep += *ep;
			continue;
		}
		return(0);
	case NCCL:
		if (cclass(ep, *lp++, 0)) {
			ep += *ep;
			continue;
		}
		return(0);
	case CBRA:
		braslist[*ep++] = lp;
		continue;
	case CKET:
		braelist[*ep++] = lp;
		continue;
	case CBACK:
		if (braelist[i = *ep++]==0) { error(Q); }
		if (backref(i, lp)) {
			lp += braelist[i] - braslist[i];
			continue;
		}
		return(0);
	case CBACK|STAR:
		if (braelist[i = *ep++] == 0) { error(Q); }
		curlp = lp;
		while (backref(i, lp)) { lp += braelist[i] - braslist[i]; }
		while (lp >= curlp) {
			if (advance(lp, ep)) { return(1); }
			lp -= braelist[i] - braslist[i];
		}
		continue;
	case CDOT|STAR:
		curlp = lp;
		while (*lp++);
		goto star;
	case CCHR|STAR:
		curlp = lp;
		while (*lp++ == *ep);
		ep++;
		goto star;
	case CCL|STAR:
	case NCCL|STAR:
		curlp = lp;
		while (cclass(ep, *lp++, ep[-1]==(CCL|STAR)));
		ep += *ep;
		goto star;
	star:
		do {
			lp--;
			if (advance(lp, ep)) { return(1); }
		} while (lp > curlp);
		return(0);
	default:
		error(Q);
	}
}
int append(int (*f)(void), unsigned int *a) {
	unsigned int *a1, *a2, *rdot;
	int nline = 0, tl;
	dot = a;
	while ((*f)() == 0) {
		if ((dol-zero)+1 >= nlall) {
			unsigned *ozero = zero;

			nlall += 1024;
			if ((zero = (unsigned *)realloc((char *)zero, nlall*sizeof(unsigned)))==NULL) {
				error("MEM?");
			}
			dot += zero - ozero;
			dol += zero - ozero;
		}
		tl = putline();
		nline++;
		a1 = ++dol;
		a2 = a1+1;
		rdot = ++dot;
		while (a1 > rdot) { *--a2 = *--a1; }
		*rdot = tl;
	}
	return(nline);
}
int backref(int i, char *lp) {
	char *bp = braslist[i];
	while (*bp++ == *lp++)
		if (bp >= braelist[i]) { return(1); }
	return(0);
}
void blkio(int b, char *buf, long (*iofcn)(int, void*, unsigned long)) {
  lseek(tfile, (long)b*BLKSIZE, 0);  
  if ((*iofcn)(tfile, buf, BLKSIZE) != BLKSIZE) {  error(T);  }
}
int cclass(char *set, int c, int af) {
	int n;
	if (c==0) { return(0); }
	n = *set++;
	while (--n)
		if (*set++ == c) { return(af); }
	return(!af);
}
void commands(void) {
	unsigned int *a1;
	int c, temp;
	char lastsep;
	for (;;) {
	if (pflag) { 
        pflag = 0;
        addr1 = addr2 = dot;
        print();
    }
	c = '\n';
	for (addr1 = 0;;) {
		lastsep = c;
		a1 = address(); 
        c = getchr();
		if (c!=',' && c!=';'){ break; }
		if (lastsep==',') { error(Q); }
		if (a1==0) { 
            a1 = zero+1; 
            if (a1>dol) {a1--;}
         }
		addr1 = a1;
		if (c==';') { dot = a1; }
	}
	if (lastsep!='\n' && a1==0){ a1 = dol;}
	if ((addr2=a1)==0) { 
        given = 0; 
        addr2 = dot;
    }
	else { given = 1; }
	if (addr1==0) { addr1 = addr2; }

	switch(c) {
	case 'g': 
        global(1); 
        continue;
	case '\n':
		if (a1==0) { 
            a1 = dot+1; 
            addr2 = a1; 
            addr1 = a1; 
            }
		if (lastsep==';') { addr1 = a1; } 
        print(); 
        continue;
	case 'p':
	case 'P': 
        newline(); 
        print(); 
        continue;
	case 'r': filename(c);
	caseread:
		if ((io = open(file, 0)) < 0) { 
            lastc = '\n'; 
            error(file); 
        }
		setwide(); 
        squeeze(0); 
        ninbuf = 0; 
        c = zero != dol; 
        append(getfile, addr2); 
        exfile(); 
        fchange = c;
		continue;
	case EOF: return;

	}
	error(Q);
	}
}
void compile(int eof) {
	int c, cclcnt;
	char *ep = expbuf, *lastep, bracket[NBRA], *bracketp = bracket;
	if ((c = getchr()) == '\n') {
		peekc = c;
		c = eof;
	}
	if (c == eof) {
		if (*ep==0) { error(Q); }
		return;
	}
	nbra = 0;
	if (c=='^') {
		c = getchr();
		*ep++ = CCIRC;
	}
	peekc = c;
	lastep = 0;
	for (;;) {
		if (ep >= &expbuf[ESIZE]) { goto cerror; }
		c = getchr();
		if (c == '\n') {
			peekc = c;
			c = eof;
		}
		if (c==eof) {
			if (bracketp != bracket) { goto cerror; }
			*ep++ = CEOF;
			return;
		}
		if (c!='*') { lastep = ep; }
		switch (c) {
		case '\\':
			if ((c = getchr())=='(') {
				if (nbra >= NBRA) { goto cerror; }
				*bracketp++ = nbra;
				*ep++ = CBRA;
				*ep++ = nbra++;
				continue;
			}
			if (c == ')') {
				if (bracketp <= bracket) { goto cerror; }
				*ep++ = CKET;
				*ep++ = *--bracketp;
				continue;
			}
			if (c>='1' && c<'1'+NBRA) {
				*ep++ = CBACK;
				*ep++ = c-'1';
				continue;
			}
			*ep++ = CCHR;
			if (c=='\n') { goto cerror; }
			*ep++ = c;
			continue;
		case '.':
			*ep++ = CDOT;
			continue;
		case '\n':
			goto cerror;
		case '*':
			if (lastep==0 || *lastep==CBRA || *lastep==CKET) { goto defchar; }
			*lastep |= STAR;
			continue;
		case '$':
			if ((peekc=getchr()) != eof && peekc!='\n') { goto defchar; }
			*ep++ = CDOL;
			continue;
		case '[':
			*ep++ = CCL;
			*ep++ = 0;
			cclcnt = 1;
			if ((c=getchr()) == '^') {
				c = getchr();
				ep[-2] = NCCL;
			}
			do {
				if (c=='\n') { goto cerror; }
				if (c=='-' && ep[-1]!=0) {
					if ((c=getchr())==']') {
						*ep++ = '-';
						cclcnt++;
						break;
					}
					while (ep[-1]<c) {
						*ep = ep[-1]+1;
						ep++;
						cclcnt++;
						if (ep>=&expbuf[ESIZE]) { goto cerror; }
					}
				}
				*ep++ = c;
				cclcnt++;
				if (ep >= &expbuf[ESIZE]) { goto cerror; }
			} while ((c = getchr()) != ']');
			lastep[1] = cclcnt;
			continue;
		defchar:
		default:
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
   cerror:
	expbuf[0] = 0;
	nbra = 0;
	error(Q);
}
void error(char *s) {
	int c;
    wrapp = listf = listn = 0;
	putchar('?');
	puts(s);
	count = 0;
	lseek(0, (long)0, 2);
	pflag = 0;
	if (globp) { lastc = '\n'; }
	globp = 0;
	peekc = lastc;
	if(lastc)
		while ((c = getchr()) != '\n' && c != EOF);
	if (io > 0) {
		close(io);
		io = -1;
	}
	longjmp(savej, 1);
}
int execute(unsigned int *addr) {
	char *p1, *p2;
	int c;
	for (c=0; c<NBRA; c++) {
		braslist[c] = braelist[c] = 0;
	}
	p2 = expbuf;
	if (addr == (unsigned *)0) {
		if (*p2==CCIRC) { return(0); }
		p1 = loc2;
	} else if (addr==zero) { return(0); }
	else { p1 = getline_(*addr); }
	if (*p2==CCIRC) {
		loc1 = p1;
		return(advance(p1, p2+1));
	}
	/* fast check for first character */
	if (*p2==CCHR) {
		c = p2[1];
		do {
			if (*p1!=c) { continue; }
			if (advance(p1, p2)) {
				loc1 = p1;
				return(1);
			}
		} while (*p1++);
		return(0);
	}
	/* regular algorithm */
	do {
		if (advance(p1, p2)) {
			loc1 = p1;
			return(1);
		}
	} while (*p1++);
	return(0);
}
void exfile(void) {
	close(io);
	io = -1;
	if (vflag) {
		putd();
		putchar('\n');
	}
}
void filename(int comm) {
	char *p1, *p2;
	int c;
	count = 0;
	c = getchr();
	if (c=='\n' || c==EOF) {
		p1 = savedfile;
		if (*p1==0 && comm!='f') { error(Q); }
		p2 = file;
		while (*p2++ = *p1++);
		return;
	}
	if (c!=' ') { error(Q); }
	while ((c = getchr()) == ' ');
	if (c=='\n') { error(Q); }
	p1 = file;
	do {
		if (p1 >= &file[sizeof(file)-1] || c==' ' || c==EOF) { error(Q); }
		*p1++ = c;
	} while ((c = getchr()) != '\n');
	*p1++ = 0;
	if (savedfile[0]==0 || comm=='e' || comm=='f') {
		p1 = savedfile;
		p2 = file;
		while (*p1++ = *p2++);
	}
}
char *getblock(unsigned int atl, int iof) {
	int bno = (atl/(BLKSIZE/2)), off = (atl<<1) & (BLKSIZE-1) & ~03;
	if (bno >= NBLK) {
		lastc = '\n';
		error(T);
	}
	nleft = BLKSIZE - off;
	if (bno==iblock) {
		ichanged |= iof;
		return(ibuff+off);
	}
	if (bno==oblock) { return(obuff+off); }
	if (iof==READ) {
		if (ichanged) { blkio(iblock, ibuff,(long (*)(int, void*, unsigned long)) write); }
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, read);
		return(ibuff+off);
	}
	if (oblock>=0) { blkio(oblock, obuff,(long (*)(int, void*, unsigned long)) write); }
	oblock = bno;
	return(obuff+off);
}
int getchr(void) {
	char c;
	if (lastc=peekc) {
		peekc = 0;
		return(lastc);
	}
	if (globp) {
		if ((lastc = *globp++) != 0) { return(lastc); }
		globp = 0;
		return(EOF);
	}
	if (read(0, &c, 1) <= 0) { return(lastc = EOF); }
	lastc = c&0177;
	return(lastc);
}
int getfile(void) {
	int c;
	char *lp = linebuf, *fp = nextip;
	do {
		if (--ninbuf < 0) {
			if ((ninbuf = read(io, genbuf, LBSIZE)-1) < 0)
				if (lp>linebuf) {
					puts("'\\n' appended");
					*genbuf = '\n';
				}
				else return(EOF);
			fp = genbuf;
			while(fp < &genbuf[ninbuf]) {
				if (*fp++ & 0200) { break; }
			}
			fp = genbuf;
		}
		c = *fp++;
		if (c=='\0') { continue; }
		if (c&0200 || lp >= &linebuf[LBSIZE]) {
			lastc = '\n';
			error(Q);
		}
		*lp++ = c;
		count++;
	} while (c != '\n');
	*--lp = 0;
	nextip = fp;
	return(0);
}
char *getline_(unsigned int tl) {
	char *bp = getblock(tl, READ), *lp = linebuf;
	int nl = nleft;
	tl &= ~((BLKSIZE/2)-1);
	while (*lp++ = *bp++)
		if (--nl == 0) {
			bp = getblock(tl+=(BLKSIZE/2), READ);
			nl = nleft;
		}
	return(linebuf);
}
int getnum(void) {
	int r = 0, c;
	while ((c=getchr())>='0' && c<='9') { r = r*10 + c - '0'; }
	peekc = c;
	return (r);
}
void global(int k) {
	char *gp, globuf[GBSIZE] ;
	int c;
	unsigned int *a1;
	if (globp) { error(Q); }
	setwide();
	squeeze(dol>zero);
	if ((c=getchr())=='\n') { error(Q); }
	compile(c);
	gp = globuf;
	while ((c = getchr()) != '\n') {
		if (c==EOF) { error(Q); }
		if (c=='\\') {
			c = getchr();
			if (c!='\n') { *gp++ = '\\'; }
		}
		*gp++ = c;
		if (gp >= &globuf[GBSIZE-2]) { error(Q); }
	}
	if (gp == globuf) { *gp++ = 'p'; }
	*gp++ = '\n';
	*gp++ = 0;
	for (a1=zero; a1<=dol; a1++) {
		*a1 &= ~01;
		if (a1>=addr1 && a1<=addr2 && execute(a1)==k) { *a1 |= 01; }
	}
	for (a1=zero; a1<=dol; a1++) {
		if (*a1 & 01) {
			*a1 &= ~01;
			dot = a1;
			globp = globuf;
			commands();
			a1 = zero;
		}
	}
}
void init(void) {
	int *markp;
	close(tfile);
	tline = 2;
	for (markp = names; markp < &names[26]; ) { *markp++ = 0; } 
	subnewa = ichanged = anymarks = 0;
	iblock = oblock = -1;
	close(creat(tfname, 0600));
	tfile = open(tfname, 2);
	dot = dol = zero;
}
void newline(void) {
	int c;
	if ((c = getchr()) == '\n' || c == EOF) { return; }
	if (c=='p' || c=='l' || c=='n') {
		pflag++;
		if      (c=='l')             { listf++; }
		else if (c=='n')             { listn++; }
		if      ((c=getchr())=='\n') { return;  }
	}
	error(Q);
}
void print(void) {
	unsigned int *a1;
	squeeze(1);
	a1 = addr1;
	do {
		if (listn) { 
            count = a1-zero; 
            putd(); 
            putchar('\t');
        }
		puts(getline_(*a1++));
	} while (a1 <= addr2);
	dot = addr2;
	listf = listn = pflag = 0;
}
// void putchrr(int ac) {
// 	char *lp = linp;
// 	int c = ac;
// 	if (listf) {
// 		if (c=='\n') {
// 			if (linp!=line && linp[-1]==' ') {
// 				*lp++ = '\\';
// 				*lp++ = 'n';
// 			}
// 		} else {
// 			if (col > (72-4-2)) {
// 				col = 8;
// 				*lp++ = '\\';
// 				*lp++ = '\n';
// 				*lp++ = '\t';
// 			}
// 			col++;
// 			if (c=='\b' || c=='\t' || c=='\\') {
// 				*lp++ = '\\';
// 				if (c=='\b') { c = 'b'; }
// 				else if (c=='\t') { c = 't'; }
// 				col++;
// 			} else if (c<' ' || c=='\177') {
// 				*lp++ = '\\';
// 				*lp++ =  (c>>6)    +'0';
// 				*lp++ = ((c>>3)&07)+'0';
// 				c     = ( c    &07)+'0';
// 				col += 3;
// 			}
// 		}
// 	}
// 	*lp++ = c;
// 	if(c == '\n' || lp >= &line[64]) {
// 		linp = line;
// 		write(oflag?2:1, line, lp-line);
// 		return;
// 	}
// 	linp = lp;
// }
void putd(void) {
	int r = count % 10;
	count /= 10;
	if (count) { putd(); }
	putchar(r + '0');
}
int putline(void) {
    unsigned int tl = tline;
	char *bp = getblock(tl, WRITE), *lp = linebuf;
	int nl = nleft;
	fchange = 1;
	tl &= ~((BLKSIZE/2)-1);
	while (*bp = *lp++) {
		if (*bp++ == '\n') {
			*--bp = 0;
			linebp = lp;
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl+=(BLKSIZE/2), WRITE);
			nl = nleft;
		}
	}
	nl = tline;
	tline += (((lp-linebuf)+03)>>1)&077776;
	return(nl);
}
void setwide(void) {
	if (!given) {
		addr1 = zero + (dol>zero);
		addr2 = dol;
	}
}
void squeeze(int i) { if (addr1<zero+i || addr2>dol || addr1>addr2){ error(Q); }}
void quit(int n) {
	if (vflag && fchange && dol!=zero) {
		fchange = 0;
		error(Q);
	}
	unlink(tfname);
	exit(0);
}


