/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<signal.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define F 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP 5

char buffer[5] = {F,A, C_UA, A ^ C_UA, F};

int fileID;

void sendBytes(int fd, char* buf, int tamanho){
    printf("SendBytes Initialized\n");
    int escrito = 0;
    int offset = 0;
    while( (escrito = write(fd,buf + offset,tamanho)) < tamanho ){
        offset += escrito;
        tamanho -= escrito;
    }
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
                        if(buf[1] ^ buf[2] == readed){
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
    return buf[4];
}

void timeOut(){
    sendBytes(fileID, buffer, 5);
    alarm(3);
}

int main(int argc, char** argv)
{

	//signal(SIGALRM, timeOut);

    int fd,c, res;
    struct termios oldtio,newtio;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    //receive

    int i = 0;

	//Leitura
	printf("%X\n",parseSupervision(fd));

	//Resending
	sendBytes(fd,buffer,5);

    sleep(2);

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
