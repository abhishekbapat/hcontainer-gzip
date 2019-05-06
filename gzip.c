/* This work is free. I, the authour, release this work into the public domain.
 * 
 * This work comes AS IS, WITH ALL FAULTS, WITH NO WARRANTY OF ANY KIND WHATSOEVER, not even the implied warranties of MERCHANTABILITY, FITNESS, and TITLE.
 * In NO CASE SHALL THE AUTHOUR BE LIABLE for ANY DAMAGES OR HARMS CAUSED BY OR LINKED TO THIS WORK.
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


#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define BUFFER_SIZE 1024 


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
	
	memset (&s, 0, sizeof (z_stream));
	int windowBits = -MZ_DEFAULT_WINDOW_BITS;
	if (level == 0 ? inflateInit2 (&s,                     windowBits)
	               : deflateInit2 (&s, level, MZ_DEFLATED, windowBits, 6, MZ_DEFAULT_STRATEGY)) {
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
		int n;
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
	write (ofd, hdr, 10);
	st = go (level, ifd, ofd);
	storLE32 (ftr + 0, st.crc32);
	storLE32 (ftr + 4, st.l);
	write (ofd, ftr, 8);
}

void print_iteration()
{
        static int i = 0;
        printf("[%d] hetpot is waiting for compression  %d\n", getpid(), i++);
}


int main (void) {




    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    char buffer[BUFFER_SIZE];


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        bzero(buffer, BUFFER_SIZE);
        sin_size = recv(new_fd, buffer, BUFFER_SIZE,0);
        if (sin_size < 0)
        {
           printf ("Server Receive Data Failed!\n");
           continue;
        }
        else
        {
           
            int level = 6 /* zip level; 0 to mean unzip */, flags = 0, ii;
            char **ss, **ts;
            const char * split = ",";
            char * p;
            int argc =0;
            char *argu[1028];
            p = strtok(buffer, split);
             
            while(p!=NULL){
           
            argu[ii] = (char*)malloc( strlen(p)+1);
            strcpy(argu[ii], p);
            argc++;
            p = strtok(NULL, split);
            ii++;
            }


             for (ii = 1; ii < argc; ii++) {
                 if (argu[ii][0] != '-' || argu[ii][1] == 0) break;
                 if (argu[ii][1] == '-' && argu[ii][2] == 0) {
                        ii++;
                        break;
                 }
                 for (int jj = 1; argu[ii][jj]; jj++) switch (argu[ii][jj]) {
                 case 'c':
                        flags |= cFlag;
                        break;
                 case 'd':
                        level = 0;
                        break;
                 default:
                        if (argu[ii][jj] <= '9' && argu[ii][jj] >= '0') level = (int)(argu[ii][jj] - '0');
                        break;
                 }
            }

            argu[--ii] = argu[0];
            argc -= ii;
      //      argu = argu + ii;
            ss = argu;
            if (!flags & cFlag) {
                ts = malloc (sizeof (char *)*(argc - 1));
                if (!ts) err (-1, "failed to allocate memory");
                for (int ii = 1; ii < argc; ii++) {
                        if (level == 0) {
                                char *p;
                                printf("s1s[ii]:  %s\n", ss[ii]);    
                                ts[ii] = strdup (ss[ii]);
                                printf("ts[ii]:    %s\n", ts[ii]);
                                if (!ts[ii]) err (-1, "failed to allocate memory");
                                p = strrchr (ts[ii], '.');
                                if (!p) errx (-1, "%s: no \".gz\" suffix", ts[ii]);
                                p[0] = 0;
                        }
                        else {
                                ts[ii] = malloc (strlen (ss[ii]) + 4);
                                
                                strcpy (ts[ii], ss[ii]);
                                strcat (ts[ii], ".gz");
                        }
                }
            }


            for (int i = 0; i < 6; i++)
            {
               print_iteration();
               sleep(1);
            }





            for (int ii = 1; ii < argc; ii++) {
                int ifd, ofd;
                int sfd;
                struct stat st;
                st.st_blksize = 521;
                st.st_blocks = 521;
//              if (stat (ss[ii], &st) < 0) err (-1, "failed to stat %s:", ss[ii]);

                ifd = open (ss[ii], O_RDONLY);
                ofd = flags & cFlag ? 1 : open (ts[ii], O_WRONLY | O_CREAT, st.st_mode);
                sfd = open(ts[ii], O_WRONLY | O_CREAT, st.st_mode);
                if (ifd < 0 || ofd < 0) err (-1, "failed to open");

                printf("Hetpot starts compression\n");
                if (level == 0) ungz (ifd, ofd);
                else gz (level, ifd, ofd);
            }

            if (argc <= 1) {
                if (level == 0) ungz (0, 1);
                else gz (level, 0, 1);
            }






    }



        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Finish Compression", 23, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

  
    return 0;
}
