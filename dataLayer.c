#include "linkLayer.h"



#define MAX_DATA_LENGTH 65535

#define DATA_PACKET 0x01
#define START_PACKET 0x02
#define END_PACKET 0x03

#define FILE_SIZE 0x00
#define FILE_NAME 0x01



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
void CreateFile(struct controlData data, char* fileData);

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

	DEBUG("Creating buffer: %d\n", buffSize);
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
	unsigned int bufferIndex = 0;//bufer principal com a data toda
	unsigned int bufferLength = 0;//tamanho do buffer todo

	unsigned int currentPacketIndex = 0; //index do pacote que esta a ser lido (controlo ou dados)

	unsigned int parameterLength = -1;

	unsigned int controlSize = 0;
	char controlBufferStart[3+255*2];
	char controlBufferEnd[3+255*2];

	char* controlBuffer;//auxiliar para ler os buffer de controlo
	char data[MAX_DATA_LENGTH];//data propriamente dita do pacote de dados a ser lido

	char dataPacket[MAX_DATA_LENGTH + 4];// pacote de dados
	unsigned int length2 = 0;

	unsigned char readedTypes = 0;//para saber qual os  T do pacote de controlo foram lidos
	char tempData[PACKAGE_LENGTH];//pacotes lidos no llread com packet_length de dados atualmente 5
	while(estado != READING_END){


		unsigned int index = 0;//index do temp data
		DEBUG("Reading nextData:\n");
		int length = readData(fd, tempData);


		while(index < length){
			DEBUG("[%d, %02x]::", index, estado);
			switch(estado){
				case READING_CONTROL_START: DEBUG("READING_CONTROL_START %02x\n", tempData[index]);
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
				case READING_CONTROL_END:DEBUG("READING_CONTROL_END %02x\n", tempData[index]);
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
						case FILE_SIZE:DEBUG("FILE_SIZE %02x \n", tempData[index]);
							controlBuffer[currentPacketIndex] = tempData[index];
							estado = READING_CONTROL_FILESIZE;
							parameterLength = -1;
							currentPacketIndex++;
							index++;

							break;
						case FILE_NAME:DEBUG("FILE_NAME: %02x \n", tempData[index] );
							controlBuffer[currentPacketIndex] = tempData[index];
							estado = READING_CONTROL_NAME;
							parameterLength = -1;
							currentPacketIndex++;
							index++;
							break;
					}
					readedTypes++;

					break;
				case READING_CONTROL_NAME: DEBUG("READING_CONTROL_NAME: %02x \n", tempData[index] );
				case READING_CONTROL_FILESIZE: DEBUG("READING_CONTROL_FILESIZE: %02x \n", tempData[index] );
					if(parameterLength == -1){
						parameterLength = (unsigned char)tempData[index] + 1;
						DEBUG("Parameter Length: %d\n", parameterLength );
					}
					controlBuffer[currentPacketIndex] = tempData[index];

					currentPacketIndex++;
					index++;
					parameterLength--;
					if(parameterLength <= 0){
						if(readedTypes == 2){
							if(isStart){
								DEBUG("Changing state to READING_CONTROL_FINALIZE \n");
								index--;
								estado = READING_CONTROL_FINALIZE;
							}else
								estado = READING_END;
						}else
							estado = READING_CONTROL_T;
					}
					break;
				case READING_CONTROL_FINALIZE: DEBUG("READING_CONTROL_FINALIZE: %02x \n", tempData[index] );


					controlSize = currentPacketIndex;
					
					bufferLength = fileInfo->length;

					decodeControlPacket(controlBuffer, fileInfo);

					DEBUG("Debug: %s, %d\n", fileInfo->name, fileInfo->length);

					buffer = (char*)malloc(sizeof(char) * bufferLength);
					estado = READING_DATA;
					index++;
					readedTypes = 0;
					isStart = 0;

					break;
				case READING_DATA:  DEBUG("READING_DATA: %02x \n", tempData[index] );// LÊ O C
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
				case READING_DATA_SEQUENCE: DEBUG("READING_DATA_SEQUENCE: %02x \n", tempData[index] );//LÊ O N
					dataPacket[currentPacketIndex] = tempData[index];
					estado = READING_DATA_LENGTH0;
					index++;
					currentPacketIndex++;
					break;
				case READING_DATA_LENGTH0: DEBUG("READING_DATA_LENGTH0: %02x \n", tempData[index] );// LÊ O L2
					dataPacket[currentPacketIndex] = tempData[index];
					estado = READING_DATA_LENGTH1;
					parameterLength = ((unsigned char)tempData[index] << 8);
					index++;
					currentPacketIndex++;
					break;
				case READING_DATA_LENGTH1: DEBUG("READING_DATA_LENGTH1: %02x \n", tempData[index] );//LÊ O L1
					dataPacket[currentPacketIndex] = tempData[index];
					estado = READING_DATA_PACKETS;
					parameterLength += (unsigned char)tempData[index];
					index++;
					currentPacketIndex++;
					break;
				case READING_DATA_PACKETS:  DEBUG("READING_DATA_PACKETS: %02x \n", tempData[index] );
					dataPacket[currentPacketIndex] = tempData[index];

					parameterLength--;
					index++;
					currentPacketIndex++;
					if(parameterLength <= 0){
							index--;
							estado = READING_DATA_FINALIZE;
					}

					break;
				case READING_DATA_FINALIZE:  DEBUG("READING_DATA_FINALIZE: %02x \n", tempData[index] );

					//Funcao que retorna o char* dados ou o apontador para os dados + numero de dados para ler :D
										
					length2 = decodeDataPacket(data, dataPacket);


					memcpy(buffer + bufferIndex, data, length2);
					
					bufferIndex+=length2;
					index++;
					
					if(bufferIndex >= bufferLength){
						DEBUG("Alterando Estado para ControlEND\n");
						estado = READING_CONTROL_END;
					}else
						estado = READING_DATA;

					
					parameterLength = 0;

					break;
			}

		}
	}/*
	decodeControlPacket(controlBufferStart, fileInfo);
	decodeControlPacket(controlBufferEnd, fileInfo);*/
	



	if(!memcmp(controlBufferStart + 1, controlBufferEnd + 1, controlSize))
		return buffer;

	return NULL;
}


