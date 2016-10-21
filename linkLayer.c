/*Non-Canonical Input Processing*/
#include "linkLayer.h"


struct buffer{
	char buffer[MAX_PACKAGE_SIZE];
	int bufSize;
} lastBuffer;

int fileID;
unsigned int sideMacro;
char internal_state = 0;

struct termios oldtio;
unsigned char ns = 0;


void getSupervisionBuf(char * buffer, char address, char control){
	buffer[0] = F;
	buffer[1] = address;
	buffer[2] = control;
	buffer[3] = buffer[1] ^ buffer[2];
	buffer[4] = F;
}

int bytesDuped(char * buf, unsigned int length){
	int i = 1;
	int res = 0;
	for(; i < length-1; i++){
		if(buf[i] == ESCAPE){
			res++;
			i++;
		}
	}
	return res;
}

unsigned int byteDesStuffing(char* res, char * buf,unsigned int length){
	unsigned int len = length - bytesDuped(buf, length);

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
	return len;
}

unsigned int byteStuffing(char * res, char * buf,unsigned int length, int stuffingLength){

	unsigned int lengthR = length + stuffingLength;
	int offset = 0;
	int i = 0;
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
	return lengthR;
}
//	char res[MAX_PACKAGE_SIZE];
unsigned int getDataBuf(char * res, char addr, char ns, char * data,unsigned int length){

	int dataLength = (length < PACKAGE_LENGTH)? length : PACKAGE_LENGTH;

	int totalLength = 5 + dataLength + 1;

	char buff[MAX_PACKAGE_DESTUFFED_SIZE];

	buff[0] = F;
	buff[1] = addr;
	buff[2] = C_INFO(ns);
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

	return byteStuffing(res, buff, totalLength, stuffSize);

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
	memcpy(buffer->buffer,newBuffer,newSize);
	buffer->bufSize = newSize;
}

unsigned int getDataFromBuffer(char* res, char * buffer, unsigned int length){
	unsigned int size = (length - 6);
	unsigned char xor = buffer[4];

	printf("Antes Malloc %d\n", size);

	write(1, "OLA",3);
	if(res == NULL)
		printf("CAN'T ALLOC!!!! DAMN SON \n");

	printf("Apos Malloc %d\n", size);
	int i = 0;
	for(; i < size; i++){
		res[i] = buffer[i+4];

		if(i != 0)
			xor ^= buffer[i+4];
	}

	printf("Verdade? %02x %02x\n", buffer[length - 2], xor);

	if((unsigned char)buffer[length - 2] == (unsigned char)xor)
		return size;

	return -1;
}

char parseSupervision(int fd, char* data, unsigned int* length){

	unsigned int maxSize = 2 + (4 + PACKAGE_LENGTH) * 2;
	char buf[maxSize];
	char temp[MAX_PACKAGE_DESTUFFED_SIZE];
	unsigned int lengthTemp;
	  								//Se reader FAC
	unsigned int index = 0;
	printf("--------------------------------------------\n");
	while(internal_state != STOP){
		char readed;
		read(fd, &readed, 1);

		switch(internal_state){
			case START:printf("START: %02x \n",readed);
				if(readed == F){
					internal_state = FLAG_RCV;
					buf[0] = readed;
				}
				break;
			case FLAG_RCV:printf("FLAG_RCV: %02x \n",readed);
				switch(readed){
					case F: //RETORNAR quando algo nao corresponde ao pertendido.
						break;
					case A_SENDER:
					case A_RECEIVER:
						internal_state = A_RCV;
						buf[1] = readed;
						break;
					default:
						internal_state = START; //RETORNAR EM ZONAS QUE internal_state = START.
						break;
				}
				break;
			case A_RCV:printf("A_RCV: %02x  \n",readed);
				switch(readed){
					case F:
						internal_state = FLAG_RCV;
						break;
					case C_RR0:
					case (char)C_RR1:
					case C_REJ0:
					case (char)C_REJ1:
					case C_I:
					case C_I2:
					case C_DISC:
					case C_SET:
					case C_UA:
						internal_state = C_RCV;
						buf[2] = readed;
						break;
					default:
						internal_state = START;
						break;
				}
				break;
			case C_RCV:printf("C_RCV: %02x \n",readed);
				switch(readed){
					case F:
						internal_state = FLAG_RCV;
						break;
					default:
						//Se BCC ERRADO Voltar Retornar
						if((buf[1] ^ buf[2]) == readed){
							buf[3] = readed;
							internal_state = BCC_OK;
						}else
							internal_state = START;
						break;
				}
				break;
			case BCC_OK:printf("BCC_OK: %02x \n",readed);
				switch(readed){
					case F:
						buf[4] = readed;
						internal_state = STOP;
						break;
					default:
						internal_state = D_RCV;
						buf[4] = readed;
						index = 5;
						break;
				}
				break;
			case D_RCV:printf("D_RCV: %02x \n",readed);
					switch(readed){
						case F:
							if(length == NULL)
								return -1;

							printf("Index: %d\n", index);
							buf[index] = readed;
							index++;

							printf("Index PASSOU!!!: %d\n", index);

							lengthTemp = byteDesStuffing(temp,buf, index);
							debugChar(temp,lengthTemp);
							printf("Ja nao passa!!!: %d\n", index);

							*length = getDataFromBuffer(data,temp,lengthTemp);

							debugChar(data,*length);

							//Verificar data se as condicoes estao esperadas, ver se o valor de data corresponde com o valor pretendido.

							internal_state=STOP;

							printf("D_RCV: END\n");

						break;
						default:
							if(index >= maxSize -1){
								internal_state=STOP;
								break;
							}

							buf[index] = readed;
							index++;
						break;
					}

				break;
		}
	}

	if(internal_state != FLAG_RCV){
		internal_state = START;
	}
	printf("------Finalized Supervision \n");
	alarm(0);
	return buf[2];
}

