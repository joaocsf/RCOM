#include "linkLayer.h"



#define START_PACKET 2
#define END_PACKET 3
#define FILE_SIZE 0
#define FILE_NAME 1

#define READING_CONTROLL_START 0x00
#define READING_CONTROLL_END 0x10
#define READING_DATA 0x20


#define READING_CONTROLL_T 0x01
#define READING_CONTROLL_FILESIZE 0x02
#define READING_CONTROLL_NAME 0x03

#define READING_DATA_SEQUENCE 0x21
#define READING_DATA_LENGTH 0x22
#define READING_DATA_PACKETS 0x23


struct controlData{
	char *name;
	unsigned int length;
};

void clearControlData(struct controlData * packet){
	if(packet->name != NULL)
		free(packet->name);
	packet->length = 0;
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
				packet->name = (char *)malloc(sizeof(char) * tempSize + 1);
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

int readData(int fd, char ** data){
	int res=  0;
	do{
		res = llread(fd, data);	
	}while(res < 0);
	return res;
}

/*

#define READING_CONTROLL_START 0x00
#define READING_CONTROLL_END 0x10
#define READING_DATA 0x20
#define READING_END 0xFF

#define READING_CONTROLL_T 0x01
#define READING_CONTROLL_FILESIZE 0x02
#define READING_CONTROLL_NAME 0x03
#define READING_CONTROLL_FINALIZE 0x04

#define READING_DATA_SEQUENCE 0x21
#define READING_DATA_LENGTH 0x22
#define READING_DATA_PACKETS 0x23

#define START_PACKET 2
#define END_PACKET 3
#define FILE_SIZE 0
#define FILE_NAME 1

/**/
char* readPackets(int fd, unsigned int* buffLength){
	char loop = 1;
	unsigned char bufferLength = 200;
	unsigned int globalIndex = 0; 	
	unsigned char estado = READING_CONTROLL_START;		
	char isStart = 1;
	char * buffer = (char*)malloc(sizeof(char)*bufferLength);
	
	unsigned int controllIndex = 0;
	char controllBufferStart[3+255*2];	
	char controllBufferEnd[3+255*2];	
	
	char data[3+255*2];	
	

	unsigned char readedTypes = 0;	

	char * res;
	while(estado != READING_END){
		char * tempData;
		
		unsigned int index = 0;	
	
		int length = readData(fd, &tempData);


		while(index < length){
			switch(estado){
				case READING_CONTROLL_START:
					
					if(tempData[index] == START_PACKET){
						buffer[globalIndex] = tempData[index];
						estado = READING_CONTROLL_T;
						isStart = 1; 
						globalIndex++;
						index++;
					}			
						

					break;
				case READING_CONTROLL_END:
					isStart = 0;
							
					break;
				case READING_CONTROLL_T:
					switch(tempData[index]){
						case FILE_SIZE:
														
								
							break;
						case FILE_NAME:
							
							break;
					}

					readedTypes++;
					break;
				case READING_CONTROLL_FILESIZE:
					readedTypes = 0;
					isStart = 0;			
					
					break;
				case READING_CONTROLL_NAME:

					break;
				case READING_CONTROLL_FINALIZE:

					break;
			
			}
			
				
		}		
	
		//FAZER FREE tempDATA;
		free(tempData);		

		
	}	
	
}


char * readFile(int fd, struct * controlData, unsigned int * length){
	
	char loop;
	char * fileData; //Include StartControll and EndControllData.
	
	while(loop){
		
		
		//getStartControl
		//GetDataPackets
		//getEndControl
				
									


	}
	
}

void sendFile(char* path){
	
	//readFile

	//ControlPacket1
	
	//While(dataPackets)	

	//ControlPacket2
	
	//FreeMEMORY
	

}



int main(int argc,char *argv[]){
	if(argc != 3){
		printf("Usage: port_number <int> side <char>\n");
		return -1;
	}
	
	char side = argv[2][0];
	
	int port_number = atoi(argv[1]);
	unsigned char sideMacro;


	unsigned int size;
	struct controlData batata;
	batata.name = NULL;
	decodeControlPacket(createControlPacket(FILE_NAME,"ola",6,&size), &batata);
	
	printf("%s , %d\n" , batata.name, batata.length);
	
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
	
	if(sideMacro == TRANSMITTER){
		printf("Sending Data : \n");
		char bytes[] = {F,ESCAPE,0x57,0x68,0x66,F}; 
		llwrite(fileID, bytes,6);
		
		if(llclose(fileID) < 0){
			perror("Error closing fileID");
	}
	}else{
		printf("Receiving Data : \n");
		char *buff;		
		while(1){
		
		unsigned int size=llread(fileID,&buff);
		debugChar(buff,size);
			if(size==1)
				break;
		}		
		if(llclose(fileID) < 0)
			perror("Error closing fileID");
		
		
	}

	

    	printf("\n Done!\n");



	
	return 0;
}
