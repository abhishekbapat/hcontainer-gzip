#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#define PORT 25659 
   
int main(int argc, char const *argv[]) 
{ 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    FILE* fd;
    clock_t start, end;
    unsigned theTick = 0U;
    int mega_bytes =  1024, send_left;
    char *buffer = (char *) malloc(300 * mega_bytes + 10), *buf_ptr;
    buf_ptr = buffer; 
    char *ackbuf = (char *) malloc(3);
    int readsize = 300 * mega_bytes;
    int send_count;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
//        printf("Socket creation error \n"); 
        return -1; 
    } 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)  
    { 
//        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
//	printf("false to connect sock\n");
        return -1; 
    }
    fd=fopen("1.txt","r");
    if(fd<0) { free(buffer); free(ackbuf); free(fd);return -1;}
    bzero(buffer,sizeof(buffer));
    fread(buffer,readsize,1,fd);
    if(fread<0) { free(buffer); free(ackbuf); free(fd); return -1; }
    send_left = readsize;
    
    while (send_left>0)
    {
    	send_count = send(sock,buf_ptr,send_left,0);
    	if(send_count<0) { break; } 
	send_left -= send_count;
	buf_ptr += send_count;
	//printf("send_count %d send_left %d\n", send_count, send_left);
    }
    bzero(buffer,sizeof(buffer));
    //printf("wait for ack\n");
    start = clock();
    while(1) {
        bzero(ackbuf,sizeof(ackbuf));
	read(sock,ackbuf,sizeof(ackbuf)); 
	if(ackbuf[0] =='A'){
	//printf( "%c%c%c\n",ackbuf[0],ackbuf[1],ackbuf[2]); 
 	break;}
	end = clock();
        if((end - start)>=2) {
         printf("2 sec timeout!\n"); free(buffer); free(ackbuf); free(fd); return -1;
	} }
    //printf("receive %s\n",ackbuf); 
    free(buffer); 
    free(ackbuf); 
    free(fd);
    return 0; 
} 

