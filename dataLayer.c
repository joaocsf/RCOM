#include "linkLayer.h"


#define MAX_DATA_LENGTH 65535

#define DATA_PACKET 0x01
#define START_PACKET 0x02
#define END_PACKET 0x03

#define FILE_SIZE 0x00
#define FILE_NAME 0x01


#define CONTROLB

#define READING_CONTROL_START 0x00
#define READING_CONTROL_END 0x10
#define READING_DATA 0x20
#define READING_END 0xFF

#define READING_CONTROL_T 0x01
#define READING_CONTROL_FILESIZE 0x02
#define READING_CONTROL_NAME 0x03
#define READING_CONTROL_FINALIZE 0x04

#define READING_DATA_SEQUENCE 0x21
#define READING_DATA_LENGTH0 0x22
#define READING_DATA_LENGTH1 0x23
#define READING_DATA_PACKETS 0x24
#define READING_DATA_FINALIZE 0x25


struct controlData{
	char name[255];
	unsigned int length;
};

struct controlData getControlData(){
	struct controlData controlData;
	controlData.length = 0;
	return controlData;

}

unsigned char dataPacketNumber = 0;

void clearControlData(struct controlData * packet){

	packet->length = 0;
}
//Passas-lhe os dados e o tamanho maximo que ele devolve o packet com o tamanho + o tamanho dos dados OP
char * createDataPacket(char * data, unsigned int dataLength, unsigned int * buffLength, unsigned int * packetLength){

	unsigned int packetSize;
	if(dataLength > MAX_DATA_LENGTH)
		packetSize = MAX_DATA_LENGTH;
	else
		packetSize = dataLength;

	*packetLength = packetSize;

	unsigned int buffSize = 4 + packetSize;

	*buffLength = buffSize;

	printf("Creating buffer: %d\n", buffSize);
	char * buff = (char*)malloc(sizeof(char) * buffSize);

	buff[0] = DATA_PACKET;
	buff[1] = dataPacketNumber++;
	buff[2] = (unsigned char)(packetSize >> 8);
	buff[3] = (unsigned char)(packetSize);

	memcpy(buff + 4, data , packetSize);

	return buff;
}
//Mandas o packet e ele devolve os dados + o seu tamanho;
unsigned int decodeDataPacket(char* destination, char* packet){
	if(packet[0] != DATA_PACKET)
		return -1;

	unsigned int packetNumber = (unsigned char)packet[1]; //N
	packetNumber++;
	unsigned int packetSize = 256 * (unsigned char)packet[2] + (unsigned char)packet[3];

	memcpy(destination, packet + 4, packetSize); //4
	return packetSize;
}

char* createControlPacket(char typePacket,char *name,unsigned int fileSize, unsigned int *length){

	int nameSize=strlen(name);
	unsigned int size= 5 + nameSize + sizeof(unsigned int);
	*length=size;
	char *buf= (char *)malloc(sizeof(char) * size);


	buf[0]=typePacket;
	buf[1]=FILE_NAME;
	buf[2]=nameSize;

	int i = 0;
	int offset = 3;
	for(;i < nameSize; i++){
		buf[i + offset]=name[i];
	}

	offset+=nameSize;

	buf[offset] = FILE_SIZE;
	offset++;
	buf[offset] = sizeof(unsigned int);
	offset++;

	memcpy(buf + offset,&fileSize,sizeof(unsigned int));
	return buf;//FAZER FREE !!!!!!!!!!!!!!!!!!!!
}

char decodeControlPacket(char * buff, struct controlData *packet){
	int index = 1;

	clearControlData(packet);

	char readedTypes = 0;
	unsigned int tempSize = 0;
	while(readedTypes < 2){
		switch(buff[index]){
			case FILE_NAME:
				tempSize =buff[index + 1];

				memcpy(	packet->name, buff + index + 2, tempSize);
				packet->name[tempSize]= 0;
				index += 2 + tempSize;
				readedTypes++;
				break;
			case FILE_SIZE:
				tempSize = buff[index + 1];
				memcpy(&(packet->length), buff + index + 2, tempSize);
				index += 2 + tempSize;
				readedTypes++;
				break;
		}

	}

	return buff[0];
}

