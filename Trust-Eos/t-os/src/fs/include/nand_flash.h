#include "devextras.h"

#define	NAND_BASE	0xb0e00000

#define NFCONF		(*(volatile unsigned *)(NAND_BASE + 0x00))
#define	NFCONT		(*(volatile unsigned *)(NAND_BASE + 0x04))
#define	NFCMMD		(*(volatile	unsigned *)(NAND_BASE + 0x08))
#define	NFADDR		(*(volatile unsigned char*)(NAND_BASE + 0x0c))
#define	NFDATA		(*(volatile unsigned char*)(NAND_BASE + 0x10))
#define	NFSTAT		(*(volatile unsigned *)(NAND_BASE + 0x28))

#define MP0_1CON	(*(volatile unsigned *)0xe02002e0)

//#define	MEM_SYS_CFG	(*(volatile unsigned *)0x7e00f120)


#define	TACLS		15
#define	TWPRH0		15
#define	TWPRH1		15

//void nand_init();
void reset();
void send_addr(unsigned int addr);
// void write_data(unsigned int mem_addr,unsigned int nand_addr,unsigned int len);
// void read_data(unsigned int mem_addr,unsigned int nand_addr,unsigned int len);
// int erase(unsigned int addr);
void read_id(char *buf);