char * readFile(int fd, struct controlData * controlData, unsigned int * length){

	char * res = readPackets(fd, length, controlData);

	
	return res;
}

void sendFile(int fd, char* path){

	int file = open(path, O_RDWR);

	struct stat fileStat;
	if(lstat(path, &fileStat) < 0){
		DEBUG("Problem using fstat\n");
	}



	unsigned int fileSize = fileStat.st_size ;

	DEBUG("SENDING FILE: %s %d\n", path, fileSize);
	//readFile
	struct controlData controlData;
	strcpy(controlData.name, path);
	controlData.length = fileSize;
	//char * fileData = "OlaBelhoteTudoBemContigo? EU ca estou bem..... poucos bytes pah!"; //Include StartControl and EndControlData.
		
	char* fileData = (char*)malloc(sizeof(char) * fileSize);

	unsigned int dataWritten = 0;
	do{
		unsigned int res = read(file, fileData + dataWritten, fileSize - dataWritten);
		dataWritten+=res;
		DEBUG("dataWritten + %d\n", dataWritten);
	}while(dataWritten < fileSize);




	strcpy(controlData.name, "./pinguim2.gif");
	CreateFile(controlData, fileData);
	//strcpy(controlData.name, path);
	debugChar(fileData, controlData.length);

	

	char * fileData2 = fileData;



	controlData.length = fileSize;
	unsigned int length = controlData.length;
	unsigned int size;

	char * controlStart = createControlPacket(START_PACKET,controlData.name,controlData.length,&size);


	DEBUG("----Sending ControlSTART!\n");
	llwrite(fd, controlStart, size);

	free(controlStart);
	controlStart = NULL;
	unsigned int offset = 0;

	do{

		unsigned int packetLength;
		char * data = createDataPacket(fileData2 + offset, length-offset, &size, &packetLength);

		offset += packetLength;

		DEBUG("----Sending DataBuffer!\n");
		llwrite(fd,data,size);
		free(data);
		data = NULL;
	}while(offset < length);


	DEBUG("----Sending ControlEND!\n");
	char * controlEND = createControlPacket(END_PACKET,controlData.name,controlData.length,&size);
	llwrite(fd, controlEND, size);
	free(controlEND);
	controlEND = NULL;

}


void testDataPacket(){

	unsigned int buffLength, packetLength;
	DEBUG("Executing testDataPacket()\n");
	char * data = "Bela Cena";
	debugChar(data,8);

	char * buffy = createDataPacket(data,8, &buffLength, &packetLength);

	DEBUG("buffLength %d", buffLength);

	debugChar(buffy,buffLength);
	char buffy2[MAX_DATA_LENGTH];

	buffLength = decodeDataPacket(buffy2, buffy);

	debugChar(buffy2,buffLength);

	free(buffy);
	buffy = NULL;


	DEBUG("FINALIZING: testDataPacket()\n");

}

void CreateFile(struct controlData data, char* fileData){

	int fd;
	DEBUG("Creating FILE %s %d \n", data.name, data.length);
	fd = open(data.name, O_RDWR | O_TRUNC | O_CREAT, S_IRWXU);

	unsigned int dataWritten = 0;

	do{
		unsigned int res = write(fd,fileData + dataWritten, data.length - dataWritten);
		dataWritten+=res;
	}while(dataWritten < data.length);

}

void testSendFile(int fd, int side){
	DEBUG("######TESTSEINDFILE()\n");

	if(side == TRANSMITTER){
		sendFile(fd,"./pinguim.gif");
	}else{
		unsigned int size = 0;
		struct controlData controlData = getControlData();
		char* file = readFile(fd, &controlData ,&size );

		CreateFile(controlData, file);

		DEBUG("testSendFile: Received: %s", file);

	}

	DEBUG("######ENDTESTSEINDFILE()\n");

}


int main(int argc,char *argv[]){

	if(argc != 3){
		DEBUG("Usage: port_number <int> side <char>\n");
		return -1;
	}
	//testDataPacket();
	char side = argv[2][0];

	int port_number = atoi(argv[1]);
	unsigned char sideMacro;

	int fileID;

	if(side == 's'){
		fileID = llopen(port_number, TRANSMITTER);
		sideMacro=TRANSMITTER;
	}else if(side == 'r'){
		fileID = llopen(port_number, RECEIVER);
		sideMacro=RECEIVER;
	}else{
		DEBUG("Side must be (sender) 's' or (receiver) 'r' ");
		return 1;
	}

	llinit(sideMacro);

	testSendFile(fileID, sideMacro);

	llclose(fileID);

	DEBUG("\n Done!\n");




	return 0;
}
