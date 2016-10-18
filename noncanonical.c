/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define BIT(n) (0x01 << n)

#define F 0x7E
#define A 0x03
#define A_SENDER 0x03
#define A_RECIVER 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_INFO BIT(6)

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP 5
#define TRANSMITTER O_WRONLY
#define RECEIVER O_RDONLY
#define ESCAPE 0x7d
#define ESCAPE_XOR 0x20


#define C_RR0 0x05
#define C_RR1 (BIT(7) | C_RR0)

#define C_REJ0 0x01
#define C_REJ1 (BIT(7) | C_REJ0)

#define RS(n) (n >>7 )

#define PACKAGE_LENGTH 5

#define SupervisionSize 5

struct buffer{
	char * buffer;
	int bufSize;
} lastBuffer;

int fileID;


struct termios oldtio;


char * getSupervisionBuf(char address, char control){
	char* buff = (char*)malloc(sizeof(char) * SupervisionSize);
	buff[0] = F;
	buff[1] = address;
	buff[2] = control;
	buff[3] = buff[1] ^ buff[2];
	buff[4] = F;
	return buff;
}

int bytesDuped(char * buf, unsigned int length){
	int i = 1;
	int res = 0;
	for(; i < length-1; i++){
		if(buf[i] == 0x7d){
			res++;
			i++;
		}
	}
	return res;
}

char* byteDesStuffing(char * buf,unsigned int length, unsigned int * rLen){
	unsigned int len = length - bytesDuped(buf, length);
	*rLen = len;
	char* res = (char *)malloc(sizeof(char) *  len);
	int i = 0;
	int n = 0;
	for(; i < length; i++){

		if(buf[i] == ESCAPE){

			res[n] = buf[i+1] ^ ESCAPE_XOR;
			i++;
		}else{
			res[n] = buf[i];
		}

		n++;
	}
	return res;
}

char* byteStuffing(char * buf,unsigned int length, int stuffingLength, unsigned int * rLen){
	unsigned int lengthR = length + stuffingLength;
	char * res = (char*)malloc(sizeof(char)* (lengthR));
	int offset = 0;
	int i = 0;
	(*rLen) = lengthR;
	for(; i < length; i++){
		if(i == 0 || i == length-1){
			res[i+offset] = buf[i];
		}else if(buf[i] == F || buf[i] == ESCAPE){
			res[i+offset] = ESCAPE;
			offset++;
			res[i+offset] = buf[i] ^ ESCAPE_XOR;
		}else
			res[i+offset] = buf[i];
	}
	return res;
}

char * getDataBuf(char addr, char ns, char * data,unsigned int length, unsigned int* rLen){

	int dataLength = (length < PACKAGE_LENGTH)? length : PACKAGE_LENGTH;

	int totalLength = 5 + dataLength + 1;
	int buffSize = sizeof(char) * totalLength;
	char* buff = (char*)malloc(buffSize);

	buff[0] = F;
	buff[1] = addr;
	buff[2] = C_INFO;
	buff[3] = buff[1] ^ buff[2];

	unsigned char bcc2 = data[0];
	int stuffSize = 0;
	int i = 0;
	for(; i < dataLength; i++){
		buff[ 4 + i ] = data[i];
		if(i > 0)
			bcc2 ^= data[i];
		if(data[i] == F || data[i] == ESCAPE)
			stuffSize++;
	}

	buff[4 + dataLength] = bcc2;
	if(bcc2 == F || bcc2 == ESCAPE)
		stuffSize++;

	buff[5 + dataLength] = F;

	char * res = byteStuffing(buff, totalLength, stuffSize,rLen);
	free(buff);
	return res;
}

void sendBytes(int fd, char* buf, int tamanho){
	printf("SendBytes Initialized\n");
	int escrito = 0;
    int offset = 0;
    while( (escrito = write(fd,buf + offset,tamanho)) < tamanho ){
        offset += escrito;
        tamanho -= escrito;
    }
}

void sendLastBuffer(int fd, struct buffer* buffer){
	sendBytes(fd, buffer->buffer, buffer->bufSize);
}

void setBuffer(struct buffer* buffer, char * newBuffer, int newSize){
	free(buffer->buffer);
	buffer->buffer = newBuffer;
	buffer->bufSize = newSize;
}

