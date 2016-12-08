/*      (C)2000 FEUP  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

#define DEFAULT_PORT 21
#define PARSE_DOUBLESLASH 0
#define PARSE_DOUBLESLASH 0

struct linkInfo{
  char host[4048];
  char path[4048];
  char usr[4048];
  char pw[4048];
  char file[4048];
  unsigned short port;

};

/*Method to parse username and passwrod from an ftp link!*/
void parseUserData(char * link,unsigned int size, struct linkInfo * usrInfo){

  char * tmp;
  int n1, nm, n2;
  n1 = nm = n2 = 0;
  unsigned int slash = 0;
  int i = 0;
  for(; i < size; i ++){
    if(link[i] == '/')
      slash ++;
    if(slash)
      if(link[i]=='/')
        n1 = i;
      else if(link[i]==':')
        nm = i;
      else if(link[i]=='@'){
        n2 = i;
        break;
      }
  }

  if(n1 >= n2){
    usrInfo->usr[0] = '\0';
    usrInfo->pw[0] = '-';
    usrInfo->pw[1] = '\0';
    return;
  }

  memcpy(usrInfo->usr, link + n1 + 1, nm - n1 - 1);
  memcpy(usrInfo->pw, link + nm + 1, n2 - nm - 1);
  usrInfo->usr[nm - n1] = '\0';
  usrInfo->pw[n2 - nm] = '\0';
}
/*Method to parse the port and host*/
void parseHostPath(char* host, unsigned int size, struct linkInfo* info){
  int i = 0;
  int med = -1;
  char porta[25];

  for(; i < size; i++)
    if(host[i] == ':'){
        med = i;
        break;
    }

  if(med < 0){
    info->port = DEFAULT_PORT;
    return;
  }

  info->host[med] = '\0';

  memcpy(porta, host + med + 1, size - med );
  porta[size - med] = '\0';

  info->port = atoi(porta);
}

