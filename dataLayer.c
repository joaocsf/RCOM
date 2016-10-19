#include "linkLayer.h"



#define START_PACKET 2
#define END_PACKET 3
#define FILE_SIZE 0
#define FILE_NAME 1


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