char parseSupervision(int fd){
	char buf[5];
	char status = START;
	while(status != STOP){
		char readed;
		read(fd, &readed, 1);

		switch(status){
			case START:
				if(readed == F){
					status = FLAG_RCV;
					buf[0] = readed;
				}
				break;
			case FLAG_RCV:
				switch(readed){
					case F:
						break;
					case A:
						status = A_RCV;
						buf[1] = readed;
						break;
					default:
						status = START;
						break;
				}
				break;
			case A_RCV:
				switch(readed){
					case F:
						status = FLAG_RCV;
						break;
					case (char)C_RR0:
					case (char)C_RR1:
					case (char)C_REJ0:
					case (char)C_REJ1:
					case C_SET:
					case C_UA:
						status = C_RCV;
						buf[2] = readed;
						break;
					default:
						status = START;
						break;
				}
				break;
			case C_RCV:
				switch(readed){
					case F:
						status = FLAG_RCV;
						break;
					default:
						if((buf[1] ^ buf[2]) == readed){
							buf[3] = readed;
							status = BCC_OK;
						}else
							status = START;
						break;
				}
				break;
			case BCC_OK:
				switch(readed){
					case F:
						buf[4] = readed;
						status = STOP;
						break;
					default:
						status = START;
						break;
				}
				break;
		}
	}
	alarm(0);
	return buf[2];
}

void timeOut(){

	sendLastBuffer(fileID,&lastBuffer);
	printf("Alarm\n");
	alarm(3);
}


int initTransmitter(){
	char * bufToSend = getSupervisionBuf(A,C_SET);

	setBuffer(&lastBuffer, bufToSend , SupervisionSize);
	sendLastBuffer(fileID,&lastBuffer);
	alarm(3);

  char content = parseSupervision(fileID);

	return (content == C_UA);
}

int llopen(int porta, unsigned char side){

	if(porta < 0 || porta > 3){
		printf("Error: port range is [0;3] given: %d \n", porta);
		return -1;
	}
    int fd;
    struct termios newtio;


	char nomePorta[11];
	strcpy(nomePorta, "/dev/ttyS0");
	nomePorta[9] = '0' + porta;

    fd = open(nomePorta, O_RDWR | O_NOCTTY );

	printf("port %s  %d \n", nomePorta,porta);

    if (fd <0) {
		perror(nomePorta);

		exit(-1);
	}

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0; //OPOST;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

	fileID = fd;

	if(side == TRANSMITTER){
		if(!initTransmitter()){
			close(fd);
			exit(-1);
		}
	}

	return fd;
}

int llclose(int fd){

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
	int res = close(fd);
	if(res == 0)
		return 1;
	return res;
}

int llread(int fd, char* buff){

	char *bufftmp= (char*) malloc(sizeof(char)* (5+1+PACKAGE_LENGTH));
	int i=0;
	while (read(fd,&bufftmp[i],1) != 0) {
		i++;
	}


	unsigned int rlen;
	byteDesStuffing(buff,length,&rlen);

	//parse da trama e verificação bcc




	return 0;
}

int llwrite(int fd, char* buffer, int length){

	char *data = buffer;
	unsigned int maxLen = length/PACKAGE_LENGTH;
	unsigned int actualFrame = 0;
	int i;
	for(i = 0 ; i < maxLen + 1; i++){

		unsigned int len = PACKAGE_LENGTH;

		if( i == maxLen){
			len = length % PACKAGE_LENGTH;
			if( len == 0)
				break;
		}
		unsigned int frameLength = 0;
		char * frame =  getDataBuf(A, 0, data,len, &frameLength);

		while(1){

			setBuffer(&lastBuffer, frame, frameLength);
			sendLastBuffer(fd, &lastBuffer);
			alarm(3);
			//Wait RR
			char res = parseSupervision(fd);

			if(actualFrame != RS(res)){
				break;
			}else{
				continue;
			}
		}

		data += PACKAGE_LENGTH;
	}

	return 0;
}

void debugChar(char* bytes, int len){
	printf("Elements: %d , 0x" , len);
	int i = 0;

	for(;i < len; i++){
		printf(" %02x ", (unsigned char)bytes[i]);
	}
	printf("\n");
}

int main(int argc, char** argv)
{

	char bytes []= {F, ESCAPE,0x55,0x11,0xAA};
	unsigned int length = 0;

	debugChar(bytes, 5);

	char* res = getDataBuf(A, 0, bytes, 5, &length);
	debugChar(res, length);

	unsigned int len = 0;
	char* res2 = byteDesStuffing(res, length, &len);
	debugChar(res2, len);


	if(argc != 2){
		printf("You need to specify the port number\n");
		return -1;
	}
	signal(SIGALRM, timeOut);

	fileID = llopen(atoi(argv[1]), TRANSMITTER);

	printf("Sending Data : ");

	if(llclose(fileID) < 0){
		perror("Error closing fileID");
	}

    printf("\n Done!\n");
    return 0;
}
