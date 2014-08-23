#include "nand_flash.h"
//#include "sw_types.h"

#define ENABLE_NAND		(NFCONT &= ~(1 << 1))
#define	DISABLE_NAND	(NFCONT |= (1 << 1))
#define PAGE_SIZE		512
#define	BLOCK_PER_PAGE	64
#define SPARE_SIZE		16

/*************************************************
Initialize the  hardware of nandflash
*************************************************/
int nand_flash_hwr_init(void)
{
	MP0_1CON &= ~(0xf << 8);
	MP0_1CON |= (0x3 << 8);
	
	NFCONF &= ~((1 << 1) | (1 << 2) | (1 << 3) | (0xf << 4) | (0xf << 8) | (0xf << 12) | (3 << 23));
	NFCONF |= (TACLS << 12) | (TWPRH0 << 8) |(TWPRH1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) |(3 << 23);
	
	NFCONT |= 1 ;
	
	reset();
	return 0;
}

void nand_ready()
{
	while(!(NFSTAT & 1));
}

void reset()
{
	ENABLE_NAND;
	NFCMMD = 0xff;
	nand_ready();
	DISABLE_NAND;
}

void send_addr(unsigned int blockpage)
{
	//页内地址
	NFADDR = 0;  //a0~a7
	NFADDR = 0;  //a8~a11
	//哪页
	NFADDR = (blockpage) & 0xff;  //a12~a19
	NFADDR = (blockpage>> 8) & 0xff;   //a20~a27
	NFADDR = (blockpage >> 16) & 0xff;    //a28
	
}

void send_addr0(unsigned int blockpage)
{
	//页内地址
	NFADDR = 0;  //a0~a7
	NFADDR = 2;  //a8~a11
	//哪页
	NFADDR = (blockpage) & 0xff;  //a12~a19
	NFADDR = (blockpage>> 8) & 0xff;   //a20~a27
	NFADDR = (blockpage >> 16) & 0xff;    //a28
}
/************************************************
Erase a block on nandflash;
block:	the number of block;

*************************************************/
int nand_flash_erase_block(int block)
{
	unsigned int page = block * BLOCK_PER_PAGE;
	ENABLE_NAND;
	NFCMMD = 0x60;

	NFADDR = page & 0xff;
	NFADDR = (page >> 8) & 0xff;
	NFADDR = (page >> 16) & 0x7;
	// NFADDR = 0;
	// NFADDR = 0;
	// NFADDR = block;
	NFCMMD = 0xD0;
	nand_ready();
	NFCMMD = 0x70;
	// if(!(NFDATA & 1))
	// {
		// printk("erase completed!");
	// }
	DISABLE_NAND;
	return 1;
}

/************************************************
Write data to blockpage;
blockpage:
data:	the first 512bytes
spare:	the next 16bytes
*************************************************/
int nand_flash_program_buf(int blockpage, const unsigned char *data, const unsigned char *spare)
{
	int i;
	if(data != NULL)
	{
		ENABLE_NAND;
		NFCMMD = 0x80;
		send_addr(blockpage);
		for(i = 0;i < PAGE_SIZE;i++)
		{
			NFDATA = data[i];
		}
		if(spare != NULL)
		{
			for(i = 0;i < SPARE_SIZE;i++)
			{
				NFDATA = spare[i];
			}
		}
		NFCMMD = 0x10;
		nand_ready();
		DISABLE_NAND;
	}	
	else if(spare != NULL)
	{
		//blockpage += PAGE_SIZE;
		ENABLE_NAND;
		NFCMMD = 0x80;
		send_addr0(blockpage);
		for(i = 0;i < SPARE_SIZE;i++)
		{
			NFDATA = spare[i];
		}
		NFCMMD = 0x10;
		nand_ready();
		DISABLE_NAND;
	}
	else 
		return 1;
	return 0;
}



/************************************************
Read data to memory;
blockpage:
data:	the first 512 bytes
spare:	the next 16 bytes
*************************************************/
int nand_flash_read_buf(int blockpage, unsigned char *data,unsigned char *spare)
{
	int i = 0;
	if(data != NULL)
	{
		ENABLE_NAND;
		NFCMMD = 0x00;
		send_addr(blockpage);
		NFCMMD = 0x30;
		nand_ready();
		for(i = 0;i < PAGE_SIZE;i++)
		{
			data[i] = NFDATA;
		}
		if(spare != NULL)
		{
			for(i = 0;i < SPARE_SIZE;i++)
			{
				spare[i] = NFDATA;
			}
		}
		DISABLE_NAND;
	}
	else if(spare != NULL)
	{
		
		ENABLE_NAND;
		NFCMMD = 0x00;
		send_addr0(blockpage);
		NFCMMD = 0x30;
		nand_ready();
		
		for(i = 0;i < SPARE_SIZE;i++)
		{
			spare[i] = NFDATA;
		}
		DISABLE_NAND;
	}
	else 
		return 1;
	return 0;
}

// int nand_flash_read_spare(int blockpage,unsigned char *spare_data)
// {
	// int i = 0;
	// //blockpage += PAGE_SIZE;
	// ENABLE_NAND;
	// NFCMMD = 0x00;
	// send_addr0(blockpage);
	// NFCMMD = 0x30;
	// nand_ready();
	// for(;(i < SPARE_SIZE);i++)
	// {
		// spare_data[i] = NFDATA;
	// }
	// DISABLE_NAND;
	// return 1;
// }

// int nand_flash_write_spare(int blockpage,unsigned char *spare_data)
// {
	// int i = 0;
	// //blockpage += PAGE_SIZE;
	// ENABLE_NAND;
	// NFCMMD = 0x80;
	// send_addr0(blockpage);
	// for(i = 0;i < SPARE_SIZE;i++)
	// {
		// NFDATA = spare_data[i];
	// }
	// NFCMMD = 0x10;
	// nand_ready();
	// DISABLE_NAND;
	// return 1;
// }
// void read_id(char *buf)
// {
	// int i;
	// ENABLE_NAND;
	// NFCMMD = 0x90;
	// NFADDR = 0x00;
	// nand_ready();
	// for(i = 0;i < 7;i++)
	// {
		// buf[i] = NFDATA;
	// }
	// DISABLE_NAND;
	
// }