void timeOut(){

	sendLastBuffer(fileID,&lastBuffer);
	printf("Alarm\n");
	alarm(3);
}


int initTransmitter(){
	char bufToSend[SupervisionSize];
	getSupervisionBuf(bufToSend,A,C_SET);

	setBuffer(&lastBuffer, bufToSend , SupervisionSize);
	sendLastBuffer(fileID,&lastBuffer);
	alarm(3);

	unsigned char content;
  	do{
		content = parseSupervision(fileID, NULL, NULL);
	}while(content != C_UA);

	return 0;
}

int initReceiver(){


	unsigned char content;
	do{
		content = parseSupervision(fileID, NULL, NULL);
	}while(content != C_SET);



	char bufToSend[SupervisionSize];
	getSupervisionBuf(bufToSend,A,C_UA);

	setBuffer(&lastBuffer, bufToSend , SupervisionSize);
	sendLastBuffer(fileID,&lastBuffer);

	return 0;

}

int llopen(int porta, unsigned char side){

	if(porta < 0 || porta > 3){
		printf("Error: port range is [0;3] given: %d \n", porta);
		return -1;
	}
    int fd;
    struct termios newtio;

	signal(SIGALRM, timeOut);

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
		if(initTransmitter()){
			close(fd);
			exit(-1);
		}
	}else{
		if(initReceiver()){
			close(fd);
			exit(-1);
		}
	}

	return fd;
}

void llinit(char side){
	sideMacro =side;
}

int llclose(int fd){

	unsigned char c;


	if(sideMacro == TRANSMITTER){
		//sending DISC
		printf("sendng DISC \n");
    char disc[SupervisionSize];
		getSupervisionBuf(disc,A_SENDER,C_DISC);
		setBuffer(&lastBuffer,disc,SupervisionSize);
		sendLastBuffer(fileID,&lastBuffer);
		alarm(3);
		//read DISC
		do{
						printf(" DISC \n");
	 		c=parseSupervision(fileID,NULL,NULL);
			printf("receiving DISC \n");
		}while(c != C_DISC);
		//SEND UA
		printf("sendng UA \n");
		char ua[SupervisionSize];
		getSupervisionBuf(ua,A_SENDER,C_UA);
		setBuffer(&lastBuffer,ua,SupervisionSize);
		sendLastBuffer(fileID,&lastBuffer);
		alarm(3);


	}else{
		//read disc
		do{
	 		c=parseSupervision(fileID,NULL,NULL);
		}while(c != C_DISC);
		//sending DISC
    char disc[SupervisionSize];
		getSupervisionBuf(disc,A_RECEIVER,C_DISC);
		setBuffer(&lastBuffer,disc,SupervisionSize);
		sendLastBuffer(fileID,&lastBuffer);
		alarm(3);
		//read UA
		do{
	 		c=parseSupervision(fileID,NULL,NULL);
		}while(c != C_UA);
	}

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
	int res = close(fd);

	return res ? -1 : 1;
}

void sendResponse(int fd, char a, char c){
		char buf[SupervisionSize];
		getSupervisionBuf(buf,a,c);
		sendBytes(fd,  buf, 5);
}

int llread(int fd, char* buff){
	unsigned int length;
	unsigned char c = parseSupervision(fd,buff,&length);

	c >>= 6;

	if(length == -1){//leu mal
		printf("#####LEU MAL\n");
		if(c != ns ){//é repetido
			printf("#####E REPETIDO\n");
			sendResponse(fd,A,C_RR(ns));
			return -1;
		}else{
			printf("#####NOT REPETIDO\n");
			sendResponse(fd,A,C_REJ(ns));
			return -1;
		}
	}else{//leu bem
		printf("#####LEU BEM\n");
		if(c != ns ){//é repetido
			printf("#####E REPETIDO\n");
			sendResponse(fd,A,C_RR(ns));
			return -1;
		}else{
			printf("#####NOT REPETIDO\n");
			ns = ns ? 0:1;
			sendResponse(fd,A,C_RR(ns));
			return length;
		}
	}

}



int llwrite(int fd, char* buffer, int length){

	char *data = buffer;
	unsigned int maxLen = length/PACKAGE_LENGTH;

	printf("maxLen : %d\n", maxLen);
	sleep(3);

	int i;
	for(i = 0 ; i < maxLen + 1; i++){
		printf("Position: %d", (buffer - data));
		unsigned int len = PACKAGE_LENGTH;

		if( i == maxLen){

			len = length % PACKAGE_LENGTH;
			printf("len:%d \n",len);
			sleep(5);
			if( len == 0)
				break;
		}
		unsigned int frameLength = 0;
		char frame[MAX_PACKAGE_SIZE];
		frameLength = getDataBuf(frame,A, ns, data,len);
		printf("Writing Next DataBuf:\n");
		debugChar(frame,frameLength);

		while(1){

			setBuffer(&lastBuffer, frame, frameLength);
			sendLastBuffer(fd, &lastBuffer);
			alarm(3);
			//Wait RR
			char res = parseSupervision(fd, NULL, NULL);//verificar se o C é valido!

			printf("RS %x %x \n" , RS(res), ns);

			if(ns != RS(res)){//proxima trama
				ns = ns ? 0:1;
				printf("Ns correto\n");
				break;
			}else{//resend trama
				printf("Ns resend\n");
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
