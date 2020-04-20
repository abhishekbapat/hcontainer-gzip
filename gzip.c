/* This work is free. I, the authour, release this work into the public domain.
 * 
 * This work comes AS IS, WITH ALL FAULTS, WITH NO WARRANTY OF ANY KIND WHATSOEVER, not even the implied warranties of MERCHANTABILITY, FITNESS, and TITLE.
 * In NO CASE SHALL THE AUTHOUR BE LIABLE for ANY DAMAGES OR HARMS CAUSED BY OR LINKED TO THIS WORK.
   ADDED MORE FUNCTIONALITY FOR SERVER BY WEI WANG STEVENS 2019
   REWRITE SERVER AND MEMORY ALLOACTION BY TONG XING STEVENS 2019
 */
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include "miniz.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <time.h>
#include <sys/time.h>
#define MAX 80 
#define SA struct sockaddr 

#define PORT 25659  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


typedef struct {
	int l;
	uint32_t crc32;
} St;

enum {
	cFlag = 1,
};

static int chunkSize = 1 << 20;

St go (int level /* Zip level; 0 to mean unzip */, int ifd, int ofd) {
	uint8_t x[1048576], y[1048576];
	z_stream s;
        s.zalloc =NULL; s.zfree = NULL; s.opaque =NULL;
	memset (&s, 0, sizeof (z_stream));
	int windowBits = -MZ_DEFAULT_WINDOW_BITS;
	if (level == 0 ? inflateInit2 (&s,                     windowBits)
	               : mz_deflateInit2 (&s, level, MZ_DEFLATED, windowBits, 6, MZ_DEFAULT_STRATEGY)) {
		errx (-1, "failed");
	}

	for (;;) {
		int n, fin = 0;
		n = read (ifd, x + s.avail_in, chunkSize - s.avail_in);
		if (n < 0) err (n, "failed to read");
		s.next_in   = x;
		s.next_out  = y;
		s.avail_in += n;
		s.avail_out = chunkSize;
retry:
		switch (level == 0 ? mz_inflate (&s, MZ_SYNC_FLUSH) : mz_deflate (&s, n > 0 ? MZ_FINISH : MZ_SYNC_FLUSH)) {
		
		case MZ_STREAM_END:
			fin = 1;
		case MZ_OK:
			write (ofd, y, chunkSize - s.avail_out);
			s.avail_in = 0;
			if (fin) return (St) { .l = level == 0 ? s.total_out : s.total_in, .crc32 = s.crc32 };
			break;
		case MZ_BUF_ERROR:
			continue;
		case MZ_DATA_ERROR:
			errx (-1, "not flated data");
		case MZ_PARAM_ERROR:
			errx (-1, "failed");
		}
	}
}

ssize_t readn (int fd, void *x, size_t n) {
	int n0 = n;
	while (n > 0) {
		int m = read (fd, x, n);
		if (m < 0) return m;
		n -= m;
		x = (uint8_t *)x + m;
	}
	return n0;
}

void skipString (int fd) {
	uint8_t _[1];
	for (_[0] = 1; _[0]; readn (fd, _, 1));
}

void ungz (int ifd, int ofd) {
	uint8_t x[10];
	switch (readn (ifd, x, 10)) {
	case  0: return;
	case 10: break;
	default: errx (-1, "not in gz format");
	}
	if (x[0] != 0x1F || x[1] != 0x8B) errx (-1, "not in gz format");
	if (x[2] != 8) errx (-1, "unknown z-algorithm: 0x%0hhX", x[2]);
	if (x[3] & 1 << 2) /* FEXTRA */ {
		uint16_t n;
		uint8_t n_[2], _[1];
		readn (ifd, n_, 2);
		for (n = n_[0] << 0 | n_[1] << 8; n-- > 0; readn (ifd, _, 1));
	}
	if (x[3] & 1 << 3) /* FNAME    */ skipString (ifd);
	if (x[3] & 1 << 4) /* FCOMMENT */ skipString (ifd);
	if (x[3] & 1 << 1) /* FCRC */ {
		uint16_t _[1];
		readn (ifd, _, 2);
	}
	go (0, ifd, ofd);
}

void storLE32 (uint8_t *p, uint32_t n) {
	p[0] = n >>  0;
	p[1] = n >>  8;
	p[2] = n >> 16;
	p[3] = n >> 24;
}