int readData(int fd, char * data){
	int res=  0;
	do{

		res = llread(fd, data);

	}while(res < 0);
	return res;
}

/*

#define READING_CONTROL_START 0x00
#define READING_CONTROL_END 0x10
#define READING_DATA 0x20
#define READING_END 0xFF

#define READING_CONTROL_T 0x01  //Reading Type
#define READING_CONTROL_FILESIZE 0x02 //Reading FileSize
#define READING_CONTROL_NAME 0x03     //reading name
#define READING_CONTROL_FINALIZE 0x04 //Finalizing

#define READING_DATA_SEQUENCE 0x21
#define READING_DATA_LENGTH 0x22
#define READING_DATA_PACKETS 0x23

#define START_PACKET 2
#define END_PACKET 3
#define FILE_SIZE 0
#define FILE_NAME 1

*/
char* readPackets(int fd, unsigned int* buffLength, struct controlData * fileInfo){
	unsigned char estado = READING_CONTROL_START;
	char isStart = 1;

	char * buffer;
	unsigned int bufferIndex = 0;
	unsigned char bufferLength = 0;

	unsigned int currentPacketIndex = 0;

	unsigned int parameterLength = -1;

	unsigned int controlSize = 0;
	char controlBufferStart[3+255*2];
	char controlBufferEnd[3+255*2];

	char* controlBuffer;
	char data[MAX_DATA_LENGTH];

	char dataPacket[3+255*2];
	unsigned int length2 = 0;

	unsigned char readedTypes = 0;
	char tempData[PACKAGE_LENGTH];
	while(estado != READING_END){


		unsigned int index = 0;
		printf("Reading nextData:\n");
		int length = readData(fd, tempData);


		while(index < length){
			printf("[%d, %02x]::", index, estado);
			switch(estado){
				case READING_CONTROL_START: printf("READING_CONTROL_START %02x\n", tempData[index]);
					currentPacketIndex = 0;
					if(tempData[index] == START_PACKET){
						controlBuffer = controlBufferStart;
						controlBuffer[currentPacketIndex] = tempData[index];
						estado = READING_CONTROL_T;
						isStart = 1;
						currentPacketIndex++;
						index++;
					}else
						index++;
					break;
				case READING_CONTROL_END:printf("READING_CONTROL_END %02x\n", tempData[index]);
					currentPacketIndex = 0;
					isStart = 0;
					if(tempData[index] == END_PACKET){

						controlBuffer = controlBufferEnd;
						controlBuffer[currentPacketIndex] = tempData[index];
						estado = READING_CONTROL_T;
						isStart = 0;
						currentPacketIndex++;
						index++;
					}else
						index++;

					break;
				case READING_CONTROL_T:
					switch(tempData[index]){
						case FILE_SIZE:printf("FILE_SIZE %02x \n", tempData[index]);
							controlBuffer[currentPacketIndex] = tempData[index];
							estado = READING_CONTROL_FILESIZE;
							parameterLength = -1;
							currentPacketIndex++;
							index++;

							break;
						case FILE_NAME:printf("FILE_NAME: %02x \n", tempData[index] );
							controlBuffer[currentPacketIndex] = tempData[index];
							estado = READING_CONTROL_NAME;
							parameterLength = -1;
							currentPacketIndex++;
							index++;
							break;
					}
					readedTypes++;

					break;
				case READING_CONTROL_NAME: printf("READING_CONTROL_NAME: %02x \n", tempData[index] );
				case READING_CONTROL_FILESIZE: printf("READING_CONTROL_FILESIZE: %02x \n", tempData[index] );
					if(parameterLength == -1){
						parameterLength = (unsigned char)tempData[index] + 1;
						printf("Parameter Length: %d\n", parameterLength );
					}
					controlBuffer[currentPacketIndex] = tempData[index];

					currentPacketIndex++;
					index++;
					parameterLength--;
					if(parameterLength <= 0){
						if(readedTypes == 2){
							if(isStart){
								printf("Changing state to READING_CONTROL_FINALIZE \n");
								index--;
								estado = READING_CONTROL_FINALIZE;
							}else
								estado = READING_END;
						}else
							estado = READING_CONTROL_T;
					}
					break;
				case READING_CONTROL_FINALIZE: printf("READING_CONTROL_FINALIZE: %02x \n", tempData[index] );


					controlSize = currentPacketIndex;
					debugChar(controlBuffer, currentPacketIndex);

					bufferLength = fileInfo->length;

					decodeControlPacket(controlBuffer, fileInfo);

					printf("Debug: %s, %d\n", fileInfo->name, fileInfo->length);

					buffer = (char*)malloc(sizeof(char) * bufferLength);
					estado = READING_DATA;
					index++;
					readedTypes = 0;
					isStart = 0;

					break;
				case READING_DATA:  printf("READING_DATA: %02x \n", tempData[index] );
					currentPacketIndex = 0;
					switch(tempData[index]){
						case DATA_PACKET:
							dataPacket[currentPacketIndex] = tempData[index];
							estado = READING_DATA_SEQUENCE;
							index++;
							currentPacketIndex++;
						break;
						default:
							index++;
						break;
					}
					break;
				case READING_DATA_SEQUENCE: printf("READING_DATA_SEQUENCE: %02x \n", tempData[index] );
					dataPacket[currentPacketIndex] = tempData[index];
					estado = READING_DATA_LENGTH0;
					index++;
					currentPacketIndex++;
					break;
				case READING_DATA_LENGTH0: printf("READING_DATA_LENGTH0: %02x \n", tempData[index] );
					dataPacket[currentPacketIndex] = tempData[index];
					estado = READING_DATA_LENGTH1;
					parameterLength = ((unsigned char)tempData[index] << 8);
					index++;
					currentPacketIndex++;
					break;
				case READING_DATA_LENGTH1: printf("READING_DATA_LENGTH1: %02x \n", tempData[index] );
					dataPacket[currentPacketIndex] = tempData[index];
					estado = READING_DATA_PACKETS;
					parameterLength += (unsigned char)tempData[index];
					printf("parameterLength: %d", parameterLength);
					index++;
					currentPacketIndex++;
					break;
				case READING_DATA_PACKETS: printf("READING_DATA_PACKETS: %02x \n", tempData[index] );
					dataPacket[currentPacketIndex] = tempData[index];

					parameterLength--;
					index++;
					currentPacketIndex++;
					if(parameterLength <= 0){
							index--;
							estado = READING_DATA_FINALIZE;
					}

					break;
				case READING_DATA_FINALIZE:  printf("READING_DATA_FINALIZE: %02x \n", tempData[index] );

					//Funcao que retorna o char* dados ou o apontador para os dados + numero de dados para ler :D
					printf("A\n");
					printf("dataPacket[0] %02x \n", dataPacket[0]);
					length2 = decodeDataPacket(data, dataPacket);

					printf("Adress: %x\n", data);

					debugChar(data,length2);

					printf("Adress: %x\n", data);

					memcpy(buffer + bufferIndex, data, length2);
					printf("C\n");
					bufferIndex+=length2;
					index++;
					printf("D\n");
					if(bufferIndex >= bufferLength){
						printf("Alterando Estado para ControlEND\n");
						estado = READING_CONTROL_END;
					}else
						estado = READING_DATA;

					printf("E\n");
					parameterLength = 0;

					break;
			}

		}
	}
	decodeControlPacket(controlBufferStart, fileInfo);

	printf("fileInfo %s %d\n",fileInfo->name, fileInfo->length);

	decodeControlPacket(controlBufferEnd, fileInfo);
	printf("fileInfo2 %s %d\n",fileInfo->name, fileInfo->length);

	debugChar(controlBufferStart, controlSize);
	debugChar(controlBufferEnd, controlSize);


	if(!memcmp(controlBufferStart + 1, controlBufferEnd + 1, controlSize))
		return buffer;

	return NULL;
}


