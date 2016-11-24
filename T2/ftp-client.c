/*      (C)2000 FEUP  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

#define SERVER_PORT 6000

#define PARSE_DOUBLESLASH 0
#define PARSE_DOUBLESLASH 0

struct linkInfo{
  char host[4048];
  char path[4048];
  char usr[4048];
  char pw[4048];
};

/*Method to parse username and passwrod from an ftp link!*/
void parseUserData(char * link,unsigned int size, struct linkInfo * usrInfo){

  char * tmp;
  int n1, nm, n2;
  n1 = nm = n2 = 0;

  int i = 0;
  for(; i < size; i ++){
    if(link[i]=='[')
      n1 = i;
    else if(link[i]==':')
      nm = i;
    else if(link[i]=='@'){
      n2 = i;
      break;
    }
  }

  if(n1 == n2){
    usrInfo->usr[0] = '\0';
    usrInfo->pw[0] = '\0';
    return;
  }

  memcpy(usrInfo->usr, link + n1 + 1, nm - n1 - 1);
  memcpy(usrInfo->pw, link + nm + 1, n2 - nm - 1);
  usrInfo->usr[nm - n1] = '\0';
  usrInfo->pw[n2 - nm] = '\0';
}
/*Method to parse hostname an ftp link beware call parseUserData before this one!!*/
void parseHostData(char * link, unsigned int size, struct linkInfo* info){
  unsigned char hasPw = info->usr[0] != '\0';
  int n1,n2;
  n1 = n2 = 0;
  int i = 0;
  if(hasPw){
    for(; i < size; i++){
      if(link[i]==']')
        n1 = i;
      if(link[i]=='/' && n1 != 0){
        n2 = i;
        break;
      }
    }
  }else{
    int slash = 0;
    for(;i < size; i++){
      if(link[i] == '/')
        slash++;

      if(link[i] == '/'){
        switch (slash) {
          case 2:
            n1 = i;
            break;
          case 3:
            n2 = i;
            i = size; //End loop!
            break;
        }
      }
    }
  }
  memcpy(info->host, link + n1 + 1, n2 - n1 - 1);
  info->host[n2 - n1] = '\0';
}
/*Method to parse path an ftp link beware call parseUserData before this one!!*/
void parsePathData(char * link, unsigned int size, struct linkInfo* info){
  int n = 0;
  int i = 0;
  int slash = 0;

  for(; i < size; i++){
    if(link[i] == '/')
      slash++;

    if(slash == 3){
      n = i;
      break;
    }

  }

  memcpy(info->path, link + n, size - n);
  info->path[size-n + 1] = '\0';
}


void parseLink(char * link, struct linkInfo* info){

  size_t length = strlen(link);
  int i = 0;

  unsigned char iState;

  parseUserData(link, length, info);
  parseHostData(link, length, info);
  parsePathData(link, length, info);

  printf("Link Given: %s\nUsr:%s\nPw:%s\nHost:%s\nPath:%s\n",
          link , info->usr, info->pw, info->host, info->path);

}

int initConnection(char* address, unsigned short port){
  /*server address handling*/
  struct	sockaddr_in server_addr;
  bzero((char*)&server_addr,sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(address);
  server_addr.sin_port = htons(port);

  int sockfd;
  /*open an TCP socket*/
  if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socket()");
          exit(0);
      }
  /*connect to the server*/
      if(connect(sockfd,
             (struct sockaddr *)&server_addr,
       sizeof(server_addr)) < 0){
          perror("connect()");
    exit(0);
  }
  return sockfd;
}

/* Function to atribute an IP address using a given hostname.*/
char * transformHostName(char * name){

  struct hostent *h;

  printf("\nTransforming host name!\n");


  if ((h=gethostbyname(name)) == NULL) {
      herror("gethostbyname");
      exit(1);
  }
  char * addr = inet_ntoa(*((struct in_addr *)h->h_addr));
  printf("Host name  : %s\n", (char*)h->h_name);
  printf("IP Address : %s\n",addr);

  return addr;
}



int main(int argc, char** argv){

	int	sockfd;

	char	buf[2];
  unsigned short port = 21;
	int	bytes;

  if(argc != 2){
    printf("Invalid Arguments!\nUsage: %s <URL>", argv[0]);
  }
  struct linkInfo connectionInfo;

  parseLink(argv[1], &connectionInfo);

  char* address = transformHostName(connectionInfo.host);

  sockfd = initConnection(address, port);

  /*send a string to the server*/
	bytes = write(sockfd, buf, strlen(buf));
	printf("Bytes escritos %d\n", bytes);
  /* */

	close(sockfd);
	exit(0);
}
