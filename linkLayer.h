#ifndef __LINKLAYER_H
#define __LINKLAYER_H

#include <mcheck.h>
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
#include <time.h>

#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
	#define DEBUG(...) printf(__VA_ARGS__)
#else
	#define DEBUG(...)
#endif

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define BIT(n) (0x01 << n)

#define F 0x7E
#define A 0x03
#define A_SENDER 0x03
#define A_RECEIVER 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define C_INFO(n) (n << 6)

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define C_I 0x00
#define C_I2 BIT(6)
#define BCC_OK 4
#define STOP 5
#define D_RCV 6
#define TRANSMITTER 1
#define RECEIVER 2
#define ESCAPE 0x7d
#define ESCAPE_XOR 0x20

#define C_RR0 0x05
#define C_RR1 (BIT(7) | C_RR0)
#define C_RR(n) ((n << 7) | 0x05)

#define C_REJ0 0x01
#define C_REJ1 (BIT(7) | C_REJ0)

#define C_REJ(n) ((n << 7) | 0x01)

#define RS(n) (unsigned char)((unsigned char)n >>7 )

//#define PACKAGE_LENGTH 5

//#define MAX_PACKAGE_SIZE (2 + (4 + PACKAGE_LENGTH) * 2)
//#define MAX_PACKAGE_DESTUFFED_SIZE (6 + PACKAGE_LENGTH)
#define SupervisionSize 5

extern unsigned int _user_baudrate;
extern unsigned int _packageLength;
extern unsigned int _maxResends;
extern unsigned int _alarmInterval;
extern unsigned int _max_package_size;
extern unsigned int _max_package_destuffed_size;

void initLastBuffer();

void llinit(char side);

int llopen(int porta, unsigned char side);

int llclose(int fd);

int llread(int fd, char* buff);

int llwrite(int fd, char* buffer, int length);

void debugChar(char* bytes, int len);

void writeTransmitterInfo();

void writeReceiverInfo();

#endif