char * readFile(int fd, struct controlData * controlData, unsigned int * length){

	char * res = readPackets(fd, length, controlData);

	//char* readPackets(int fd, unsigned int* buffLength, struct controlData * fileInfo);
	return res;
}

void sendFile(int fd, char* path){

	//readFile
	struct controlData controlData;

	char * fileData = "OlaBelhoteTudoBemContigo? EU ca estou bem..... poucos bytes pah!"; //Include StartControl and EndControlData.
	char * fileData2 = fileData;
	memcpy(controlData.name, "Ficheiro Fixe",14);
	controlData.length = 66;
	unsigned int length = controlData.length;
	unsigned int size;
	char * controlStart = createControlPacket(START_PACKET,controlData.name,controlData.length,&size);


	printf("----Sending ControlSTART!\n");
	llwrite(fd, controlStart, size);

	free(controlStart);
	controlStart = NULL;
	unsigned int offset = 0;

	do{

		unsigned int packetLength;
		char * data = createDataPacket(fileData2 + offset, length, &size, &packetLength);

		offset += packetLength;

		printf("----Sending DataBuffer!\n");
		llwrite(fd,data,size);
		free(data);
		data = NULL;
	}while(offset < length);


	printf("----Sending ControlEND!\n");
	char * controlEND = createControlPacket(END_PACKET,controlData.name,controlData.length,&size);
	llwrite(fd, controlEND, size);
	free(controlEND);
	controlEND = NULL;

}