void gz (int level, int ifd, int ofd) {
	uint8_t hdr[10] = {
		0x1F, 0x8B,	/* magic */
		8,		/* z method */
		0,		/* flags */
		0,0,0,0,	/* mtime */
		0,		/* xfl */
		0xFF,		/* OS */
	};
	uint8_t ftr[8];
	St st;
	int ret;
	ret = write(ofd,hdr,10);
	if(ret < 0) {printf("write error\n");}
	st = go(level,ifd,ofd);
	storLE32 (ftr + 0, st.crc32);
	storLE32 (ftr + 4, st.l);
        ret = write(ofd,ftr,8);
	if(ret < 0) {printf("write error\n");}
}

void print_iteration()
{
        static int i = 0;
        printf("[%d] hetpot is waiting for compression  %d\n", getpid(), i++);
}

int dealshit(){
               int ifd, ofd, level = 6;
               char *ts = "1.txt.gz", *ss = "1.txt"; 
               ifd = open (ss, O_RDONLY);
	       if(ifd==-1) { printf("read 1.txt fail\n"); close(ifd);free(ts);free(ss); return 0;}
               ofd = open (ts, O_WRONLY | O_CREAT | O_TRUNC);
               if (ofd==-1){ printf("open 1.txt.gz fail\n"); close(ifd); close(ofd);free(ts);free(ss); return 0;}
               gz(level,ifd,ofd);
	       close(ifd); 
	       close(ofd); 
               return 1; 
}
// reset function is needed for add migration point in side select loop.
void reset(struct timeval* tv){
    tv->tv_sec = 0;
    tv->tv_usec = 30;
}

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread, check, result, opt=1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int mega_bytes = (1024);
    char *buffer = (char *) malloc(90 * mega_bytes + 100);
    char *buf_ptr, *ack = "AKC", *f_ack="NAK";
    int writesize = 90 * mega_bytes;
    clock_t start, end;
    FILE *fd;
    int rec_left;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 30;

    // Creating socket file descriptor 

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("socket failed\n");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port  
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        printf("setsockopt\n");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    // Forcefully attaching socket to the port  
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        printf("bind failed\n");
        exit(EXIT_FAILURE);
    }
    printf("launching...\n");
    if (listen(server_fd, 10) < 0)
    {
   //     printf("listen");
        exit(EXIT_FAILURE);
    }
    fd_set socketfds;  //create select set
    int select_ret;
    while(1)
    {
    FD_ZERO (&socketfds);   //clean set
    FD_SET(server_fd,&socketfds); //add socket fd to the set

    reset(&tv); //once have timeout, the time should reinit. also as a function migrated point
    //listening server_fd socket file describtor, once have incomming socket rise the accept socket.
    select_ret = select (server_fd + 1, &socketfds, NULL, NULL, &tv);
    //printf("server_fd %d select %d\n",server_fd, select_ret);
    if (select_ret == -1 ) {printf("select error\n");continue;}
    if (FD_ISSET(server_fd , &socketfds)){
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                       (socklen_t*)&addrlen))<0)
    {
         perror("accept\n");
         exit(EXIT_FAILURE);
    }
    
    bzero(buffer, sizeof(buffer));
    valread = read(new_socket, buffer, writesize);
    rec_left = writesize - valread;
    buf_ptr = buffer;
    buf_ptr += valread; 
    while(rec_left > 0){
    	valread = read(new_socket, buf_ptr, rec_left); 
	if(valread <0) break;
	rec_left -= valread;
	buf_ptr += valread;
    }
    //end = clock();
    //printf("receive buffer from socket cost %f\n",((double) (end - start)) / CLOCKS_PER_SEC);
    fd=fopen("1.txt","w");
    start = clock();
    check = fwrite(buffer, writesize, 1, fd);
    if(check<0) { printf("file write error\n"); fclose(fd); continue; }
    end = clock();
    printf("write buffer to file cost %f ms\n",(((double) (end - start)) / CLOCKS_PER_SEC));
    bzero(buffer,sizeof(buffer));
    fclose(fd);
    start = clock();
    result = dealshit();
   end = clock();
    printf("zip time is %f ms\n",(((double) (end - start)) / CLOCKS_PER_SEC )*1000 );
    if (result ==0) send(new_socket,f_ack,3,0);
    else  send(new_socket,ack,3,0);
    close (new_socket);
    }}
    free(buffer);
    return 0;
}
