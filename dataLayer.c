#include "linkLayer.h"



int main(int argc,char *argv[]){
	if(argc != 3){
		printf("Usage: port_number <int> side <char>\n");
		return -1;
	}
	
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
