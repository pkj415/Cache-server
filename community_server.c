/*webserve.c*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#define HTTP_Port 80
#define CHUNKSIZE 512
#define BUFFER_SIZE 2048
#define MAX_NUMBER_OF_THREADS 50

FILE *cache;

pthread_mutex_t lock;
pthread_t thds[MAX_NUMBER_OF_THREADS];
struct param{
	long int lower;
	long int upper;
};
struct downloadInfo{
	char *host;
	char *uri;
	char *fileNameFromURI;
	FILE *fp;
};
struct downloadInfo dnldInfo;
void Die(char *str){
	perror(str);
	exit(0);
}
int counter=0;
int socket_connect(char *host, in_port_t port){
    struct hostent *hp;
    struct sockaddr_in addr;
    int on = 1, sock;     
    

    if((hp = gethostbyname(host)) == NULL){
        herror("gethostbyname");
        return -1;
    }

    bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(sock == -1){
        perror("setsockopt");
        return -1;
    }
    
    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        return -1;

    }
    return sock;
}

void *startDownload(void *arg){
	printf("Running thread \n");
	struct param *parm= (struct param*)arg;
		char *y;
    	long int x=0;
        int fd = socket_connect(dnldInfo.host, 80);
        char request[1000];
        sprintf(request, "GET %s HTTP1.1\r\nHost: %s\r\nRange: bytes=%lu-%lu\r\n\r\n", dnldInfo.uri, dnldInfo.host, parm->lower, parm->upper);
        if(write(fd, request, strlen(request))<0){
        	Die("Request failed from Thread for data in range.\n");
        }
        char buffer[2048];
        bzero(buffer, BUFFER_SIZE);
        
        
        x=read(fd, buffer, sizeof(buffer));
        if(x<0){
        	Die("Thread read failed from server.\n");
        }
        //printf("DOWNLOAD RESPONSE : %s\n",buffer);
        y=buffer;

                char c;
                int flagFound=0;
                int offset=0;int pointer=0;
                while(1){
                        if(buffer[pointer]=='\r'&&buffer[pointer+1]=='\n'&&buffer[pointer+2]=='\r'&&buffer[pointer+3]=='\n'){
                            printf("FOUND header end\n");
                            break;
                        }
                        pointer++;
                }
                offset++;pointer=pointer+4;
                char newBuff[2048];
                memset(newBuff, 0, sizeof(newBuff));
                memcpy(newBuff, buffer+pointer, x-pointer);
        

        long int cnt=0;
        

        
        x=x-pointer;
        while(1){
        	if((parm->lower)/500<=counter){
        		pthread_mutex_lock(&lock);
        		fwrite(newBuff,x,1,dnldInfo.fp);
        		fflush(dnldInfo.fp);
        		pthread_mutex_unlock(&lock);
        		counter++;
        		break;	
        	}	
        }
        printf("Thread ended\n");
        return(NULL);
}
int downloadFile2(char *host, char* uri, char *fileNameFromURI){

	
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
	    printf("\n mutex init failed\n");
	    return -1;
	}
	//printf("HELLO1\n");
	printf("\nStarting Download...\n");
    long int lm=0,hm=499,mcnt=0,content_len;

    
    FILE *fp = fopen(fileNameFromURI,"wb");
    fclose(fp);
	fp = fopen(fileNameFromURI,"ab");
	dnldInfo.fp = fp;
	dnldInfo.host = host;
	dnldInfo.uri = uri;
	dnldInfo.fileNameFromURI = fileNameFromURI;
    //printf("HELLO1\n");
    char buffer[BUFFER_SIZE];
    
    char request[1000], fsize[10];

       
    int fd = socket_connect(host, 80); 
    if(fd==-1){
    	return -1;
    }
    bzero(request, 1000);
    sprintf(request, "GET %s HTTP1.1\r\nHost: %s\r\nRange: bytes=0-0\r\n\r\n", uri, host);

    write(fd, request, strlen(request)); // write(fd, char[]*, len); 

    //printf("HELLO2\n");
    bzero(buffer, BUFFER_SIZE);

    if(read(fd, buffer, sizeof(buffer))<0){
    	Die("Initial read for size failed fron server.\n");
    }


    //printf("%s\n",buffer );
    // exit(0);

    int i=0;

    int pos= strstr(buffer, "Content-Range: bytes") - buffer;

    while(buffer[pos++] != '/'){}

    while(buffer[pos] != ' ')
        fsize[i++] = buffer[pos++];
    content_len = atoi(fsize);

    int numOfThreads = ceil(content_len/(float)500);
	struct param prms[numOfThreads];
	for(i=0;i<numOfThreads;i++){
		prms[i].lower = lm+mcnt;
		prms[i].upper = hm+mcnt;
		//sleep(1);
		if(pthread_create(&(thds[i]), NULL, startDownload, (void*)(prms+i))==-1){
			Die("Error creating threads.\n");
		}
		mcnt+=500;
	}
	for(i=0;i<numOfThreads;i++){
		pthread_join(thds[i],NULL);
	}
	//
    printf("Total bytes transferred - %lu\n",content_len );
    printf("Download ended.\n");

    close(fd); 
    fclose(fp);

    return 0;
}
void sendFileToBrowser(int connfd, char *fileName){
				char sendBuff[20000];
				char toBeSentData[CHUNKSIZE];
				FILE* fileFD = fopen(fileName, "r");
				fseek(fileFD, 0, SEEK_END); // seek to end of file
				int size = ftell(fileFD); // get current file pointer
				fseek(fileFD, 0, SEEK_SET); // seek back to beginning of file
				memset(sendBuff, 0, sizeof(sendBuff));
				memset(toBeSentData, 0, sizeof(toBeSentData));
				sprintf(sendBuff, "HTTP/1.1 200 OK\nAccept-Ranges: bytes\nConnection: Keep-Alive\nContent-Length: %d\nContent-Type: application/octet-stream\nTransfer-Encoding: chunked\nContent-Disposition: attachment; filename=\"%s\"\r\n\r\n", size, fileName);
				//strcpy(sendBuff, "HTTP/1.1 200 OK\nContent-Length: 18\nAccept-Ranges: bytes\nConnection: Keep-Alive\nContent-Type: text/html\n\n<html>Hello</html>");
				printf("Response to browser : %s", sendBuff);
				if(send(connfd, sendBuff, strlen(sendBuff), 0)<0){
					Die("File send to browser failed.\n");
				}
				int numAvail;
				int totalSize = 0;
				while((numAvail=read(fileno(fileFD), toBeSentData, CHUNKSIZE))>0){
						//printf("Read Data\n");
						//printf("LENGTH %d",numAvail);
						totalSize = totalSize + numAvail;
						memset(sendBuff, '\0', sizeof(sendBuff));
						sprintf(sendBuff, "%x\r\n", numAvail);
						//printf("LENGTH : %s", sendBuff);
						if(send(connfd, sendBuff, strlen (sendBuff), 0)==-1)
							Die("Send failed to browser\n");
						//toBeSentData = strcat(toBeSentData, "\r\n");
						//printf("%s\r\n",toBeSentData);
						if(send(connfd, toBeSentData, numAvail, 0)==-1)
							Die("Send failed to browser\n");
						if(send(connfd, "\r\n", sizeof("\r\n"), 0)==-1)
							Die("Send failed to browser\n");						
						
						memset(toBeSentData, 0, sizeof(toBeSentData));            
						//printf("Sent to browser%s\n", sendBuff);
				}
				printf("Total size : %d",totalSize);
				memset(sendBuff, 0, sizeof(sendBuff));
				sprintf(sendBuff, "0\r\n\r\n");
				if(send (connfd, sendBuff, strlen (sendBuff), 0)==-1)
							Die("Send failed to browser\n");
}

int main (int argc, char **argv)
{
	if(argc!=2)
	{
		printf("Format is :- %s <port_number_to_run_the_proxy_cache_on>", argv[0]);
		exit(0);	
	}
	cache = fopen("cache.txt", "a+");
	int connfd, sd, i = 0;
	
	socklen_t proxyClientLen;
	char sendBuff[20000], recvBuff[20000];
	struct sockaddr_in proxyClientAddr, proxyServAddr;
	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
			perror ("sockfd error");
	}

	bzero (&proxyServAddr, sizeof (proxyServAddr));
	proxyServAddr.sin_family = AF_INET;
	proxyServAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	proxyServAddr.sin_port = htons (atoi (argv[1]));
	int optval = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (bind (sd, (struct sockaddr *) &proxyServAddr, sizeof (proxyServAddr)))
		{
			perror ("Bind error");
		}
	listen (sd, 30);

	//Working on request from browser
	for (;;)
	{
			proxyClientLen = sizeof (proxyClientAddr);
			if ((connfd = accept (sd, (struct sockaddr *) &proxyClientAddr, &proxyClientLen)) < 0)
			{
				perror ("connection error");
			}
			if(fork()==0){
				close(sd);
				memset(recvBuff, 0, sizeof(recvBuff));
				if(recv (connfd, recvBuff, sizeof (recvBuff), 0)<0){
					Die("Error receiving request from client.\n");
				}
				printf("REQUEST = \n%s",recvBuff);
				char host[200], uri[200], remaining[5000], checkUri[200], fileName[200];
				memset(host, 0, sizeof(host));
				memset(uri, 0, sizeof(uri));
				memset(remaining, 0, sizeof(remaining));
				memset(checkUri, 0, sizeof(checkUri));
				memset(fileName, 0, sizeof(fileName));
				sscanf(recvBuff,"GET %s HTTP/1.1 \nHost: %s User-Agent: %s",uri, host, remaining);
				if(strlen(host)==0||strlen(uri)==0){
					exit(0);
				}
				printf ("GET %s HTTP/1.1 \nHost: %s User-Agent: %s\n",uri, host, remaining);
				
				int flagFound=0;
				lseek(fileno(cache), 0, SEEK_SET);
				while(fscanf(cache, "%s %s",checkUri, fileName)!=EOF){
					//printf("Comparing %s and %s",checkUri, uri);
					if(strcmp(checkUri, uri)==0){
						flagFound=1;
						break;
					}
				}
				if(flagFound==1){
					printf("File found on local cache!!!!!\n");
					sendFileToBrowser(connfd, fileName);
					printf("\nSend to client complete\n");
					close (connfd);
					exit(0);  
				}
				if(flagFound==0){
					memset(sendBuff, 0, sizeof(sendBuff));
					int strlengs = 13+65+strlen(uri)+strlen(inet_ntoa(proxyClientAddr.sin_addr))+1;
					sprintf(sendBuff, "HTTP/1.1 200 OK\nServer: Apache/1.2.6 Red Hat\nContent-Length: %d\nAccept-Ranges: bytes\nKeep-Alive: timeout=15, max=100\nConnection: Keep-Alive\nContent-Type: text/html\n\n<html>Request from %s:%s <br>File not Available. Request Again after some Time.</html>", strlengs, uri, inet_ntoa(proxyClientAddr.sin_addr));
					if(send(connfd, sendBuff, strlen(sendBuff), 0)<0){
						Die("Send to client requesting file failed.\n");
					}
				}
				//strcpy(sendBuff, "HTTP/1.1 200 OK\nDate: Mon, 19 Oct 2009 01:26:17 GMT\nServer: Apache/1.2.6 Red Hat\nAccept-Ranges: bytes\nKeep-Alive: timeout=15, max=100\nConnection: Keep-Alive\nContent-Type: application/zip\nTransfer-Encoding: chunked\n29\n<html><body><p>The file you requested is\n5\n3,400\n23\nbytes long and was last modified:\n1d\nSat, 20 Mar 2004 21:12:00 GMT\n13\n.</p></body></html>\n0\r\n");
				//send (connfd, sendBuff, strlen (sendBuff), 0);  
				close (connfd);
				char *fileNameFromURI = strrchr(uri, '/');
				fileNameFromURI++;
				
				if(downloadFile2(host, uri, fileNameFromURI)==-1){
					printf("Error downloading from remote server.\n");
				}
				lseek(fileno(cache), 0, SEEK_END);
				fprintf(cache, "%s %s\n", uri, fileNameFromURI);
				fflush(cache);
				printf("Added file entry to cahce.\n");
				exit(0);
			}
			else{
				close(connfd);
			}
	}
}