/*Method to parse hostname an ftp link beware call parseUserData before this one!!*/
void parseHostData(char * link, unsigned int size, struct linkInfo* info){
  unsigned char hasPw = info->usr[0] != '\0';
  int n1,n2;
  n1 = n2 = 0;
  int i = 0;


  if(hasPw){
    for(; i < size; i++){
      if(link[i]=='@')
        n1 = i;
      if(link[i]=='/' && n1 != 0){
        n2 = i;
        break;
      }
    }
  }else{
    memcpy(info->usr, (char*)"anonymous", 10);
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

  parseHostPath(info->host,n2-n1, info);
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

  unsigned int file = 0;
  unsigned int pSize = size-n;
  i = size-n;
  for(; i > -1; i--){
    if(info->path[i] == '/'){
      file = i;
      break;
    }
  }

  memcpy(info->file, info->path + file + 1, pSize - file);

  info->path[file + 1] = '\0';
  info->file[pSize-file + 1];



}


void parseLink(char * link, struct linkInfo* info){

  size_t length = strlen(link);
  int i = 0;

  unsigned char iState;
  parseUserData(link, length, info);
  parseHostData(link, length, info);
  parsePathData(link, length, info);

  printf("Link Given: %s\nUsr:%s\nPw:%s\nHost:%s\nPort:%d\nPath:%s\nFile:%s\n",
          link , info->usr, info->pw, info->host, info->port, info->path, info->file);

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

/*Comunicacao por ftp:
  enviar user:
                USER usrname
                  Receber resposta
                PASS password
                  Receber resposta
                CWD diretorio
                  Receber resposta
                GET ficheiro
                  Receber ficheiro
                  [Loop enquanto receber read* retorna]
                    read -1 entao merdou.
                QUIT fechar

*/
void test_ipParser(char * str){
  printf("\n[Testing]\n[%s]\n", str);
  struct linkInfo connectionInfo;
  parseLink(str, &connectionInfo);
  char* address = transformHostName(connectionInfo.host);

}

void ftp_error(char * msg){
  printf("Error!: %s", msg);
  exit(-1);
}

int ftp_send_cmd(int fd, char * buf, unsigned int size){
  int sent = 0;
  if((sent = send(fd,buf,size,0)) < 0){
    perror("send");
    return -1;
  }
  return sent;
}

void printData(char * buf, unsigned int size){
  printf("Data:");
  int i = 0;
  for(; i < size; i++){
    printf(" %x ", (char)buf[i]);
  }
  printf("\n");
}

/*Method to receive the status of the last ftp call*/
int ftp_receive_status(int fd){

  char status[19048];
  unsigned int received = 0;
  printf("Status:");
  while( (received = recv(fd, status , 19048,0)) ){

    if(received < 0){
      perror("Recv");
      return -1;
    }
    printf("%s", status);
    char* line = strtok((char *)status, "\n");
    char end = 0;
    while(line != NULL){
      if(line[0] >= '1' && line[0] <= '5' && line[3] ==' ')
        end = 1;
      line = strtok(NULL, "\n");
    }

    if(end)
      break;
  }
  printf("\n");

  return 1;
}

unsigned int ftp_cmd(char* buf, char* cmd, char* append){
  unsigned int cmdLen = strlen(cmd);
  unsigned int appendLen = strlen(append);

  memcpy(buf,cmd, cmdLen);
  memcpy(buf + cmdLen ,append, appendLen + 1);

  unsigned int size = cmdLen + appendLen + 1;
  buf[size] = '\r';
  buf[size + 1] = '\n';
  buf[size + 2] = '\0';

  return cmdLen + appendLen + 3;
}

void ftp_login(int fd, char* usr, char* pw){
  char buff[4028];
  printf("Login\n");
  unsigned int size = 0;

  size = ftp_cmd(buff, "USER ", usr);
  printf("%s\n", buff);
  if(ftp_send_cmd(fd, buff, size) < 0)
    ftp_error("Error Sending USER Command!");

  if(ftp_receive_status(fd) < 1)
    ftp_error("Error Receving USER confirmation!");

  size = ftp_cmd(buff, "PASS ", pw);
  if(ftp_send_cmd(fd, buff, size) < 0)
    ftp_error("Error Sending PASS Command!");

  if(ftp_receive_status(fd) < 1)
    ftp_error("Error Receving PASS confirmation!");

}

void ftp_changePath(int fd, char* path){
  char buff[4028];
  printf("Change Path\n");
  unsigned int size = 0;

  size = ftp_cmd(buff, "CWD ", path);
  if(ftp_send_cmd(fd, buff, size) < 0)
    ftp_error("Error Sending CWD Command!");

  if(ftp_receive_status(fd) < 1)
    ftp_error("Error Receving CWD confirmation!");

}

unsigned short ftp_parsePASV(char * ip, char* msg){
  int i1,i2,i3,i4,p1,p2;

  sscanf(msg, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &i1,&i2,&i3,&i4,&p1,&p2);

  sprintf(ip, "%d.%d.%d.%d", i1,i2,i3,i4);
  return p1 * 256 + p2;
}

int ftp_setPASV(int fd){
  char buff[1024];
  unsigned int size;
  size = ftp_cmd(buff, "PASV", "");
  if(ftp_send_cmd(fd, buff, size) < 0)
    ftp_error("Error Sending PASV Command!");

  char passvMSG[1024];
  if(recv(fd, passvMSG, 1024, 0) < 1)
    ftp_error("Error Receving PASV confirmation!");
  char address[1024];
  unsigned short port = ftp_parsePASV(address, passvMSG);

  return initConnection(address, port);
}

void ftp_getFile(int fd, char * file){
  unsigned char buff[20028];
  unsigned int fd2 = ftp_setPASV(fd);

  printf("Get File\n");
  unsigned int size = 0;


  size = ftp_cmd(buff, "TYPE I", "");
  if(ftp_send_cmd(fd, buff, size) < 0)
    ftp_error("Error Sending RETR Command!");

  if(ftp_receive_status(fd) < 1)
    ftp_error("Error Receving RETR confirmation!");


  size = ftp_cmd(buff, "RETR ", file);
  if(ftp_send_cmd(fd, buff, size) < 0)
    ftp_error("Error Sending RETR Command!");

  if(ftp_receive_status(fd) < 1)
    ftp_error("Error Receving RETR confirmation!");



  int dataReceived = 0;
  int vezes = 0;
  int fileFD = open( file, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
  if(fileFD < 0 ){ printf("Error Creating file! %s\n", file); exit(-1);}

  while( (dataReceived = read(fd2,buff, 20028))){
    if(dataReceived < 0){
      printf("Error! Downloading the file!");
      exit(-1);
    }
    int dataWritten = 0;
    printf("Data: %d - %d\n", dataReceived, vezes++);

    while( dataWritten < (dataReceived) ){
      dataWritten += write(fileFD, buff + dataWritten, dataReceived - dataWritten);
    }
  }
  close(fileFD);
  close(fd2);
}

void ftp_disconnect(int fd){
  char buff[20];
  unsigned int size;
  if(ftp_receive_status(fd) < 1)
    ftp_error("Error Receving Download confirmation!");

  size = ftp_cmd(buff, "QUIT ", "");
  if(ftp_send_cmd(fd, buff, size) < 0)
    ftp_error("Error Sending QUIT Command!");

  if(ftp_receive_status(fd) < 1)
    ftp_error("Error Receving QUIT confirmation!");

  close(fd);
}

int main(int argc, char** argv){

	int	sockfd;

	char	buf[2];
  unsigned short port = 21;
	int	bytes;
  /*
  test_ipParser("ftp://user:password@fe.up.pt:34/path/file.txt");
  test_ipParser("ftp://user:password@fe.up.pt/path/file.txt");
  test_ipParser("ftp://fe.up.pt/path/file2.txt");
  test_ipParser("ftp://fe.up.pt:870/path/file2.txt");
  test_ipParser("ftp://fe.up.pt:870/file2.txt"); */

  if(argc != 2){
    printf("Invalid Arguments!\nUsage: %s <URL>\n", argv[0]);
    return -1;
  }
  struct linkInfo connectionInfo;
  /*struct linkInfo{
    char host[4048];
    char path[4048];
    char usr[4048];
    char pw[4048];
    char file[4048];
    unsigned short port;

  }; */
  parseLink(argv[1], &connectionInfo);

  char* address = transformHostName(connectionInfo.host);
  printf("Init connection!\n");
  sockfd = initConnection(address, connectionInfo.port);
  printf("Connected!\n");

  char buff2[2000];
  recv(sockfd,buff2,2000, 0);
  printf("Buff2 Info: %s\n", buff2);

  ftp_login(sockfd, connectionInfo.usr, connectionInfo.pw);
  ftp_changePath(sockfd, connectionInfo.path);
  ftp_getFile(sockfd, connectionInfo.file);
  ftp_disconnect(sockfd);
	close(sockfd);
	exit(0);
}
