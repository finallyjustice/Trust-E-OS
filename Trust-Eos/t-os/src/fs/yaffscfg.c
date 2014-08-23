#include "yaffscfg.h"
#include "yaffsfs.h"

unsigned yaffs_traceMask = 0x11;

void yaffsfs_SetError(int err)
{
	//Do whatever to set error
	//errno = err;
}



void yaffsfs_Lock(void)
{
}

void yaffsfs_Unlock(void)
{
}

uint32_t yaffsfs_CurrentTime(void)
{
	return 0;
}

void yaffsfs_LocalInitialisation(void)
{
	// Define locking semaphore.
}

#include "yaffs_flashif.h"

extern int nand_flash_hwr_init(void);

//static yaffs_Device bootDev;
static yaffs_Device flashDev0;
//static yaffs_Device flashDev1;

static yaffsfs_DeviceConfiguration yaffsfs_config[] = {

	//{ "/boot", &bootDev},
	{ "/c", &flashDev0},
	//{ "/d", &flashDev1},
	{(void *)0,(void *)0}
};


int yaffs_StartUp(void)
{	
	
	flashDev0.nBytesPerChunk = YAFFS_BYTES_PER_CHUNK;
	flashDev0.nChunksPerBlock = YAFFS_CHUNKS_PER_BLOCK;
	flashDev0.nReservedBlocks = 5;
	flashDev0.startBlock = 144; // First block after 2MB
	flashDev0.endBlock = 256; // next block in 32MB
	flashDev0.useNANDECC = 0; // use YAFFS's ECC
	flashDev0.nShortOpCaches = 10; // Use caches
	flashDev0.genericDevice = (void *) 2;	// Used to identify the device in fstat.
	flashDev0.writeChunkToNAND = yflash_WriteChunkToNAND;
	flashDev0.readChunkFromNAND = yflash_ReadChunkFromNAND;
	flashDev0.eraseBlockInNAND = yflash_EraseBlockInNAND;
	flashDev0.initialiseNAND = yflash_InitialiseNAND;
	

	return 0;
}