void testDataPacket(){

	unsigned int buffLength, packetLength;
	printf("Executing testDataPacket()\n");
	char * data = "Bela Cena";
	debugChar(data,8);

	char * buffy = createDataPacket(data,8, &buffLength, &packetLength);

	printf("buffLength %d", buffLength);

	debugChar(buffy,buffLength);
	char buffy2[MAX_DATA_LENGTH];

	buffLength = decodeDataPacket(buffy2, buffy);

	debugChar(buffy2,buffLength);

	free(buffy);
	buffy = NULL;


	printf("FINALIZING: testDataPacket()\n");

}

void testSendFile(int fd, int side){
	printf("######TESTSEINDFILE()\n");

	if(side == TRANSMITTER){
		sendFile(fd,"A");
	}else{
		unsigned int size = 0;
		struct controlData controlData = getControlData();
		char* file = readFile(fd, &controlData ,&size );

		printf("testSendFile: Received: %s", file);

	}

	printf("######ENDTESTSEINDFILE()\n");

}


int main(int argc,char *argv[]){

	mtrace();

	if(argc != 3){
		printf("Usage: port_number <int> side <char>\n");
		return -1;
	}
	testDataPacket();
	char side = argv[2][0];

	int port_number = atoi(argv[1]);
	unsigned char sideMacro;

	unsigned int size;
	struct controlData batata;
	char * data = createControlPacket(START_PACKET,"ola",6,&size);

	decodeControlPacket(data, &batata);
	printf("FILE INFO: %s , %d\n" , batata.name, batata.length);
	debugChar(data, size);

	int fileID;

	if(side == 's'){
		fileID = llopen(port_number, TRANSMITTER);
		sideMacro=TRANSMITTER;
	}else if(side == 'r'){
		fileID = llopen(port_number, RECEIVER);
		sideMacro=RECEIVER;
	}else{
		printf("Side must be (sender) 's' or (receiver) 'r' ");
		return 1;
	}

	llinit(sideMacro);

	testSendFile(fileID, sideMacro);

	printf("\n Done!\n");




	return 0;
}
