/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system.
 * yaffs_guts.c  The main guts of YAFFS
 *
 * Copyright (C) 2002 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 //yaffs_guts.c

const char *yaffs_guts_c_version="$Id: yaffs_guts.c,v 1.43 2005/10/11 23:43:27 charles Exp $";

#include "ydirectenv.h"

#include "yaffs_guts.h"

#define YAFFS_PASSIVE_GC_CHUNKS 2

#if 0
// Use Steven Hill's ECC struff instead
// External functions for ECC on data
void nand_calculate_ecc (const u_char *dat, u_char *ecc_code);
int nand_correct_data (u_char *dat, u_char *read_ecc, u_char *calc_ecc);
#define yaffs_ECCCalculate(data,ecc) nand_calculate_ecc(data,ecc)
#define yaffs_ECCCorrect(data,read_ecc,calc_ecc) nand_correct_ecc(data,read_ecc,calc_ecc)
#else
#include "yaffs_ecc.h"
#endif

// countBits is a quick way of counting the number of bits in a byte.
// ie. countBits[n] holds the number of 1 bits in a byte with the value n.
//extern int sprintf(char * buf, const char *fmt, ...);

static const char yaffs_countBitsTable[256] =
{
0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

static int yaffs_CountBits(uint8_t x)
{
	int retVal;
	retVal = yaffs_countBitsTable[x];
	return retVal;
}



// Local prototypes
//static int yaffs_CheckObjectHashSanity(yaffs_Device *dev);
static void yaffs_LoadTagsIntoSpare(yaffs_Spare *sparePtr, yaffs_Tags *tagsPtr);
static void yaffs_GetTagsFromSpare(yaffs_Device *dev, yaffs_Spare *sparePtr,yaffs_Tags *tagsPtr);
static int yaffs_PutChunkIntoFile(yaffs_Object *inObj,int chunkInInode, int chunkInNAND, int inScan);

static yaffs_Object *yaffs_CreateNewObject(yaffs_Device *dev,int number,yaffs_ObjectType type);
static void yaffs_AddObjectToDirectory(yaffs_Object *directory, yaffs_Object *obj);
static int yaffs_UpdateObjectHeader(yaffs_Object *inObj,const char *name, int force);
static void yaffs_DeleteChunk(yaffs_Device *dev,int chunkId,int markNAND);
static void yaffs_RemoveObjectFromDirectory(yaffs_Object *obj);
static int yaffs_CheckStructures(void);
static int yaffs_DeleteWorker(yaffs_Object *inObj, yaffs_Tnode *tn, uint32_t level, int chunkOffset,int *limit);
static int yaffs_DoGenericObjectDeletion(yaffs_Object *inObj);

static yaffs_BlockInfo *yaffs_GetBlockInfo(yaffs_Device *dev,int blockNo);

// Robustification
static void yaffs_RetireBlock(yaffs_Device *dev,int blockInNAND);
static void yaffs_HandleReadDataError(yaffs_Device *dev,int chunkInNAND);
static void yaffs_HandleWriteChunkError(yaffs_Device *dev,int chunkInNAND);
static void yaffs_HandleWriteChunkOk(yaffs_Device *dev,int chunkInNAND,const uint8_t *data, const yaffs_Spare *spare);
static void yaffs_HandleUpdateChunk(yaffs_Device *dev,int chunkInNAND, const yaffs_Spare *spare);

static int  yaffs_CheckChunkErased(struct yaffs_DeviceStruct *dev,int chunkInNAND);

static int yaffs_UnlinkWorker(yaffs_Object *obj);
static void yaffs_AbortHalfCreatedObject(yaffs_Object *obj);

static int yaffs_VerifyCompare(const uint8_t *d0, const uint8_t * d1, const yaffs_Spare *s0, const yaffs_Spare *s1);



off_t yaffs_GetFileSize(yaffs_Object *obj);

static int yaffs_ReadChunkTagsFromNAND(yaffs_Device *dev,int chunkInNAND, yaffs_Tags *tags, int *chunkDeleted);
static int yaffs_TagsMatch(const yaffs_Tags *tags, int objectId, int chunkInObject, int chunkDeleted);


static int yaffs_AllocateChunk(yaffs_Device *dev,int useReserve);

#ifdef YAFFS_PARANOID
static int yaffs_CheckFileSanity(yaffs_Object *inObj);
#else
#define yaffs_CheckFileSanity(inObj)
#endif

static void yaffs_InvalidateWholeChunkCache(yaffs_Object *inObj);
static void yaffs_InvalidateChunkCache(yaffs_Object *object, int chunkId);

// Chunk bitmap manipulations

static __inline uint8_t *yaffs_BlockBits(yaffs_Device *dev, int blk)
{
	if(blk < dev->internalStartBlock || blk > dev->internalEndBlock)
	{
		T(YAFFS_TRACE_ERROR,(TSTR("**>> yaffs: BlockBits block %d is not valid" TENDSTR),blk));
		YBUG();
	}
	return dev->chunkBits + (dev->chunkBitmapStride * (blk - dev->internalStartBlock));
}

static __inline__ void yaffs_ClearChunkBits(yaffs_Device *dev,int blk)
{
	uint8_t *blkBits = yaffs_BlockBits(dev,blk);

	 TEE_MemFill(blkBits,0,dev->chunkBitmapStride);
}

static __inline__ void yaffs_ClearChunkBit(yaffs_Device *dev,int blk,int chunk)
{
	uint8_t *blkBits = yaffs_BlockBits(dev,blk);
	
	blkBits[chunk/8] &=  ~ (1<<(chunk & 7));
}

static __inline__ void yaffs_SetChunkBit(yaffs_Device *dev,int blk,int chunk)
{
	uint8_t *blkBits = yaffs_BlockBits(dev,blk);

	blkBits[chunk/8] |=   (1<<(chunk & 7));
}

static __inline__ int yaffs_CheckChunkBit(yaffs_Device *dev,int blk,int chunk)
{
	uint8_t *blkBits = yaffs_BlockBits(dev,blk);
	return (blkBits[chunk/8] &  (1<<(chunk & 7))) ? 1 :0;
}

static __inline__ int yaffs_StillSomeChunkBits(yaffs_Device *dev,int blk)
{
	uint8_t *blkBits = yaffs_BlockBits(dev,blk);
	int i;
	for(i = 0; i < dev->chunkBitmapStride; i++)
	{
		if(*blkBits) return 1;
		blkBits++;
	}
	return 0;
}

// Function to manipulate block info
static  __inline__ yaffs_BlockInfo* yaffs_GetBlockInfo(yaffs_Device *dev, int blk)
{
	if(blk < dev->internalStartBlock || blk > dev->internalEndBlock)
	{
		T(YAFFS_TRACE_ERROR,(TSTR("**>> yaffs: getBlockInfo block %d is not valid" TENDSTR),blk));
		YBUG();
	}
	return &dev->blockInfo[blk - dev->internalStartBlock];
}


static  __inline__ int yaffs_HashFunction(int n)
{
	return (n % YAFFS_NOBJECT_BUCKETS);
}


yaffs_Object *yaffs_Root(yaffs_Device *dev)
{
	return dev->rootDir;
}

yaffs_Object *yaffs_LostNFound(yaffs_Device *dev)
{
	return dev->lostNFoundDir;
}


static int yaffs_WriteChunkToNAND(struct yaffs_DeviceStruct *dev,int chunkInNAND, const uint8_t *data, yaffs_Spare *spare)
{
	if(chunkInNAND < dev->internalStartBlock * dev->nChunksPerBlock)
	{
		T(YAFFS_TRACE_ERROR,(TSTR("**>> yaffs chunk %d is not valid" TENDSTR),chunkInNAND));
		return YAFFS_FAIL;
	}

	dev->nPageWrites++;
	return dev->writeChunkToNAND(dev,chunkInNAND - dev->chunkOffset,data,spare);
}



int yaffs_ReadChunkFromNAND(struct yaffs_DeviceStruct *dev,
							int chunkInNAND, 
							uint8_t *data, 
							yaffs_Spare *spare, 
							int doErrorCorrection)
{
	int retVal;
	yaffs_Spare localSpare;

	dev->nPageReads++;
	
	

	
	if(!spare && data)
	{
		// If we don't have a real spare, then we use a local one.
		// Need this for the calculation of the ecc
		spare = &localSpare;
	}
	

	if(!dev->useNANDECC)
	{
		retVal  = dev->readChunkFromNAND(dev,chunkInNAND - dev->chunkOffset,data,spare);
		if(data && doErrorCorrection)
		{
			// Do ECC correction
			//Todo handle any errors
         	int eccResult1,eccResult2;
        	uint8_t calcEcc[3];
                
			yaffs_ECCCalculate(data,calcEcc);
			eccResult1 = yaffs_ECCCorrect (data,spare->ecc1, calcEcc);
			yaffs_ECCCalculate(&data[256],calcEcc);
			eccResult2 = yaffs_ECCCorrect(&data[256],spare->ecc2, calcEcc);

			if(eccResult1>0)
			{
				T(YAFFS_TRACE_ERROR, (TSTR("**>>ecc error fix performed on chunk %d:0" TENDSTR),chunkInNAND - dev->chunkOffset));
				dev->eccFixed++;
			}
			else if(eccResult1<0)
			{
				T(YAFFS_TRACE_ERROR,(TSTR("**>>ecc error unfixed on chunk %d:0" TENDSTR),chunkInNAND - dev->chunkOffset));
				dev->eccUnfixed++;
			}

			if(eccResult2>0)
			{
				T(YAFFS_TRACE_ERROR,(TSTR("**>>ecc error fix performed on chunk %d:1" TENDSTR),chunkInNAND - dev->chunkOffset));
				dev->eccFixed++;
			}
			else if(eccResult2<0)
			{
				T(YAFFS_TRACE_ERROR,(TSTR("**>>ecc error unfixed on chunk %d:1" TENDSTR),chunkInNAND - dev->chunkOffset));
				dev->eccUnfixed++;
			}

			if(eccResult1 || eccResult2)
			{
				// Hoosterman, we had a data problem on this page
				yaffs_HandleReadDataError(dev,chunkInNAND);
			}
		}
	}
	else
	{
        // Must allocate enough memory for spare+2*sizeof(int) for ecc results from device.
    	struct yaffs_NANDSpare nspare;
		retVal  = dev->readChunkFromNAND(dev,chunkInNAND - dev->chunkOffset,data,(yaffs_Spare*)&nspare);
		TEE_MemMove (spare, &nspare, sizeof(yaffs_Spare));
		if(data && doErrorCorrection)
		{
			if(nspare.eccres1>0)
			{
				T(YAFFS_TRACE_ERROR,(TSTR("**>>ecc error fix performed on chunk %d:0" TENDSTR),chunkInNAND - dev->chunkOffset));
			}
			else if(nspare.eccres1<0)
			{
				T(YAFFS_TRACE_ERROR,(TSTR("**>>ecc error unfixed on chunk %d:0" TENDSTR),chunkInNAND - dev->chunkOffset));
			}

			if(nspare.eccres2>0)
			{
				T(YAFFS_TRACE_ERROR,(TSTR("**>>ecc error fix performed on chunk %d:1" TENDSTR),chunkInNAND - dev->chunkOffset));
			}
			else if(nspare.eccres2<0)
			{
				T(YAFFS_TRACE_ERROR,(TSTR("**>>ecc error unfixed on chunk %d:1" TENDSTR),chunkInNAND - dev->chunkOffset));
			}

			if(nspare.eccres1 || nspare.eccres2)
			{
				// Hoosterman, we had a data problem on this page
				yaffs_HandleReadDataError(dev,chunkInNAND);
			}

		}
	}
	return retVal;
}


static int yaffs_CheckChunkErased(struct yaffs_DeviceStruct *dev,int chunkInNAND)
{

	static int init = 0;
	static uint8_t cmpbuf[YAFFS_BYTES_PER_CHUNK];
	static uint8_t data[YAFFS_BYTES_PER_CHUNK];
    // Might as well always allocate the larger size for dev->useNANDECC == true;
	static uint8_t spare[sizeof(struct yaffs_NANDSpare)];	

  	dev->readChunkFromNAND(dev,chunkInNAND - dev->chunkOffset,data,(yaffs_Spare *)spare);
	
	if(!init)
	{
		TEE_MemFill(cmpbuf,0xff,YAFFS_BYTES_PER_CHUNK);
		init = 1;
	}
	
	if(TEE_MemCompare(cmpbuf,data,YAFFS_BYTES_PER_CHUNK)) return  YAFFS_FAIL;
	if(TEE_MemCompare(cmpbuf,spare,16)) return YAFFS_FAIL;

	
	return YAFFS_OK;
	
}



int yaffs_EraseBlockInNAND(struct yaffs_DeviceStruct *dev,int blockInNAND)
{
	dev->nBlockErasures++;
	return dev->eraseBlockInNAND(dev,blockInNAND - dev->blockOffset);
}

int yaffs_InitialiseNAND(struct yaffs_DeviceStruct *dev)
{
	return dev->initialiseNAND(dev);
}

static int yaffs_WriteNewChunkToNAND(struct yaffs_DeviceStruct *dev, const uint8_t *data, yaffs_Spare *spare,int useReserve)
{
	int chunk;
	
	int writeOk = 1;
	int attempts = 0;
	
	unsigned char rbData[YAFFS_BYTES_PER_CHUNK];
	yaffs_Spare rbSpare;
	
	do{
		chunk = yaffs_AllocateChunk(dev,useReserve);
	
		if(chunk >= 0)
		{

			// First check this chunk is erased...
#ifndef CONFIG_YAFFS_DISABLE_CHUNK_ERASED_CHECK
			writeOk = yaffs_CheckChunkErased(dev,chunk);
#endif		
			if(!writeOk)
			{
				T(YAFFS_TRACE_ERROR,(TSTR("**>> yaffs chunk %d was not erased" TENDSTR),chunk));
			}
			else
			{
				writeOk =  yaffs_WriteChunkToNAND(dev,chunk,data,spare);
			}
			attempts++;
			if(writeOk)
			{
				// Readback & verify
				// If verify fails, then delete this chunk and try again
				// To verify we compare everything except the block and 
				// page status bytes.
				// NB We check a raw read without ECC correction applied
				yaffs_ReadChunkFromNAND(dev,chunk,rbData,&rbSpare,0);
				
#ifndef CONFIG_YAFFS_DISABLE_WRITE_VERIFY
				if(!yaffs_VerifyCompare(data,rbData,spare,&rbSpare))
				{
					// Didn't verify
					T(YAFFS_TRACE_ERROR,(TSTR("**>> yaffs write verify failed on chunk %d" TENDSTR), chunk));

					writeOk = 0;
				}	
#endif				
				
			}
			if(writeOk)
			{
				// Copy the data into the write buffer.
				// NB We do this at the end to prevent duplicates in the case of a write error.
				//Todo
				yaffs_HandleWriteChunkOk(dev,chunk,data,spare);
			}
			else
			{
				yaffs_HandleWriteChunkError(dev,chunk);
			}
		}
		
	} while(chunk >= 0 && ! writeOk);
	
	if(attempts > 1)
	{
		T(YAFFS_TRACE_ERROR,(TSTR("**>> yaffs write required %d attempts" TENDSTR),attempts));
		dev->nRetriedWrites+= (attempts - 1);	
	}
	
	return chunk;
}

///
// Functions for robustisizing
//
//

static void yaffs_RetireBlock(yaffs_Device *dev,int blockInNAND)
{
	// Ding the blockStatus in the first two pages of the block.
	
	yaffs_Spare spare;

	TEE_MemFill(&spare, 0xff,sizeof(yaffs_Spare));

	spare.blockStatus = 0;
	
	// TODO change this retirement marking for other NAND types
	yaffs_WriteChunkToNAND(dev, blockInNAND * dev->nChunksPerBlock, NULL , &spare);
	yaffs_WriteChunkToNAND(dev, blockInNAND * dev->nChunksPerBlock + 1, NULL , &spare);
	
	yaffs_GetBlockInfo(dev,blockInNAND)->blockState = YAFFS_BLOCK_STATE_DEAD;
	dev->nRetiredBlocks++;
}


#if 0
//defined but not used 
static int yaffs_RewriteBufferedBlock(yaffs_Device *dev)
{
	dev->doingBufferedBlockRewrite = 1;
	//
	//	Remove erased chunks
	//  Rewrite existing chunks to a new block
	//	Set current write block to the new block
	
	dev->doingBufferedBlockRewrite = 0;
	
	return 1;
}
#endif

static void yaffs_HandleReadDataError(yaffs_Device *dev,int chunkInNAND)
{
	int blockInNAND = (unsigned int)chunkInNAND/(unsigned int)dev->nChunksPerBlock;

	// Mark the block for retirement
	yaffs_GetBlockInfo(dev,blockInNAND)->needsRetiring = 1;
	T(YAFFS_TRACE_ERROR | YAFFS_TRACE_BAD_BLOCKS,(TSTR("**>>Block %d marked for retirement" TENDSTR),blockInNAND));


	//TODO	
	// Just do a garbage collection on the affected block then retire the block
	// NB recursion
}

#if 0
//defined but not used
static void yaffs_CheckWrittenBlock(yaffs_Device *dev,int chunkInNAND)
{
}
#endif

static void yaffs_HandleWriteChunkOk(yaffs_Device *dev,int chunkInNAND,const uint8_t *data, const yaffs_Spare *spare)
{
}

static void yaffs_HandleUpdateChunk(yaffs_Device *dev,int chunkInNAND, const yaffs_Spare *spare)
{
}

static void yaffs_HandleWriteChunkError(yaffs_Device *dev,int chunkInNAND)
{
	int blockInNAND = (unsigned int)chunkInNAND/(unsigned int)dev->nChunksPerBlock;

	// Mark the block for retirement
	yaffs_GetBlockInfo(dev,blockInNAND)->needsRetiring = 1;
	// Delete the chunk
	yaffs_DeleteChunk(dev,chunkInNAND,1);
}




static int yaffs_VerifyCompare(const uint8_t *d0, const uint8_t * d1, const yaffs_Spare *s0, const yaffs_Spare *s1)
{


	if( TEE_MemCompare(d0,d1,YAFFS_BYTES_PER_CHUNK) != 0 ||
		s0->tagByte0 != s1->tagByte0 ||
		s0->tagByte1 != s1->tagByte1 ||
		s0->tagByte2 != s1->tagByte2 ||
		s0->tagByte3 != s1->tagByte3 ||
		s0->tagByte4 != s1->tagByte4 ||
		s0->tagByte5 != s1->tagByte5 ||
		s0->tagByte6 != s1->tagByte6 ||
		s0->tagByte7 != s1->tagByte7 ||
		s0->ecc1[0]  != s1->ecc1[0]  ||
		s0->ecc1[1]  != s1->ecc1[1]  ||
		s0->ecc1[2]  != s1->ecc1[2]  ||
		s0->ecc2[0]  != s1->ecc2[0]  ||
		s0->ecc2[1]  != s1->ecc2[1]  ||
		s0->ecc2[2]  != s1->ecc2[2] )
		{
			return 0;
		}
	
	return 1;
}


///////////////////////// Object management //////////////////
// List of spare objects
// The list is hooked together using the first pointer
// in the object

// static yaffs_Object *yaffs_freeObjects = NULL;

// static int yaffs_nFreeObjects;

// static yaffs_ObjectList *yaffs_allocatedObjectList = NULL;

// static yaffs_ObjectBucket yaffs_objectBucket[YAFFS_NOBJECT_BUCKETS];


static uint16_t yaffs_CalcNameSum(const char *name)
{
	uint16_t sum = 0;
	uint16_t i = 1;
	
	uint8_t *bname = (uint8_t *)name;
	if(bname)
	{
		while ((*bname) && (i <=YAFFS_MAX_NAME_LENGTH))
		{

#ifdef CONFIG_YAFFS_CASE_INSENSITIVE
			sum += toupper(*bname) * i;
#else
			sum += (*bname) * i;
#endif
			i++;
			bname++;
		}
	}
	return sum;
}

void yaffs_SetObjectName(yaffs_Object *obj, const char *name)
{
#ifdef CONFIG_YAFFS_SHORT_NAMES_IN_RAM
					if(name && TEE_StrLen(name) <= YAFFS_SHORT_NAME_LENGTH)
					{
						sw_strcpy(obj->shortName,name);
					}
					else
					{
						obj->shortName[0]='\0';
					}
#endif
					obj->sum = yaffs_CalcNameSum(name);
}

void yaffs_CalcECC(const uint8_t *data, yaffs_Spare *spare)
{
	yaffs_ECCCalculate(data , spare->ecc1);
	yaffs_ECCCalculate(&data[256] , spare->ecc2);
}

void yaffs_CalcTagsECC(yaffs_Tags *tags)
{
	// Calculate an ecc
	
	unsigned char *b = ((yaffs_TagsUnion *)tags)->asBytes;
	unsigned  i,j;
	unsigned  ecc = 0;
	unsigned bit = 0;

	tags->ecc = 0;
	
	for(i = 0; i < 8; i++)
	{
		for(j = 1; j &0xff; j<<=1)
		{
			bit++;
			if(b[i] & j)
			{
				ecc ^= bit;
			}
		}
	}
	
	tags->ecc = ecc;
	
	
}

int  yaffs_CheckECCOnTags(yaffs_Tags *tags)
{
	unsigned ecc = tags->ecc;
	
	yaffs_CalcTagsECC(tags);
	
	ecc ^= tags->ecc;
	
	if(ecc && ecc <= 64)
	{
		// TODO: Handle the failure better. Retire?
		unsigned char *b = ((yaffs_TagsUnion *)tags)->asBytes;

		ecc--;
				
		b[ecc / 8] ^= (1 << (ecc & 7));
		
		// Now recvalc the ecc
		yaffs_CalcTagsECC(tags);
		
		return 1; // recovered error
	}
	else if(ecc)
	{
		// Wierd ecc failure value
		// TODO Need to do somethiong here
		return -1; //unrecovered error
	}
	
	return 0;
}


///////////////////////// TNODES ///////////////////////

// List of spare tnodes
// The list is hooked together using the first pointer
// in the tnode.

//static yaffs_Tnode *yaffs_freeTnodes = NULL;

// static int yaffs_nFreeTnodes;

//static yaffs_TnodeList *yaffs_allocatedTnodeList = NULL;



// yaffs_CreateTnodes creates a bunch more tnodes and
// adds them to the tnode free list.
// Don't use this function directly

static int yaffs_CreateTnodes(yaffs_Device *dev,int nTnodes)
{
    int i;
    yaffs_Tnode *newTnodes;
    yaffs_TnodeList *tnl;
    
    if(nTnodes < 1) return YAFFS_OK;
   
	// make these things
	
    newTnodes = YMALLOC(nTnodes * sizeof(yaffs_Tnode));
   
    if (!newTnodes)
    {
		T(YAFFS_TRACE_ERROR,(TSTR("yaffs: Could not allocate Tnodes"TENDSTR)));
		return YAFFS_FAIL;
    }
    
    // Hook them into the free list
    for(i = 0; i < nTnodes - 1; i++)
    {
    	newTnodes[i].internal[0] = &newTnodes[i+1];
#ifdef CONFIG_YAFFS_TNODE_LIST_DEBUG
    	newTnodes[i].internal[YAFFS_NTNODES_INTERNAL] = 1;
#endif
    }
    	
	newTnodes[nTnodes - 1].internal[0] = dev->freeTnodes;
#ifdef CONFIG_YAFFS_TNODE_LIST_DEBUG
   	newTnodes[nTnodes - 1].internal[YAFFS_NTNODES_INTERNAL] = 1;
#endif
	dev->freeTnodes = newTnodes;
	dev->nFreeTnodes+= nTnodes;
	dev->nTnodesCreated += nTnodes;

	// Now add this bunch of tnodes to a list for freeing up.
	// NB If we can't add this to the management list it isn't fatal
	// but it just means we can't free this bunch of tnodes later.
	tnl = YMALLOC(sizeof(yaffs_TnodeList));
	if(!tnl)
	{
		T(YAFFS_TRACE_ERROR,(TSTR("yaffs: Could not add tnodes to management list" TENDSTR)));
		
	}
	else
	{
		tnl->tnodes = newTnodes;
		tnl->next = dev->allocatedTnodeList;
		dev->allocatedTnodeList = tnl;
	}


	T(YAFFS_TRACE_ALLOCATE,(TSTR("yaffs: Tnodes added" TENDSTR)));


	return YAFFS_OK;
}


// GetTnode gets us a clean tnode. Tries to make allocate more if we run out
static yaffs_Tnode *yaffs_GetTnode(yaffs_Device *dev)
{
	yaffs_Tnode *tn = NULL;
	
	// If there are none left make more
	if(!dev->freeTnodes)
	{
		yaffs_CreateTnodes(dev,YAFFS_ALLOCATION_NTNODES);
	}
	
	if(dev->freeTnodes)
	{
		tn = dev->freeTnodes;
#ifdef CONFIG_YAFFS_TNODE_LIST_DEBUG
    	if(tn->internal[YAFFS_NTNODES_INTERNAL] != 1)
		{
			// Hoosterman, this thing looks like it isn't in the list
				T(YAFFS_TRACE_ALWAYS,(TSTR("yaffs: Tnode list bug 1" TENDSTR)));
		}
#endif
		dev->freeTnodes = dev->freeTnodes->internal[0];
		dev->nFreeTnodes--;
		// zero out
		TEE_MemFill(tn,0,sizeof(yaffs_Tnode));
	}
	

	return tn;
}


// FreeTnode frees up a tnode and puts it back on the free list
static void yaffs_FreeTnode(yaffs_Device*dev, yaffs_Tnode *tn)
{
	if(tn)
	{
#ifdef CONFIG_YAFFS_TNODE_LIST_DEBUG
    	if(tn->internal[YAFFS_NTNODES_INTERNAL] != 0)
		{
			// Hoosterman, this thing looks like it is already in the list
				T(YAFFS_TRACE_ALWAYS,(TSTR("yaffs: Tnode list bug 2" TENDSTR)));
		}
		tn->internal[YAFFS_NTNODES_INTERNAL] = 1;
#endif
		tn->internal[0] = dev->freeTnodes;
		dev->freeTnodes = tn;
		dev->nFreeTnodes++;
	}
}


static void yaffs_DeinitialiseTnodes(yaffs_Device*dev)
{
	// Free the list of allocated tnodes
	yaffs_TnodeList *tmp;
		
	while(dev->allocatedTnodeList)
	{
		tmp =  dev->allocatedTnodeList->next;

		YFREE(dev->allocatedTnodeList->tnodes);
		YFREE(dev->allocatedTnodeList);
		dev->allocatedTnodeList	= tmp;
		
	}
	
	dev->freeTnodes = NULL;
	dev->nFreeTnodes = 0;
}

static void yaffs_InitialiseTnodes(yaffs_Device*dev)
{
	dev->allocatedTnodeList = NULL;
	dev->freeTnodes = NULL;
	dev->nFreeTnodes = 0;
	dev->nTnodesCreated = 0;

}

#if 0
void yaffs_TnodeTest(yaffs_Device *dev)
{

	int i;
	int j;
	yaffs_Tnode *tn[1000];
	
	YINFO("Testing TNodes");
	
	for(j = 0; j < 50; j++)
	{
		for(i = 0; i < 1000; i++)
		{
			tn[i] = yaffs_GetTnode(dev);
			if(!tn[i])
			{
				YALERT("Getting tnode failed");
			}
		}
		for(i = 0; i < 1000; i+=3)
		{
			yaffs_FreeTnode(dev,tn[i]);
			tn[i] = NULL;
		}
		
	}
}
#endif


////////////////// END OF TNODE MANIPULATION ///////////////////////////

/////////////// Functions to manipulate the look-up tree (made up of tnodes)
// The look up tree is represented by the top tnode and the number of topLevel
// in the tree. 0 means only the level 0 tnode is in the tree.


// FindLevel0Tnode finds the level 0 tnode, if one exists.
// Used when reading.....
static yaffs_Tnode *yaffs_FindLevel0Tnode(yaffs_Device *dev,yaffs_FileStructure *fStruct, uint32_t chunkId)
{
	
	yaffs_Tnode *tn = fStruct->top;
	uint32_t i;
	int requiredTallness;	
	int level = fStruct->topLevel;
	
	// Check sane level and chunk Id
	if(level < 0 || level > YAFFS_TNODES_MAX_LEVEL)
	{
//		char str[50];
//		sprintf(str,"Bad level %d",level);
//		YALERT(str);
		return NULL;
	}
	
	if(chunkId > YAFFS_MAX_CHUNK_ID)
	{
//		char str[50];
//		sprintf(str,"Bad chunkId %d",chunkId);
//		YALERT(str);
		return NULL;
	}

	// First check we're tall enough (ie enough topLevel)
	
	i = chunkId >> (/*dev->chunkGroupBits  + */YAFFS_TNODES_LEVEL0_BITS);
	requiredTallness = 0;
	while(i)
	{
		i >>= YAFFS_TNODES_INTERNAL_BITS;
		requiredTallness++;
	}
	
	
	if(requiredTallness > fStruct->topLevel)
	{
		// Not tall enough, so we can't find it, return NULL.
		return NULL;
	}
		
	
	// Traverse down to level 0
	while (level > 0 && tn)
	{
	    tn = tn->internal[(chunkId >>(/* dev->chunkGroupBits + */ YAFFS_TNODES_LEVEL0_BITS + (level-1) * YAFFS_TNODES_INTERNAL_BITS)) & 
	                       YAFFS_TNODES_INTERNAL_MASK]; 
		level--;
	
	}
	
	return tn;		
}

// AddOrFindLevel0Tnode finds the level 0 tnode if it exists, otherwise first expands the tree.
// This happens in two steps:
//  1. If the tree isn't tall enough, then make it taller.
//  2. Scan down the tree towards the level 0 tnode adding tnodes if required.
//
// Used when modifying the tree.
//
static yaffs_Tnode *yaffs_AddOrFindLevel0Tnode(yaffs_Device *dev, yaffs_FileStructure *fStruct, uint32_t chunkId)
{
	
	yaffs_Tnode *tn; 
	
	int requiredTallness;
	int i;
	int l;
	
	uint32_t x;
		
	
	//T((TSTR("AddOrFind topLevel=%d, chunk=%d"),fStruct->topLevel,chunkId));
	
	// Check sane level and page Id
	if(fStruct->topLevel < 0 || fStruct->topLevel > YAFFS_TNODES_MAX_LEVEL)
	{
//		char str[50];
//		sprintf(str,"Bad level %d",fStruct->topLevel);
//		YALERT(str);
		return NULL;
	}
	
	if(chunkId > YAFFS_MAX_CHUNK_ID)
	{
//		char str[50];
//		sprintf(str,"Bad chunkId %d",chunkId);
//		YALERT(str);
		return NULL;
	}
	
	// First check we're tall enough (ie enough topLevel)
	
	x = chunkId >> (/*dev->chunkGroupBits + */YAFFS_TNODES_LEVEL0_BITS);
	requiredTallness = 0;
	while(x)
	{
		x >>= YAFFS_TNODES_INTERNAL_BITS;
		requiredTallness++;
	}
	
	//T((TSTR(" required=%d"),requiredTallness));
	
	
	if(requiredTallness > fStruct->topLevel)
	{
		// Not tall enough,gotta make the tree taller
		for(i = fStruct->topLevel; i < requiredTallness; i++)
		{
			//T((TSTR(" add new top")));
			
			tn = yaffs_GetTnode(dev);
			
			if(tn)
			{
				tn->internal[0] = fStruct->top;
				fStruct->top = tn;
			}
			else
			{
					T(YAFFS_TRACE_ERROR,(TSTR("yaffs: no more tnodes" TENDSTR)));
			}
		}
		
		fStruct->topLevel = requiredTallness;
	}
	
	
	// Traverse down to level 0, adding anything we need
	
	l = fStruct->topLevel;
	tn = fStruct->top;
	while (l > 0 && tn)
	{
		x = (chunkId >> (/*dev->chunkGroupBits + */YAFFS_TNODES_LEVEL0_BITS + (l-1) * YAFFS_TNODES_INTERNAL_BITS)) & 
	                       YAFFS_TNODES_INTERNAL_MASK;
			       
		//T((TSTR(" [%d:%d]"),l,i));
		
	    if(!tn->internal[x])
	    {
	    	//T((TSTR(" added")));
		
	    	tn->internal[x] = yaffs_GetTnode(dev);
	    }
	    
	    tn = 	tn->internal[x];
		l--;
	
	}
	
	//TSTR(TENDSTR)));
	
	return tn;		
}

// DeleteWorker scans backwards through the tnode tree and deletes all the
// chunks and tnodes in the file
// Returns 1 if the tree was deleted. Returns 0 if it stopped early due to hitting the limit and the delete is incomplete.

static int yaffs_DeleteWorker(yaffs_Object *inObj, yaffs_Tnode *tn, uint32_t level, int chunkOffset,int *limit)
{
	int i;
	int chunkInInode;
	int theChunk;
	yaffs_Tags tags;
	int found;
	int chunkDeleted;
	int allDone = 1;
	
	
	if(tn)
	{
		if(level > 0)
		{
		
			for(i = YAFFS_NTNODES_INTERNAL -1; allDone && i >= 0; i--)
			{
			    if(tn->internal[i])
		    	{
					if(limit && (*limit) < 0)
					{
						allDone = 0;
					}
					else
					{
						allDone = yaffs_DeleteWorker(inObj,tn->internal[i],level - 1,
										(chunkOffset << YAFFS_TNODES_INTERNAL_BITS ) + i ,limit);
					}
					if(allDone)
					{
						yaffs_FreeTnode(inObj->myDev,tn->internal[i]);
			    		tn->internal[i] = NULL;
					}
			    }
		    
			}
			return (allDone) ? 1 : 0;
		}
		else if(level == 0)
		{
			int hitLimit = 0;
			
			for(i = YAFFS_NTNODES_LEVEL0 -1; i >= 0 && !hitLimit; i--)
			{
			    if(tn->level0[i])
		    	{
					int j;
					
					chunkInInode = (chunkOffset << YAFFS_TNODES_LEVEL0_BITS ) + i;
					
					theChunk =  tn->level0[i] << inObj->myDev->chunkGroupBits;

					// Now we need to search for it
					for(j = 0,found = 0; theChunk && j < inObj->myDev->chunkGroupSize && !found; j++)
					{
						yaffs_ReadChunkTagsFromNAND(inObj->myDev,theChunk,&tags,&chunkDeleted);
						if(yaffs_TagsMatch(&tags,inObj->objectId,chunkInInode,chunkDeleted))
						{
							// found it;
							found = 1;
					
						}
						else
						{
							theChunk++;
						}
					}
					
					if(found)
					{
						yaffs_DeleteChunk(inObj->myDev,theChunk,1);
						inObj->nDataChunks--;
						if(limit)
						{ 
							*limit = *limit-1;
							if(*limit <= 0) 
							{ 
								hitLimit = 1;
							}
						}
					
					}
					
			    	tn->level0[i] = 0;
			    }
		    
			}
			return (i < 0) ? 1 : 0;

			
		}
		
	}
	
	return 1;
	
}

// SoftDeleteWorker scans backwards through the tnode tree and soft deletes all the chunks in the file.
// All soft deleting does is increment the block's softdelete count and pulls the chunk out
// of the tnode.
// THus, essentially this is the same as DeleteWorker except that the chunks are soft deleted.
//
static int yaffs_SoftDeleteWorker(yaffs_Object *inObj, yaffs_Tnode *tn, uint32_t level, int chunkOffset)
{
	int i;
//	int chunkInInode;  	// unused
	int theChunk;
	yaffs_BlockInfo *theBlock;
//	yaffs_Tags tags;	// unused
//	int found;
//	int chunkDeleted;
	int allDone = 1;
	
	
	if(tn)
	{
		if(level > 0)
		{
		
			for(i = YAFFS_NTNODES_INTERNAL -1; allDone && i >= 0; i--)
			{
			    if(tn->internal[i])
		    	{
						allDone = yaffs_SoftDeleteWorker(inObj,tn->internal[i],level - 1,
										(chunkOffset << YAFFS_TNODES_INTERNAL_BITS ) + i);
					if(allDone)
					{
						yaffs_FreeTnode(inObj->myDev,tn->internal[i]);
			    		tn->internal[i] = NULL;
					}
					else
					{
						//Hoosterman... how could this happen.
					}			    
				}		    
			}
			return (allDone) ? 1 : 0;
		}
		else if(level == 0)
		{
			
			for(i = YAFFS_NTNODES_LEVEL0 -1; i >=0; i--)
			{
			    if(tn->level0[i])
		    	{
					// Note this does not find the real chunk, only the chunk group.
					// We make an assumption that a chunk group is niot larger than a block.
					theChunk =  (tn->level0[i] << inObj->myDev->chunkGroupBits);
					T(YAFFS_TRACE_SCAN,(TSTR("soft delete tch %d cgb %d chunk %d" TENDSTR),
						tn->level0[i],inObj->myDev->chunkGroupBits,theChunk));
						
					theBlock =	yaffs_GetBlockInfo(inObj->myDev,  (unsigned int)theChunk/(unsigned int)inObj->myDev->nChunksPerBlock);
					if(theBlock)
					{
						theBlock->softDeletions++;
					}
			    	tn->level0[i] = 0;
			    }
		    
			}
			return 1;
			
		}
		
	}
	
	return 1;
		
}



static void yaffs_SoftDeleteFile(yaffs_Object *obj)
{
	if(obj->deleted &&
	   obj->variantType == YAFFS_OBJECT_TYPE_FILE &&
	   !obj->softDeleted)
	{
		if(obj->nDataChunks <= 0)
		{
				// Empty file, just delete it immediately
				yaffs_FreeTnode(obj->myDev,obj->variant.fileVariant.top);
				obj->variant.fileVariant.top = NULL;
				T(YAFFS_TRACE_TRACING,(TSTR("yaffs: Deleting empty file %d" TENDSTR),obj->objectId));
				yaffs_DoGenericObjectDeletion(obj);	
		}
		else
		{
			yaffs_SoftDeleteWorker(obj, obj->variant.fileVariant.top, obj->variant.fileVariant.topLevel, 0);
			obj->softDeleted = 1;
		}
	}
}





// Pruning removes any part of the file structure tree that is beyond the
// bounds of the file (ie that does not point to chunks).
//
// A file should only get pruned when its size is reduced.
//
// Before pruning, the chunks must be pulled from the tree and the
// level 0 tnode entries must be zeroed out.
// Could also use this for file deletion, but that's probably better handled
// by a special case.

// yaffs_PruneWorker should only be called by yaffs_PruneFileStructure()

static yaffs_Tnode *yaffs_PruneWorker(yaffs_Device *dev, yaffs_Tnode *tn, uint32_t level, int del0)
{
	int i;
	int hasData;
	
	if(tn)
	{
		hasData = 0;
		
		for(i = 0; i < YAFFS_NTNODES_INTERNAL; i++)
		{
		    if(tn->internal[i] && level > 0)
		    {
		    	tn->internal[i] = yaffs_PruneWorker(dev,tn->internal[i],level - 1, ( i == 0) ? del0 : 1);
		    }
		    
		    if(tn->internal[i])
		    {
		    	hasData++;
			}
		}
		
		if(hasData == 0 && del0)
		{
			// Free and return NULL
			
			yaffs_FreeTnode(dev,tn);
			tn = NULL;
		}
		
	}

	return tn;
	
}

static int yaffs_PruneFileStructure(yaffs_Device *dev, yaffs_FileStructure *fStruct)
{
	int i;
	int hasData;
	int done = 0;
	yaffs_Tnode *tn;
	
	if(fStruct->topLevel > 0)
	{
		fStruct->top = yaffs_PruneWorker(dev,fStruct->top, fStruct->topLevel,0);
		
		// Now we have a tree with all the non-zero branches NULL but the height
		// is the same as it was.
		// Let's see if we can trim internal tnodes to shorten the tree.
		// We can do this if only the 0th element in the tnode is in use 
		// (ie all the non-zero are NULL)
		
		while(fStruct->topLevel && !done)
		{
			tn = fStruct->top;
			
			hasData = 0;
			for(i = 1; i <YAFFS_NTNODES_INTERNAL; i++)
			{
				if(tn->internal[i])
		    	{
		    		hasData++;
				}
			}
			
			if(!hasData)
			{
				fStruct->top = tn->internal[0];
				fStruct->topLevel--;
				yaffs_FreeTnode(dev,tn);
			}
			else
			{
				done = 1;
			}
		}
	}
	
	return YAFFS_OK;
}





/////////////////////// End of File Structure functions. /////////////////

// yaffs_CreateFreeObjects creates a bunch more objects and
// adds them to the object free list.
static int yaffs_CreateFreeObjects(yaffs_Device *dev, int nObjects)
{
    int i;
    yaffs_Object *newObjects;
    yaffs_ObjectList *list;
    
    if(nObjects < 1) return YAFFS_OK;
   
	// make these things
	
    newObjects = YMALLOC(nObjects * sizeof(yaffs_Object));
   
    if (!newObjects)
    {
		T(YAFFS_TRACE_ALLOCATE,(TSTR("yaffs: Could not allocate more objects" TENDSTR)));
		return YAFFS_FAIL;
    }
    
    // Hook them into the free list
    for(i = 0; i < nObjects - 1; i++)
    {
    	newObjects[i].siblings.next = (struct fs_list_head *)(&newObjects[i+1]);
    }
    	
	newObjects[nObjects - 1].siblings.next = (void *)dev->freeObjects;
	dev->freeObjects = newObjects;
	dev->nFreeObjects+= nObjects;
	dev->nObjectsCreated+= nObjects;
	
	// Now add this bunch of Objects to a list for freeing up.
	
	list = YMALLOC(sizeof(yaffs_ObjectList));
	if(!list)
	{
		T(YAFFS_TRACE_ALLOCATE,(TSTR("Could not add objects to management list" TENDSTR)));
	}
	else
	{
		list->objects = newObjects;
		list->next = dev->allocatedObjectList;
		dev->allocatedObjectList = list;
	}
	
	
	
	return YAFFS_OK;
}


// AllocateEmptyObject gets us a clean Object. Tries to make allocate more if we run out
static yaffs_Object *yaffs_AllocateEmptyObject(yaffs_Device *dev)
{
	yaffs_Object *tn = NULL;
	
	// If there are none left make more
	if(!dev->freeObjects)
	{
		yaffs_CreateFreeObjects(dev,YAFFS_ALLOCATION_NOBJECTS);
	}
	
	if(dev->freeObjects)
	{
		tn = dev->freeObjects;
		dev->freeObjects = (yaffs_Object *)(dev->freeObjects->siblings.next);
		dev->nFreeObjects--;
		
		// Now sweeten it up...
	
		TEE_MemFill(tn,0,sizeof(yaffs_Object));
		tn->myDev = dev;
		tn->chunkId = -1;
		tn->variantType = YAFFS_OBJECT_TYPE_UNKNOWN;
		FS_INIT_LIST_HEAD(&(tn->hardLinks));
		FS_INIT_LIST_HEAD(&(tn->hashLink));
		FS_INIT_LIST_HEAD(&tn->siblings);
		
		// Add it to the lost and found directory.
		// NB Can't put root or lostNFound in lostNFound so
		// check if lostNFound exists first
		if(dev->lostNFoundDir)
		{
			yaffs_AddObjectToDirectory(dev->lostNFoundDir,tn);	
		}
	}
	

	return tn;
}

static yaffs_Object *yaffs_CreateFakeDirectory(yaffs_Device *dev,int number,uint32_t mode)
{

	yaffs_Object *obj = yaffs_CreateNewObject(dev,number,YAFFS_OBJECT_TYPE_DIRECTORY);		
	if(obj)
	{
		obj->fake = 1;			// it is fake so it has no NAND presence...
		obj->renameAllowed= 0;	// ... and we're not allowed to rename it...
		obj->unlinkAllowed= 0;	// ... or unlink it
		obj->deleted = 0;
		obj->unlinked = 0;
		obj->yst_mode = mode;
		obj->myDev = dev;
		obj->chunkId = 0; // Not a valid chunk.
	}
	
	return obj;
	
}


static void yaffs_UnhashObject(yaffs_Object *tn)
{
	int bucket;
	yaffs_Device *dev = tn->myDev;
	
	
	// If it is still linked into the bucket list, free from the list
	if(!fs_list_empty(&tn->hashLink))
	{
		fs_list_del_init(&tn->hashLink);
		bucket =  yaffs_HashFunction(tn->objectId);
		dev->objectBucket[bucket].count--;
	}
	
}


// FreeObject frees up a Object and puts it back on the free list
static void yaffs_FreeObject(yaffs_Object *tn)
{

	yaffs_Device *dev = tn->myDev;
	
#ifdef  __KERNEL__
	if(tn->myInode)
	{
		// We're still hooked up to a cached inode.
		// Don't delete now, but mark for later deletion
		tn->deferedFree = 1;
		return;
	}
#endif
	
	yaffs_UnhashObject(tn);
	
	// Link into the free list.
	tn->siblings.next = (struct fs_list_head *)(dev->freeObjects);
	dev->freeObjects = tn;
	dev->nFreeObjects++;
}


#ifdef __KERNEL__

void yaffs_HandleDeferedFree(yaffs_Object *obj)
{
	if(obj->deferedFree)
	{
	   yaffs_FreeObject(obj);
	}
}

#endif



static void yaffs_DeinitialiseObjects(yaffs_Device *dev)
{
	// Free the list of allocated Objects
	
	yaffs_ObjectList *tmp;
	
	while( dev->allocatedObjectList)
	{
		tmp =  dev->allocatedObjectList->next;
		YFREE(dev->allocatedObjectList->objects);
		YFREE(dev->allocatedObjectList);
		
		dev->allocatedObjectList =  tmp;
	}
	
	dev->freeObjects = NULL;
	dev->nFreeObjects = 0;
}

static void yaffs_InitialiseObjects(yaffs_Device *dev)
{
	int i;
	
	dev->allocatedObjectList = NULL;
	dev->freeObjects = NULL;
	dev->nFreeObjects = 0;
	
	for(i = 0; i < YAFFS_NOBJECT_BUCKETS; i++)
	{
		FS_INIT_LIST_HEAD(&dev->objectBucket[i].list);
		dev->objectBucket[i].count = 0;	
	}

}






int yaffs_FindNiceObjectBucket(yaffs_Device *dev)
{
	static int x = 0;
	int i;
	int l = 999;
	int lowest = 999999;

		
	// First let's see if we can find one that's empty.
	
	for(i = 0; i < 10 && lowest > 0; i++)
	 {
		x++;
		x %=  YAFFS_NOBJECT_BUCKETS;
		if(dev->objectBucket[x].count < lowest)
		{
			lowest = dev->objectBucket[x].count;
			l = x;
		}
		
	}
	
	// If we didn't find an empty list, then try
	// looking a bit further for a short one
	
	for(i = 0; i < 10 && lowest > 3; i++)
	 {
		x++;
		x %=  YAFFS_NOBJECT_BUCKETS;
		if(dev->objectBucket[x].count < lowest)
		{
			lowest = dev->objectBucket[x].count;
			l = x;
		}
		
	}
	
	return l;
}

static int yaffs_CreateNewObjectNumber(yaffs_Device *dev)
{
	int bucket = yaffs_FindNiceObjectBucket(dev);
	
	// Now find an object value that has not already been taken
	// by scanning the list.
	
	int found = 0;
	struct fs_list_head *i;
	
	uint32_t n = (uint32_t)bucket;

	//yaffs_CheckObjectHashSanity();	
	
	while(!found)
	{
		found = 1;
		n +=  YAFFS_NOBJECT_BUCKETS;
		if(1 ||dev->objectBucket[bucket].count > 0)
		{
			fs_list_for_each(i,&dev->objectBucket[bucket].list)
			{
				// If there is already one in the list
				if(i && fs_list_entry(i, yaffs_Object,hashLink)->objectId == n)
				{
					found = 0;
				}
			}
		}
	}
	
	//T(("bucket %d count %d inode %d\n",bucket,yaffs_objectBucket[bucket].count,n);
	
	return n;	
}

void yaffs_HashObject(yaffs_Object *inObj)
{
	int bucket = yaffs_HashFunction(inObj->objectId);
	yaffs_Device *dev = inObj->myDev;
	
	if(!fs_list_empty(&inObj->hashLink))
	{
		//YINFO("!!!");
	}

	
	fs_list_add(&inObj->hashLink,&dev->objectBucket[bucket].list);
	dev->objectBucket[bucket].count++;

}

yaffs_Object *yaffs_FindObjectByNumber(yaffs_Device *dev,uint32_t number)
{
	int bucket = yaffs_HashFunction(number);
	struct fs_list_head *i;
	yaffs_Object *inObj;
	
	fs_list_for_each(i,&dev->objectBucket[bucket].list)
	{
		// Look if it is in the list
		if(i)
		{
			inObj = fs_list_entry(i, yaffs_Object,hashLink);
			if(inObj->objectId == number)
			{
#ifdef __KERNEL__
				// Don't tell the VFS about this if it has been marked for freeing
 				if(inObj->deferedFree) 
				    return NULL;
#endif
				return inObj;
			}
		}
	}
	
	return NULL;
}



yaffs_Object *yaffs_CreateNewObject(yaffs_Device *dev,int number,yaffs_ObjectType type)
{
		
	yaffs_Object *theObject;

	if(number < 0)
	{
		number = yaffs_CreateNewObjectNumber(dev);
	}
	
	theObject = yaffs_AllocateEmptyObject(dev);
	
	if(theObject)
	{
		theObject->fake = 0;
		theObject->renameAllowed = 1;
		theObject->unlinkAllowed = 1;
		theObject->objectId = number;
		yaffs_HashObject(theObject);
		theObject->variantType = type;
#ifdef CONFIG_YAFFS_WINCE
		yfsd_WinFileTimeNow(theObject->win_atime);
		theObject->win_ctime[0] = theObject->win_mtime[0] = theObject->win_atime[0];
		theObject->win_ctime[1] = theObject->win_mtime[1] = theObject->win_atime[1];

#else

		theObject->yst_atime = theObject->yst_mtime =	theObject->yst_ctime = Y_CURRENT_TIME;

#endif
		switch(type)
		{
			case YAFFS_OBJECT_TYPE_FILE: 
				theObject->variant.fileVariant.fileSize = 0;
				theObject->variant.fileVariant.scannedFileSize = 0;
				theObject->variant.fileVariant.topLevel = 0;
				theObject->variant.fileVariant.top  = yaffs_GetTnode(dev);
				break;
			case YAFFS_OBJECT_TYPE_DIRECTORY:
				FS_INIT_LIST_HEAD(&theObject->variant.directoryVariant.children);
				break;
			case YAFFS_OBJECT_TYPE_SYMLINK:
				// No action required
				break;
			case YAFFS_OBJECT_TYPE_HARDLINK:
				// No action required
				break;
			case YAFFS_OBJECT_TYPE_SPECIAL:
				// No action required
				break;
			case YAFFS_OBJECT_TYPE_UNKNOWN:
				// todo this should not happen
				break;
		}
	}
	
	return theObject;
}

yaffs_Object *yaffs_FindOrCreateObjectByNumber(yaffs_Device *dev, int number,yaffs_ObjectType type)
{
	yaffs_Object *theObject = NULL;
	
	if(number > 0)
	{
		theObject = yaffs_FindObjectByNumber(dev,number);
	}
	
	if(!theObject)
	{
		theObject = yaffs_CreateNewObject(dev,number,type);
	}
	
	return theObject;

}

char *yaffs_CloneString(const char *str)
{
	char *newStr = NULL;
	
	if(str && *str)
	{
		newStr = YMALLOC(TEE_StrLen(str) + 1);
		TEE_StrCopy(newStr,str);
	}

	return newStr;
	
}

//
// Mknod (create) a new object.
// equivalentObject only has meaning for a hard link;
// aliasString only has meaning for a sumlink.
// rdev only has meaning for devices (a subset of special objects)
yaffs_Object *yaffs_MknodObject( yaffs_ObjectType type,
								 yaffs_Object *parent,
								 const char *name, 
								 uint32_t mode,
								 uint32_t uid,
								 uint32_t gid,
								 yaffs_Object *equivalentObject,
								 const char *aliasString,
								 uint32_t rdev)
{
	yaffs_Object *inObj;

	yaffs_Device *dev = parent->myDev;
	
	// Check if the entry exists. If it does then fail the call since we don't want a dup.
	if(yaffs_FindObjectByName(parent,name))
	{
		return NULL;
	}
	
	inObj = yaffs_CreateNewObject(dev,-1,type);
	
	if(inObj)
	{
		inObj->chunkId = -1;
		inObj->valid = 1;
		inObj->variantType = type;

		inObj->yst_mode  = mode;
		
#ifdef CONFIG_YAFFS_WINCE
		yfsd_WinFileTimeNow(inObj->win_atime);
		inObj->win_ctime[0] = inObj->win_mtime[0] = inObj->win_atime[0];
		inObj->win_ctime[1] = inObj->win_mtime[1] = inObj->win_atime[1];
		
#else

		inObj->yst_atime = inObj->yst_mtime = inObj->yst_ctime = Y_CURRENT_TIME;
		inObj->yst_rdev  = rdev;
		inObj->yst_uid   = uid;
		inObj->yst_gid   = gid;
#endif		
		inObj->nDataChunks = 0;

		yaffs_SetObjectName(inObj,name);
		inObj->dirty = 1;
		
		yaffs_AddObjectToDirectory(parent,inObj);
		
		inObj->myDev = parent->myDev;
		
				
		switch(type)
		{
			case YAFFS_OBJECT_TYPE_SYMLINK:
				inObj->variant.symLinkVariant.alias = yaffs_CloneString(aliasString);
				break;
			case YAFFS_OBJECT_TYPE_HARDLINK:
				inObj->variant.hardLinkVariant.equivalentObject = equivalentObject;
				inObj->variant.hardLinkVariant.equivalentObjectId = equivalentObject->objectId;
				fs_list_add(&inObj->hardLinks,&equivalentObject->hardLinks);
				break;
			case YAFFS_OBJECT_TYPE_FILE: // do nothing
			case YAFFS_OBJECT_TYPE_DIRECTORY: // do nothing
			case YAFFS_OBJECT_TYPE_SPECIAL: // do nothing
			case YAFFS_OBJECT_TYPE_UNKNOWN:
				break;
		}

		if(/*yaffs_GetNumberOfFreeChunks(dev) <= 0 || */
		   yaffs_UpdateObjectHeader(inObj,name,0) < 0)
		{
			// Could not create the object header, fail the creation
			yaffs_AbortHalfCreatedObject(inObj);
			inObj = NULL;
		}

	}
	
	return inObj;
}

yaffs_Object *yaffs_MknodFile(yaffs_Object *parent,const char *name, uint32_t mode, uint32_t uid, uint32_t gid)
{
	return yaffs_MknodObject(YAFFS_OBJECT_TYPE_FILE,parent,name,mode,uid,gid,NULL,NULL,0);
}

yaffs_Object *yaffs_MknodDirectory(yaffs_Object *parent,const char *name, uint32_t mode, uint32_t uid, uint32_t gid)
{
	return yaffs_MknodObject(YAFFS_OBJECT_TYPE_DIRECTORY,parent,name,mode,uid,gid,NULL,NULL,0);
}

yaffs_Object *yaffs_MknodSpecial(yaffs_Object *parent,const char *name, uint32_t mode, uint32_t uid, uint32_t gid, uint32_t rdev)
{
	return yaffs_MknodObject(YAFFS_OBJECT_TYPE_SPECIAL,parent,name,mode,uid,gid,NULL,NULL,rdev);
}

yaffs_Object *yaffs_MknodSymLink(yaffs_Object *parent,const char *name, uint32_t mode, uint32_t uid, uint32_t gid,const char *alias)
{
	return yaffs_MknodObject(YAFFS_OBJECT_TYPE_SYMLINK,parent,name,mode,uid,gid,NULL,alias,0);
}

// NB yaffs_Link returns the object id of the equivalent object.
yaffs_Object *yaffs_Link(yaffs_Object *parent, const char *name, yaffs_Object *equivalentObject)
{
	// Get the real object in case we were fed a hard link as an equivalent object
	equivalentObject = yaffs_GetEquivalentObject(equivalentObject);
	
	if(yaffs_MknodObject(YAFFS_OBJECT_TYPE_HARDLINK,parent,name,0,0,0,equivalentObject,NULL,0))
	{
		return equivalentObject;
	}
	else
	{
		return NULL;
	}
	
}


static int yaffs_ChangeObjectName(yaffs_Object *obj, yaffs_Object *newDir, const char *newName,int force)
{
	int unlinkOp;

	if(newDir == NULL)
	{
		newDir = obj->parent; // use the old directory
	}

	unlinkOp = (newDir == obj->myDev->unlinkedDir && obj->variantType == YAFFS_OBJECT_TYPE_FILE);
	
	// If the object is a file going into the unlinked directory, then it is OK to just stuff it in since
	// duplicate names are allowed.
	// Otherwise only proceed if the new name does not exist and if we're putting it into a directory.
	if( (unlinkOp|| 
		 force || 
		 !yaffs_FindObjectByName(newDir,newName))  &&
	     newDir->variantType == YAFFS_OBJECT_TYPE_DIRECTORY)
	{
		yaffs_SetObjectName(obj,newName);
		obj->dirty = 1;
		
		yaffs_AddObjectToDirectory(newDir,obj);
		
		if(unlinkOp) obj->unlinked = 1;
		
		
		if(yaffs_UpdateObjectHeader(obj,newName,0) >= 0)
		{
			return YAFFS_OK;
		}
	}
	
	return YAFFS_FAIL;
}



int yaffs_RenameObject(yaffs_Object *oldDir, const char *oldName, yaffs_Object *newDir, const char *newName)
{
	yaffs_Object *obj;
	int force = 0;
	
#ifdef CONFIG_YAFFS_CASE_INSENSITIVE
	// Special case for WinCE.
	// While look-up is case insensitive, the name isn't.
	// THerefore we might want to change x.txt to X.txt
	if(oldDir == newDir && _stricmp(oldName,newName) == 0)
	{
		force = 1;
	}	
#endif
	
	obj = yaffs_FindObjectByName(oldDir,oldName);
	if(obj && obj->renameAllowed)
	{
		return yaffs_ChangeObjectName(obj,newDir,newName,force);
	}
	return YAFFS_FAIL;
}


#if 0
//defined but not used
static int yaffs_CheckObjectHashSanity(yaffs_Device *dev)
{
	// Scan the buckets and check that the lists 
	// have as many members as the count says there are
	int bucket;
	int countEm;
	struct fs_list_head *j;
	int ok = YAFFS_OK;
	
	for(bucket = 0; bucket < YAFFS_NOBJECT_BUCKETS; bucket++)
	{
		countEm = 0;
		
		fs_list_for_each(j,&dev->objectBucket[bucket].list)
		{
			countEm++;
		}
		
		if(countEm != dev->objectBucket[bucket].count)
		{
			T(YAFFS_TRACE_ERROR,(TSTR("Inode hash inconsistency" TENDSTR)));
			ok = YAFFS_FAIL;
		}
	}

	return ok;
}

#endif

#if 0
void yaffs_ObjectTest(yaffs_Device *dev)
{
	yaffs_Object *inObj[1000];
	int inNo[1000];
	yaffs_Object *inold[1000];
	int i;
	int j;
	
	TEE_MemFill(inObj,0,1000*sizeof(yaffs_Object *));
	TEE_MemFill(inold,0,1000*sizeof(yaffs_Object *));
	
	yaffs_CheckObjectHashSanity(dev);
	
	for(j = 0; j < 10; j++)
	{
		//T(("%d\n",j));
		
		for(i = 0; i < 1000; i++)
		{
			inObj[i] = yaffs_CreateNewObject(dev,-1,YAFFS_OBJECT_TYPE_FILE);
			if(!inObj[i])
			{
				YINFO("No more inodes");
			}
			else
			{
				inNo[i] = inObj[i]->objectId;
			}
		}
		
		for(i = 0; i < 1000; i++)
		{
			if(yaffs_FindObjectByNumber(dev,inNo[i]) != inObj[i])
			{
				//T(("Differnce in look up test\n"));
			}
			else
			{
				// T(("Look up ok\n"));
			}
		}
		
		yaffs_CheckObjectHashSanity(dev);
	
		for(i = 0; i < 1000; i+=3)
		{
			yaffs_FreeObject(inObj[i]);	
			inObj[i] = NULL;
		}
		
	
		yaffs_CheckObjectHashSanity(dev);
	}
		
}

#endif

/////////////////////////// Block Management and Page Allocation ///////////////////


static int yaffs_InitialiseBlocks(yaffs_Device *dev,int nBlocks)
{
	dev->allocationBlock = -1; // force it to get a new one
	//Todo we're assuming the malloc will pass.
	dev->blockInfo = YMALLOC(nBlocks * sizeof(yaffs_BlockInfo));
	// Set up dynamic blockinfo stuff.
	dev->chunkBitmapStride = (dev->nChunksPerBlock+7)/8;
	dev->chunkBits = YMALLOC(dev->chunkBitmapStride * nBlocks);
	if(dev->blockInfo && dev->chunkBits)
	{
		TEE_MemFill(dev->blockInfo,0,nBlocks * sizeof(yaffs_BlockInfo));
		TEE_MemFill(dev->chunkBits,0,dev->chunkBitmapStride * nBlocks);
		return YAFFS_OK;
	}
	
	return YAFFS_FAIL;
	
}

static void yaffs_DeinitialiseBlocks(yaffs_Device *dev)
{
	YFREE(dev->blockInfo);
	dev->blockInfo = NULL;
	YFREE(dev->chunkBits);
	dev->chunkBits = NULL;
}

// FindDiretiestBlock is used to select the dirtiest block (or close enough)
// for garbage collection.

static int yaffs_FindDirtiestBlock(yaffs_Device *dev,int aggressive)
{

	int b = dev->currentDirtyChecker;
	
	int i;
	int iterations;
	int dirtiest = -1;
	int pagesInUse; 
	yaffs_BlockInfo *bi;

	// If we're doing aggressive GC then we are happy to take a less-dirty block, and
	// search further.
	
	pagesInUse = (aggressive)? dev->nChunksPerBlock : YAFFS_PASSIVE_GC_CHUNKS + 1;
	if(aggressive)
	{
		iterations = dev->internalEndBlock - dev->internalStartBlock + 1;
	}
	else
	{
		iterations = dev->internalEndBlock - dev->internalStartBlock + 1;
		iterations = iterations / 16;
		if(iterations > 200)
		{
			iterations = 200;
		}
	}
	
	for(i = 0; i <= iterations && pagesInUse > 0 ; i++)
	{
		b++;
		if ( b < dev->internalStartBlock || b > dev->internalEndBlock)
		{
			b =  dev->internalStartBlock;
		}

		if(b < dev->internalStartBlock || b > dev->internalEndBlock)
		{
			T(YAFFS_TRACE_ERROR,(TSTR("**>> Block %d is not valid" TENDSTR),b));
			YBUG();
		}
		
		bi = yaffs_GetBlockInfo(dev,b);
		
		if(bi->blockState == YAFFS_BLOCK_STATE_FULL &&
		   (bi->pagesInUse - bi->softDeletions )< pagesInUse)
		{
			dirtiest = b;
			pagesInUse = (bi->pagesInUse - bi->softDeletions);
		}
	}
	
	dev->currentDirtyChecker = b;
	
	return dirtiest;
}


static void yaffs_BlockBecameDirty(yaffs_Device *dev,int blockNo)
{
	yaffs_BlockInfo *bi = yaffs_GetBlockInfo(dev,blockNo);
	
	int erasedOk = 0;
	
	// If the block is still healthy erase it and mark as clean.
	// If the block has had a data failure, then retire it.
	bi->blockState = YAFFS_BLOCK_STATE_DIRTY;

	if(!bi->needsRetiring)
	{
		erasedOk = yaffs_EraseBlockInNAND(dev,blockNo);
		if(!erasedOk)
		{
			T(YAFFS_TRACE_ERROR | YAFFS_TRACE_BAD_BLOCKS,(TSTR("**>> Erasure failed %d" TENDSTR),blockNo));
		}
	}
	
	if( erasedOk )
	{
		// Clean it up...
		bi->blockState = YAFFS_BLOCK_STATE_EMPTY;
		dev->nErasedBlocks++;
		bi->pagesInUse = 0;
		bi->softDeletions = 0;
		yaffs_ClearChunkBits(dev,blockNo);
	
		T(YAFFS_TRACE_ERASE,(TSTR("Erased block %d" TENDSTR),blockNo));
	}
	else
	{
		yaffs_RetireBlock(dev,blockNo);
		T(YAFFS_TRACE_ERROR | YAFFS_TRACE_BAD_BLOCKS,(TSTR("**>> Block %d retired" TENDSTR),blockNo));
	}
}


static int yaffs_FindBlockForAllocation(yaffs_Device *dev)
{
	int i;
	
	yaffs_BlockInfo *bi;
	
	if(dev->nErasedBlocks < 1)
	{
		// Hoosterman we've got a problem.
		// Can't get space to gc
		T(YAFFS_TRACE_ERROR, (TSTR("yaffs tragedy: no space during gc" TENDSTR)));

		return -1;
	}
	
	// Find an empty block.
	
	for(i = dev->internalStartBlock; i <= dev->internalEndBlock; i++)
	{
		dev->allocationBlockFinder++;
		if(dev->allocationBlockFinder <dev->internalStartBlock || dev->allocationBlockFinder> dev->internalEndBlock) 
		{
			dev->allocationBlockFinder = dev->internalStartBlock;
		}
		
		bi = yaffs_GetBlockInfo(dev,dev->allocationBlockFinder);

		if(bi->blockState == YAFFS_BLOCK_STATE_EMPTY)
		{
			bi->blockState = YAFFS_BLOCK_STATE_ALLOCATING;
			dev->nErasedBlocks--;			
			return dev->allocationBlockFinder;
		}
	}
	
	return -1;	
}



static int yaffs_AllocateChunk(yaffs_Device *dev,int useReserve)
{
	int retVal;
	yaffs_BlockInfo *bi;
	
	if(dev->allocationBlock < 0)
	{
		// Get next block to allocate off
		dev->allocationBlock = yaffs_FindBlockForAllocation(dev);
		dev->allocationPage = 0;
	}
	
	if(!useReserve &&  dev->nErasedBlocks <= dev->nReservedBlocks)
	{
		// Not enough space to allocate unless we're allowed to use the reserve.
		return -1;
	}
	
	// Next page please....
	if(dev->allocationBlock >= 0)
	{
		bi = yaffs_GetBlockInfo(dev,dev->allocationBlock);
		
		retVal = (dev->allocationBlock * dev->nChunksPerBlock) + 
			  	  dev->allocationPage;
		bi->pagesInUse++;
		yaffs_SetChunkBit(dev,dev->allocationBlock,dev->allocationPage);

		dev->allocationPage++;
		
		dev->nFreeChunks--;
		
		// If the block is full set the state to full
		if(dev->allocationPage >= dev->nChunksPerBlock)
		{
			bi->blockState = YAFFS_BLOCK_STATE_FULL;
			dev->allocationBlock = -1;
		}


		return retVal;
		
	}
	T(YAFFS_TRACE_ERROR,(TSTR("!!!!!!!!! Allocator out !!!!!!!!!!!!!!!!!" TENDSTR)));

	return -1;	
}

// To determine if we have enough space we just look at the 
// number of erased blocks.
// The cache is allowed to use reserved blocks.

static int yaffs_CheckSpaceForChunkCache(yaffs_Device *dev)
{
	return (dev->nErasedBlocks >= dev->nReservedBlocks);
}


static int  yaffs_GarbageCollectBlock(yaffs_Device *dev,int block)
{
	int oldChunk;
	int newChunk;
	int chunkInBlock;
	int markNAND;
	
	
	yaffs_Spare spare;
	yaffs_Tags  tags;
	uint8_t  buffer[YAFFS_BYTES_PER_CHUNK];
	
//	yaffs_BlockInfo *bi = yaffs_GetBlockInfo(dev,block);
	
	yaffs_Object *object;

	//T(("Collecting block %d n %d bits %x\n",block, bi->pagesInUse, bi->pageBits));	
	
	for(chunkInBlock = 0,oldChunk = block * dev->nChunksPerBlock; 
	    chunkInBlock < dev->nChunksPerBlock && yaffs_StillSomeChunkBits(dev,block);
	    chunkInBlock++, oldChunk++ )
	{
		if(yaffs_CheckChunkBit(dev,block,chunkInBlock))
		{
			
			// This page is in use and might need to be copied off
			
			markNAND = 1;
			
			//T(("copying page %x from %d to %d\n",mask,oldChunk,newChunk));
			
			yaffs_ReadChunkFromNAND(dev,oldChunk,buffer, &spare,1);
			
			yaffs_GetTagsFromSpare(dev,&spare,&tags);

			object = yaffs_FindObjectByNumber(dev,tags.objectId);
			
			if(object && object->deleted && tags.chunkId != 0)
			{
				// Data chunk in a deleted file, throw it away
				// It's a deleted data chunk,
				// No need to copy this, just forget about it and fix up the
				// object.
				
				//yaffs_PutChunkIntoFile(object, tags.chunkId, 0,0); 
				object->nDataChunks--;
				
				if(object->nDataChunks <= 0)
				{
					// Time to delete the file too
					yaffs_FreeTnode(object->myDev,object->variant.fileVariant.top);
					object->variant.fileVariant.top = NULL;
					T(YAFFS_TRACE_TRACING,(TSTR("yaffs: About to finally delete object %d" TENDSTR),object->objectId));
					yaffs_DoGenericObjectDeletion(object);					
 				}
				markNAND = 0;
			}
			else if( 0 /* Todo object && object->deleted && object->nDataChunks == 0 */)
			{
				// Deleted object header with no data chunks.
				// Can be discarded and the file deleted.
				object->chunkId = 0;
				yaffs_FreeTnode(object->myDev,object->variant.fileVariant.top);
				object->variant.fileVariant.top = NULL;
				yaffs_DoGenericObjectDeletion(object);
				
			}
			else if(object)
			{
				// It's either a data chunk in a live file or
				// an ObjectHeader, so we're interested in it.
				// NB Need to keep the ObjectHeaders of deleted files
				// until the whole file has been deleted off
				tags.serialNumber++;
				yaffs_LoadTagsIntoSpare(&spare,&tags);

				dev->nGCCopies++;

				newChunk = yaffs_WriteNewChunkToNAND(dev, buffer, &spare,1);
			
				if(newChunk < 0)
				{
					return YAFFS_FAIL;
				}
			
				// Ok, now fix up the Tnodes etc.
			
				if(tags.chunkId == 0)
				{
					// It's a header
					object->chunkId = newChunk;
					object->serial = tags.serialNumber;
				}
				else
				{
					// It's a data chunk
					yaffs_PutChunkIntoFile(object, tags.chunkId, newChunk,0);
				}
			}
			
			yaffs_DeleteChunk(dev,oldChunk,markNAND);			
			
		}
	}

	return YAFFS_OK;
}

#if 0
//defined but not used
static yaffs_Object *yaffs_FindDeletedUnlinkedFile(yaffs_Device *dev)
{
	// find a file to delete
	struct fs_list_head *i;	
	yaffs_Object *l;


	//Scan the unlinked files looking for one to delete
	fs_list_for_each(i,&dev->unlinkedDir->variant.directoryVariant.children)
	{
		if(i)
		{
			l = fs_list_entry(i, yaffs_Object,siblings);
			if(l->deleted)
			{
				return l;			
			}
		}
	}	
	return NULL;
}
#endif

#if 0
//defined but not used
static void yaffs_DoUnlinkedFileDeletion(yaffs_Device *dev)
{
	// This does background deletion on unlinked files.. only deleted ones.
	// If we don't have a file we're working on then find one
	if(!dev->unlinkedDeletion && dev->nDeletedFiles > 0)
	{
		dev->unlinkedDeletion = yaffs_FindDeletedUnlinkedFile(dev);
	}
	
	// OK, we're working on a file...
	if(dev->unlinkedDeletion)
	{
		yaffs_Object *obj = dev->unlinkedDeletion;
		int delresult;
		int limit; // Number of chunks to delete in a file.
				   // NB this can be exceeded, but not by much.
				   
		limit = -1;

		delresult = yaffs_DeleteWorker(obj, obj->variant.fileVariant.top, obj->variant.fileVariant.topLevel, 0,&limit);
		
		if(obj->nDataChunks == 0)
		{
			// Done all the deleting of data chunks.
			// Now dump the header and clean up
			yaffs_FreeTnode(dev,obj->variant.fileVariant.top);
			obj->variant.fileVariant.top = NULL;
			yaffs_DoGenericObjectDeletion(obj);
			dev->nDeletedFiles--;
			dev->nUnlinkedFiles--;
			dev->nBackgroundDeletions++;
			dev->unlinkedDeletion = NULL;	
		}
	}
}
#endif


#if 0
#define YAFFS_GARBAGE_COLLECT_LOW_WATER 2
static int yaffs_CheckGarbageCollection(yaffs_Device *dev)
{
	int block;
	int aggressive=0;
	
	//yaffs_DoUnlinkedFileDeletion(dev);
	
	if(dev->nErasedBlocks <= (dev->nReservedBlocks + YAFFS_GARBAGE_COLLECT_LOW_WATER))
	{
		aggressive = 1;
	}		
	
	if(aggressive)
	{
		block = yaffs_FindDirtiestBlock(dev,aggressive);
		
		if(block >= 0)
		{
			dev->garbageCollections++;
			return yaffs_GarbageCollectBlock(dev,block);
		}	
		else
		{
			return YAFFS_FAIL;
		}
	}

	return YAFFS_OK;
}
#endif

// New garbage collector
// If we're very low on erased blocks then we do aggressive garbage collection
// otherwise we do "passive" garbage collection.
// Aggressive gc looks further (whole array) and will accept dirtier blocks.
// Passive gc only inspects smaller areas and will only accept cleaner blocks.
//
// The idea is to help clear out space in a more spread-out manner.
// Dunno if it really does anything useful.
//
static int yaffs_CheckGarbageCollection(yaffs_Device *dev)
{
	int block;
	int aggressive=0;
	
	//yaffs_DoUnlinkedFileDeletion(dev);
	
	if(dev->nErasedBlocks <= (dev->nReservedBlocks + 1))
	{
		aggressive = 1;
	}		
	
	block = yaffs_FindDirtiestBlock(dev,aggressive);
	
	if(block >= 0)
	{
		dev->garbageCollections++;
		if(!aggressive)
		{
			dev->passiveGarbageCollections++;
		}

		T(YAFFS_TRACE_GC,(TSTR("yaffs: GC erasedBlocks %d aggressive %d" TENDSTR),dev->nErasedBlocks,aggressive));

		return yaffs_GarbageCollectBlock(dev,block);
	}	

	return aggressive ? YAFFS_FAIL : YAFFS_OK;
}


//////////////////////////// TAGS ///////////////////////////////////////

static void yaffs_LoadTagsIntoSpare(yaffs_Spare *sparePtr, yaffs_Tags *tagsPtr)
{
	yaffs_TagsUnion *tu = (yaffs_TagsUnion *)tagsPtr;
	
	yaffs_CalcTagsECC(tagsPtr);
	
	sparePtr->tagByte0 = tu->asBytes[0];
	sparePtr->tagByte1 = tu->asBytes[1];
	sparePtr->tagByte2 = tu->asBytes[2];
	sparePtr->tagByte3 = tu->asBytes[3];
	sparePtr->tagByte4 = tu->asBytes[4];
	sparePtr->tagByte5 = tu->asBytes[5];
	sparePtr->tagByte6 = tu->asBytes[6];
	sparePtr->tagByte7 = tu->asBytes[7];
}

static void yaffs_GetTagsFromSpare(yaffs_Device *dev, yaffs_Spare *sparePtr,yaffs_Tags *tagsPtr)
{
	yaffs_TagsUnion *tu = (yaffs_TagsUnion *)tagsPtr;
	int result;

	tu->asBytes[0]= sparePtr->tagByte0;
	tu->asBytes[1]= sparePtr->tagByte1;
	tu->asBytes[2]= sparePtr->tagByte2;
	tu->asBytes[3]= sparePtr->tagByte3;
	tu->asBytes[4]= sparePtr->tagByte4;
	tu->asBytes[5]= sparePtr->tagByte5;
	tu->asBytes[6]= sparePtr->tagByte6;
	tu->asBytes[7]= sparePtr->tagByte7;
	
	result =  yaffs_CheckECCOnTags(tagsPtr);
	if(result> 0)
	{
		dev->tagsEccFixed++;
	}
	else if(result <0)
	{
		dev->tagsEccUnfixed++;
	}
}

static void yaffs_SpareInitialise(yaffs_Spare *spare)
{
	TEE_MemFill(spare,0xFF,sizeof(yaffs_Spare));
}

static int yaffs_ReadChunkTagsFromNAND(yaffs_Device *dev,int chunkInNAND, yaffs_Tags *tags, int *chunkDeleted)
{
	if(tags)
	{
		yaffs_Spare spare;
		if(yaffs_ReadChunkFromNAND(dev,chunkInNAND,NULL,&spare,1) == YAFFS_OK)
		{
			*chunkDeleted = (yaffs_CountBits(spare.pageStatus) < 7) ? 1 : 0;
			yaffs_GetTagsFromSpare(dev,&spare,tags);
			return YAFFS_OK;
		}
		else
		{
			return YAFFS_FAIL;
		}
	}
	
	return YAFFS_OK;
}

#if 0
static int yaffs_WriteChunkWithTagsToNAND(yaffs_Device *dev,int chunkInNAND, const uint8_t *buffer, yaffs_Tags *tags)
{
	// NB There must be tags, data is optional
	// If there is data, then an ECC is calculated on it.
	
	yaffs_Spare spare;
	
	if(!tags)
	{
		return YAFFS_FAIL;
	}
	
	yaffs_SpareInitialise(&spare);
	
	if(!dev->useNANDECC && buffer)
	{
		yaffs_CalcECC(buffer,&spare);
	}
	
	yaffs_LoadTagsIntoSpare(&spare,tags);
	
	return yaffs_WriteChunkToNAND(dev,chunkInNAND,buffer,&spare);
	
}
#endif


static int yaffs_WriteNewChunkWithTagsToNAND(yaffs_Device *dev, const uint8_t *buffer, yaffs_Tags *tags, int useReserve)
{
	// NB There must be tags, data is optional
	// If there is data, then an ECC is calculated on it.
	
	yaffs_Spare spare;
	
	if(!tags)
	{
		return YAFFS_FAIL;
	}
	
	yaffs_SpareInitialise(&spare);
	
	if(!dev->useNANDECC && buffer)
	{
		yaffs_CalcECC(buffer,&spare);
	}
	
	yaffs_LoadTagsIntoSpare(&spare,tags);
	
	return yaffs_WriteNewChunkToNAND(dev,buffer,&spare,useReserve);
	
}

static int yaffs_TagsMatch(const yaffs_Tags *tags, int objectId, int chunkInObject, int chunkDeleted)
{
	return 	(  tags->chunkId == chunkInObject &&
			   tags->objectId == objectId &&
			   !chunkDeleted) ? 1 : 0;
	
}



int yaffs_FindChunkInFile(yaffs_Object *inObj,int chunkInInode,yaffs_Tags *tags)
{
	//Get the Tnode, then get the level 0 offset chunk offset
    yaffs_Tnode *tn;     
    int theChunk = -1;
    yaffs_Tags localTags;
    int i;
    int found = 0;
    int chunkDeleted;
    
    yaffs_Device *dev = inObj->myDev;
    
    
    if(!tags)
    {
    	// Passed a NULL, so use our own tags space
    	tags = &localTags;
    }
    
    tn = yaffs_FindLevel0Tnode(dev,&inObj->variant.fileVariant, chunkInInode);
    
    if(tn)
    {
		theChunk = tn->level0[chunkInInode & YAFFS_TNODES_LEVEL0_MASK] << dev->chunkGroupBits;

		// Now we need to do the shifting etc and search for it
		for(i = 0,found = 0; theChunk && i < dev->chunkGroupSize && !found; i++)
		{
			yaffs_ReadChunkTagsFromNAND(dev,theChunk,tags,&chunkDeleted);
			if(yaffs_TagsMatch(tags,inObj->objectId,chunkInInode,chunkDeleted))
			{
				// found it;
				found = 1;
			}
			else
			{
				theChunk++;
			}
		}
    }
    return found ? theChunk : -1;
}

int yaffs_FindAndDeleteChunkInFile(yaffs_Object *inObj,int chunkInInode,yaffs_Tags *tags)
{
	//Get the Tnode, then get the level 0 offset chunk offset
    yaffs_Tnode *tn;     
    int theChunk = -1;
    yaffs_Tags localTags;
    int i;
    int found = 0;
    yaffs_Device *dev = inObj->myDev;
    int chunkDeleted;
    
    if(!tags)
    {
    	// Passed a NULL, so use our own tags space
    	tags = &localTags;
    }
    
    tn = yaffs_FindLevel0Tnode(dev,&inObj->variant.fileVariant, chunkInInode);
    
    if(tn)
    {
    
		theChunk = tn->level0[chunkInInode & YAFFS_TNODES_LEVEL0_MASK] << dev->chunkGroupBits;
    
		// Now we need to do the shifting etc and search for it
		for(i = 0,found = 0; theChunk && i < dev->chunkGroupSize && !found; i++)
		{
			yaffs_ReadChunkTagsFromNAND(dev,theChunk,tags,&chunkDeleted);
			if(yaffs_TagsMatch(tags,inObj->objectId,chunkInInode,chunkDeleted))
			{
				// found it;
				found = 1;
			}
			else
			{
				theChunk++;
			}
		}
    
		// Delete the entry in the filestructure
		if(found)
		{
			tn->level0[chunkInInode & YAFFS_TNODES_LEVEL0_MASK] = 0;
		}
    }
    else
    {
    	//T(("No level 0 found for %d\n", chunkInInode));
    }
    
    if(!found)
    {
    	//T(("Could not find %d to delete\n",chunkInInode));
    }
    return found ? theChunk : -1;
}


#ifdef YAFFS_PARANOID

static int yaffs_CheckFileSanity(yaffs_Object *inObj)
{
	int chunk;
	int nChunks;
	int fSize;
	int failed = 0;
	int objId;
	yaffs_Tnode *tn;
    yaffs_Tags localTags;
    yaffs_Tags *tags = &localTags;
    int theChunk;
    int chunkDeleted;
    
    	
	if(inObj->variantType != YAFFS_OBJECT_TYPE_FILE)
	{
		//T(("Object not a file\n"));
		return YAFFS_FAIL;
	}
	
	objId = inObj->objectId;
	fSize  = inObj->variant.fileVariant.fileSize;
	nChunks = (fSize + inObj->myDev->nBytesPerChunk -1)/inObj->myDev->nBytesPerChunk;
	
	for(chunk = 1; chunk <= nChunks; chunk++)
	{
		tn = yaffs_FindLevel0Tnode(inObj->myDev,&inObj->variant.fileVariant, chunk);
    
		if(tn)
		{
    
			theChunk = tn->level0[chunk & YAFFS_TNODES_LEVEL0_MASK] << inObj->myDev->chunkGroupBits;
    

				yaffs_ReadChunkTagsFromNAND(inObj->myDev,theChunk,tags,&chunkDeleted);
				if(yaffs_TagsMatch(tags,inObj->objectId,chunk,chunkDeleted))
				{
					// found it;
				
				}
				else
				{
					//T(("File problem file [%d,%d] NAND %d  tags[%d,%d]\n",
					//		objId,chunk,theChunk,tags->chunkId,tags->objectId);
							
					failed = 1;
								
				}
    
		}
		else
		{
			//T(("No level 0 found for %d\n", chunk));
		}
	}
	
	return failed ? YAFFS_FAIL : YAFFS_OK;
}

#endif

static int yaffs_PutChunkIntoFile(yaffs_Object *inObj,int chunkInInode, int chunkInNAND, int inScan)
{
	yaffs_Tnode *tn;
	yaffs_Device *dev = inObj->myDev;
	int existingChunk;
	yaffs_Tags existingTags;
	yaffs_Tags newTags;
	unsigned existingSerial, newSerial;
	
	int newChunkDeleted;
	
	
	tn = yaffs_AddOrFindLevel0Tnode(dev,&inObj->variant.fileVariant, chunkInInode);
	if(!tn)
	{
		return YAFFS_FAIL;
	}

	existingChunk = tn->level0[chunkInInode & YAFFS_TNODES_LEVEL0_MASK];		
	
	if(inScan)
	{
		// If we're scanning then we need to test for duplicates
		// NB This does not need to be efficient since it should only ever 
		// happen when the power fails during a write, then only one
		// chunk should ever be affected.
	
		
		if(existingChunk != 0)
		{
			// NB Right now existing chunk will not be real chunkId if the device >= 32MB
			//    thus we have to do a FindChunkInFile to get the real chunk id.
			//
			// We have a duplicate now we need to decide which one to use
			// To do this we get both sets of tags and compare serial numbers.
			yaffs_ReadChunkTagsFromNAND(dev,chunkInNAND, &newTags,&newChunkDeleted);
			
			
			// Do a proper find
			existingChunk = yaffs_FindChunkInFile(inObj,chunkInInode, &existingTags);

			if(existingChunk <=0)
			{
				//Hoosterman - how did this happen?
				
				T(YAFFS_TRACE_ERROR, (TSTR("yaffs tragedy: existing chunk < 0 in scan" TENDSTR)));

			}

			
			// NB The deleted flags should be false, otherwise the chunks will 
			// not be loaded during a scan
			
			newSerial = newTags.serialNumber;
			existingSerial = existingTags.serialNumber;
			
			if( existingChunk <= 0 ||
			    ((existingSerial+1) & 3) == newSerial)
			{
				// Use new
				// Delete the old one and drop through to update the tnode
				yaffs_DeleteChunk(dev,existingChunk,1);
			}
			else
			{
				// Use existing.
				// Delete the new one and return early so that the tnode isn't changed
				yaffs_DeleteChunk(dev,chunkInNAND,1);
				return YAFFS_OK;
			}
		}

	}
		
	if(existingChunk == 0)
	{
		inObj->nDataChunks++;
	}
	
	tn->level0[chunkInInode & YAFFS_TNODES_LEVEL0_MASK] = (chunkInNAND >> dev->chunkGroupBits);
	
	return YAFFS_OK;
}



int yaffs_ReadChunkDataFromObject(yaffs_Object *inObj,int chunkInInode, uint8_t *buffer)
{
    int chunkInNAND = yaffs_FindChunkInFile(inObj,chunkInInode,NULL);
    
    if(chunkInNAND >= 0)
    {
		return yaffs_ReadChunkFromNAND(inObj->myDev,chunkInNAND,buffer,NULL,1);
	}
	else
	{
		TEE_MemFill(buffer,0,YAFFS_BYTES_PER_CHUNK);
		return 0;
	}

}


static void yaffs_DeleteChunk(yaffs_Device *dev,int chunkId,int markNAND)
{
	int block;
	int page;
	yaffs_Spare spare;
	yaffs_BlockInfo *bi;
	
	if(chunkId <= 0) return;	
	
	dev->nDeletions++;
	block = (unsigned int)chunkId / (unsigned int)dev->nChunksPerBlock;
	page = chunkId % dev->nChunksPerBlock;
	
	if(markNAND)
	{
		yaffs_SpareInitialise(&spare);

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE

                //read data before write, to ensure correct ecc 
                //if we're using MTD verification under Linux
                yaffs_ReadChunkFromNAND(dev,chunkId,NULL,&spare,0);
#endif

		spare.pageStatus = 0; // To mark it as deleted.

	
		yaffs_WriteChunkToNAND(dev,chunkId,NULL,&spare);
		yaffs_HandleUpdateChunk(dev,chunkId,&spare);
	}
	else
	{
			dev->nUnmarkedDeletions++;
	}	
	
	bi = yaffs_GetBlockInfo(dev,block);
			
	
	// Pull out of the management area.
	// If the whole block became dirty, this will kick off an erasure.
	if(	bi->blockState == YAFFS_BLOCK_STATE_ALLOCATING ||
	    bi->blockState == YAFFS_BLOCK_STATE_FULL)
	{
		dev->nFreeChunks++;

		yaffs_ClearChunkBit(dev,block,page);
		bi->pagesInUse--;
		
		if(bi->pagesInUse == 0 &&
	       bi->blockState == YAFFS_BLOCK_STATE_FULL)
	    {
	    	yaffs_BlockBecameDirty(dev,block);
	    }

	}
	else
	{
		// T(("Bad news deleting chunk %d\n",chunkId));
	}
	
}




int yaffs_WriteChunkDataToObject(yaffs_Object *inObj,int chunkInInode, const uint8_t *buffer,int nBytes,int useReserve)
{
	// Find old chunk Need to do this to get serial number
	// Write new one and patch into tree.
	// Invalidate old tags.

    int prevChunkId;
    yaffs_Tags prevTags;
    
    int newChunkId;
    yaffs_Tags newTags;

    yaffs_Device *dev = inObj->myDev;    

	yaffs_CheckGarbageCollection(dev);

	// Get the previous chunk at this location in the file if it exists
    prevChunkId  = yaffs_FindChunkInFile(inObj,chunkInInode,&prevTags);
    
    // Set up new tags
   	newTags.chunkId = chunkInInode;
	newTags.objectId = inObj->objectId;
	newTags.serialNumber = (prevChunkId >= 0) ? prevTags.serialNumber + 1 : 1;
   	newTags.byteCount = nBytes;
	newTags.unusedStuff = 0xFFFFFFFF;
		
	yaffs_CalcTagsECC(&newTags);

	newChunkId = yaffs_WriteNewChunkWithTagsToNAND(dev,buffer,&newTags,useReserve);
	if(newChunkId >= 0)
	{
		yaffs_PutChunkIntoFile(inObj,chunkInInode,newChunkId,0);
		
		
		if(prevChunkId >= 0)
		{
			yaffs_DeleteChunk(dev,prevChunkId,1);
	
		}
		
		yaffs_CheckFileSanity(inObj);
	}
	return newChunkId;





}


// UpdateObjectHeader updates the header on NAND for an object.
// If name is not NULL, then that new name is used.
//
int yaffs_UpdateObjectHeader(yaffs_Object *inObj,const char *name, int force)
{

	yaffs_Device *dev = inObj->myDev;
	
    int prevChunkId;
    
    int newChunkId;
    yaffs_Tags newTags;
    uint8_t bufferNew[YAFFS_BYTES_PER_CHUNK];
    uint8_t bufferOld[YAFFS_BYTES_PER_CHUNK];
    
    yaffs_ObjectHeader *oh = (yaffs_ObjectHeader *)bufferNew;
    yaffs_ObjectHeader *ohOld = (yaffs_ObjectHeader *)bufferOld;

    
    if(!inObj->fake || force)
    {
  
		yaffs_CheckGarbageCollection(dev);		
    
		TEE_MemFill(bufferNew,0xFF,YAFFS_BYTES_PER_CHUNK);
    
		prevChunkId = inObj->chunkId;
    
		if(prevChunkId >= 0)
		{
			yaffs_ReadChunkFromNAND(dev,prevChunkId,bufferOld,NULL,1);    	
		}

		// Header data
		oh->type = inObj->variantType;
		
		oh->yst_mode = inObj->yst_mode;

#ifdef CONFIG_YAFFS_WINCE
		oh->win_atime[0] = inObj->win_atime[0];
		oh->win_ctime[0] = inObj->win_ctime[0];
		oh->win_mtime[0] = inObj->win_mtime[0];
		oh->win_atime[1] = inObj->win_atime[1];
		oh->win_ctime[1] = inObj->win_ctime[1];
		oh->win_mtime[1] = inObj->win_mtime[1];
#else
		oh->yst_uid = inObj->yst_uid;
		oh->yst_gid = inObj->yst_gid;
		oh->yst_atime = inObj->yst_atime;
		oh->yst_mtime = inObj->yst_mtime;
		oh->yst_ctime = inObj->yst_ctime;
		oh->yst_rdev = inObj->yst_rdev;
#endif	
		if(inObj->parent)
		{
			oh->parentObjectId = inObj->parent->objectId;
		}
		else
		{
			oh->parentObjectId = 0;
		}
		
		//oh->sum = inObj->sum;
		if(name && *name)
		{
			TEE_MemFill(oh->name,0,YAFFS_MAX_NAME_LENGTH + 1);
			TEE_StrnCpy(oh->name,name,YAFFS_MAX_NAME_LENGTH);
		}
		else if(prevChunkId)
		{	
			TEE_MemMove(oh->name, ohOld->name,YAFFS_MAX_NAME_LENGTH + 1);
		}
		else
		{
			TEE_MemFill(oh->name,0,YAFFS_MAX_NAME_LENGTH + 1);	
		}
	
		switch(inObj->variantType)
		{
			case YAFFS_OBJECT_TYPE_UNKNOWN: 	
				// Should not happen
				break;
			case YAFFS_OBJECT_TYPE_FILE:
				oh->fileSize = inObj->variant.fileVariant.fileSize;
				break;
			case YAFFS_OBJECT_TYPE_HARDLINK:
				oh->equivalentObjectId = inObj->variant.hardLinkVariant.equivalentObjectId;
				break;
			case YAFFS_OBJECT_TYPE_SPECIAL:	
				// Do nothing
				break;
			case YAFFS_OBJECT_TYPE_DIRECTORY:	
				// Do nothing
				break;
			case YAFFS_OBJECT_TYPE_SYMLINK:
				TEE_StrnCpy(oh->alias,inObj->variant.symLinkVariant.alias,YAFFS_MAX_ALIAS_LENGTH);
				oh->alias[YAFFS_MAX_ALIAS_LENGTH] = 0;
				break;
		}

		// Tags
		inObj->serial++;
		newTags.chunkId = 0;
		newTags.objectId = inObj->objectId;
		newTags.serialNumber = inObj->serial;
		newTags.byteCount =   0xFFFFFFFF;
		newTags.unusedStuff = 0xFFFFFFFF;
	
		yaffs_CalcTagsECC(&newTags);
	

		// Create new chunk in NAND
		newChunkId = yaffs_WriteNewChunkWithTagsToNAND(dev,bufferNew,&newTags, (prevChunkId >= 0) ? 1 : 0 );
    
		if(newChunkId >= 0)
		{
		
			inObj->chunkId = newChunkId;		
		
			if(prevChunkId >= 0)
			{
				yaffs_DeleteChunk(dev,prevChunkId,1);
			}
		
			inObj->dirty = 0;
		}
		
		return newChunkId;

    }
    return 0;
}


/////////////////////// Short Operations Cache ////////////////////////////////
//	In many siturations where there is no high level buffering (eg WinCE) a lot of
//	reads might be short sequential reads, and a lot of writes may be short 
//  sequential writes. eg. scanning/writing a jpeg file.
//	In these cases, a short read/write cache can provide a huge perfomance benefit 
//  with dumb-as-a-rock code.
//  There are a limited number (~10) of cache chunks per device so that we don't
//  need a very intelligent search.





static void yaffs_FlushFilesChunkCache(yaffs_Object *obj)
{
	yaffs_Device *dev = obj->myDev;
	int lowest = -1;
	int i;
	yaffs_ChunkCache *cache;
	int chunkWritten = 0;
//	int nBytes;	  // unused
	int nCaches = obj->myDev->nShortOpCaches;
	
	if  (nCaches > 0)
	{
		do{
			cache = NULL;
		
			// Find the dirty cache for this object with the lowest chunk id.
			for(i = 0; i < nCaches; i++)
			{
				if(dev->srCache[i].object == obj &&
				dev->srCache[i].dirty)
				{
					if(!cache ||  dev->srCache[i].chunkId < lowest)
					{
						cache = &dev->srCache[i];
						lowest = cache->chunkId;
					}
				}
			}
		
			if(cache)
			{
				//Write it out

#if 0
				nBytes = cache->object->variant.fileVariant.fileSize - ((cache->chunkId -1) * YAFFS_BYTES_PER_CHUNK);
			
				if(nBytes > YAFFS_BYTES_PER_CHUNK)
				{
					nBytes= YAFFS_BYTES_PER_CHUNK;
				}
#endif			
				chunkWritten = yaffs_WriteChunkDataToObject(cache->object,
															cache->chunkId,
															cache->data,
															cache->nBytes,1);

				cache->dirty = 0;
				cache->object = NULL;
			}
		
		} while(cache && chunkWritten > 0);
	
		if(cache)
		{
			//Hoosterman, disk full while writing cache out.
			T(YAFFS_TRACE_ERROR, (TSTR("yaffs tragedy: no space during cache write" TENDSTR)));

		}
	}	
		
}


// Grab us a chunk for use.
// First look for an empty one. 
// Then look for the least recently used non-dirty one.
// Then look for the least recently used dirty one...., flush and look again.
static yaffs_ChunkCache *yaffs_GrabChunkCacheWorker(yaffs_Device *dev)
{
	int i;
	int usage;
	int theOne;
	
	if(dev->nShortOpCaches > 0)
	{
		for(i = 0; i < dev->nShortOpCaches; i++)
		{
			if(!dev->srCache[i].object)
			{
				//T(("Grabbing empty %d\n",i));
				
				//printf("Grabbing empty %d\n",i);
			
				return &dev->srCache[i];
			}
		}
		
		return NULL;
	
		theOne = -1; 
		usage = 0; // just to stop the compiler grizzling
	
		for(i = 0; i < dev->nShortOpCaches; i++)
		{
			if(!dev->srCache[i].dirty &&
			((dev->srCache[i].lastUse < usage  && theOne >= 0)|| 
				theOne < 0))
			{
				usage = dev->srCache[i].lastUse;
				theOne = i;
			}
		}
	
		//T(("Grabbing non-empty %d\n",theOne));
		
		//if(theOne >= 0) printf("Grabbed non-empty cache %d\n",theOne);
		
		return  theOne >= 0 ?  &dev->srCache[theOne] : NULL;
	}
	else
	{
		return NULL;
	}
	
}


static yaffs_ChunkCache *yaffs_GrabChunkCache(yaffs_Device *dev)
{
	yaffs_ChunkCache *cache;
	yaffs_Object *theObj;
	int usage;
	int i;
	int pushout;
	
	if(dev->nShortOpCaches > 0)
	{
		// Try find a non-dirty one...
	
		cache = yaffs_GrabChunkCacheWorker(dev);
	
		if(!cache)
		{
			// They were all dirty, find the last recently used object and flush
			// its cache, then  find again.
			// NB what's here is not very accurate, we actually flush the object
			// the last recently used page.
		
			theObj = dev->srCache[0].object;
			usage = dev->srCache[0].lastUse;
			cache = &dev->srCache[0];
			pushout = 0;
	
			for(i = 1; i < dev->nShortOpCaches; i++)
			{
				if( dev->srCache[i].object && 
					dev->srCache[i].lastUse < usage)
				{
					usage  = dev->srCache[i].lastUse;
					theObj = dev->srCache[i].object;
					cache = &dev->srCache[i];
					pushout = i;
				}
			}
		
			if(!cache || cache->dirty)
			{
			
				//printf("Dirty ");
				yaffs_FlushFilesChunkCache(theObj);
		
				// Try again
				cache = yaffs_GrabChunkCacheWorker(dev);
			}
			else
			{
				//printf(" pushout %d\n",pushout);
			}
			
		}

		return cache;
	}
	else
		return NULL;

}


// Find a cached chunk
static yaffs_ChunkCache *yaffs_FindChunkCache(const yaffs_Object *obj, int chunkId)
{
	yaffs_Device *dev = obj->myDev;
	int i;
	if(dev->nShortOpCaches > 0)
	{
		for(i = 0; i < dev->nShortOpCaches; i++)
		{
			if(dev->srCache[i].object == obj && 
			dev->srCache[i].chunkId == chunkId)
			{
				dev->cacheHits++;
			
				return &dev->srCache[i];
			}
		}
	}
	return NULL;
}

// Mark the chunk for the least recently used algorithym
static void yaffs_UseChunkCache(yaffs_Device *dev, yaffs_ChunkCache *cache, int isAWrite)
{

	if(dev->nShortOpCaches > 0)
	{
		if( dev->srLastUse < 0 || 
			dev->srLastUse > 100000000)
		{
			// Reset the cache usages
			int i;
			for(i = 1; i < dev->nShortOpCaches; i++)
			{
				dev->srCache[i].lastUse = 0;
			}
			dev->srLastUse = 0;
		}

		dev->srLastUse++;
	
		cache->lastUse = dev->srLastUse;

		if(isAWrite)
		{
			cache->dirty = 1;
		}
	}
}

// Invalidate a single cache page.
// Do this when a whole page gets written,
// ie the short cache for this page is no longer valid.
static void yaffs_InvalidateChunkCache(yaffs_Object *object, int chunkId)
{
	if(object->myDev->nShortOpCaches > 0)
	{
		yaffs_ChunkCache *cache = yaffs_FindChunkCache(object,chunkId);

		if(cache)
		{
			cache->object = NULL;
		}
	}
}


// Invalidate all the cache pages associated with this object
// Do this whenever ther file is deleted or resized.
static void yaffs_InvalidateWholeChunkCache(yaffs_Object *inObj)
{
	int i;
	yaffs_Device *dev = inObj->myDev;
	
	if(dev->nShortOpCaches > 0)
	{ 
		// Now invalidate it.
		for(i = 0; i < dev->nShortOpCaches; i++)
		{
			if(dev->srCache[i].object == inObj)
			{
				dev->srCache[i].object = NULL;
			}
		}
	}
}





///////////////////////// File read/write ///////////////////////////////
// Read and write have very similar structures.
// In general the read/write has three parts to it
// * An incomplete chunk to start with (if the read/write is not chunk-aligned)
// * Some complete chunks
// * An incomplete chunk to end off with
//
// Curve-balls: the first chunk might also be the last chunk.

int yaffs_ReadDataFromFile(yaffs_Object *inObj, uint8_t * buffer, uint32_t offset, int nBytes)
{
	
	
	int chunk;
	int start;
	int nToCopy;
	int n = nBytes;
	int nDone = 0;
	yaffs_ChunkCache *cache;
	
	yaffs_Device *dev;
	
	dev = inObj->myDev;
	
	while(n > 0)
	{
		chunk = offset / YAFFS_BYTES_PER_CHUNK + 1; // The first chunk is 1
		start = offset % YAFFS_BYTES_PER_CHUNK;

		// OK now check for the curveball where the start and end are in
		// the same chunk.	
		if(	(start + n) < YAFFS_BYTES_PER_CHUNK)
		{
			nToCopy = n;
		}
		else
		{
			nToCopy = YAFFS_BYTES_PER_CHUNK - start;
		}
	
		cache = yaffs_FindChunkCache(inObj,chunk);
		
		// If the chunk is already in the cache or it is less than a whole chunk
		// then use the cache (if there is caching)
		// else bypass the cache.
		if( cache || nToCopy != YAFFS_BYTES_PER_CHUNK)
		{
			if(dev->nShortOpCaches > 0)
			{
				
				// If we can't find the data in the cache, then load it up.
				
				if(!cache)
				{
					cache = yaffs_GrabChunkCache(inObj->myDev);
					cache->object = inObj;
					cache->chunkId = chunk;
					cache->dirty = 0;
					yaffs_ReadChunkDataFromObject(inObj,chunk,cache->data);
					cache->nBytes = 0;	
				}
			
				yaffs_UseChunkCache(dev,cache,0);

				TEE_MemMove(buffer,&cache->data[start],nToCopy);
			}
			else
			{
				// Read into the local buffer then copy...
				yaffs_ReadChunkDataFromObject(inObj,chunk,dev->localBuffer);		
				TEE_MemMove(buffer,&dev->localBuffer[start],nToCopy);
			}

		}
		else
		{
#ifdef CONFIG_YAFFS_WINCE
			
			// Under WinCE can't do direct transfer. Need to use a local buffer.
			// This is because we otherwise screw up WinCE's memory mapper
			yaffs_ReadChunkDataFromObject(inObj,chunk,dev->localBuffer);
			TEE_MemMove(buffer,dev->localBuffer,YAFFS_BYTES_PER_CHUNK);
#else
			// A full chunk. Read directly into the supplied buffer.
			yaffs_ReadChunkDataFromObject(inObj,chunk,buffer);
#endif
		}
		
		n -= nToCopy;
		offset += nToCopy;
		buffer += nToCopy;
		nDone += nToCopy;
		
	}
	
	return nDone;
}



int yaffs_WriteDataToFile(yaffs_Object *inObj,const uint8_t * buffer, uint32_t offset, int nBytes)
{	
	
	int chunk;
	int start;
	int nToCopy;
	int n = nBytes;
	int nDone = 0;
	int nToWriteBack;
	int startOfWrite = offset;
	int chunkWritten = 0;
	int nBytesRead;
	
	yaffs_Device *dev;
	
	dev = inObj->myDev;
	
	
	while(n > 0 && chunkWritten >= 0)
	{
		chunk = offset / YAFFS_BYTES_PER_CHUNK + 1;
		start = offset % YAFFS_BYTES_PER_CHUNK;
		

		// OK now check for the curveball where the start and end are in
		// the same chunk.
		
		if(	(start + n) < YAFFS_BYTES_PER_CHUNK)
		{
			nToCopy = n;
			
			// Now folks, to calculate how many bytes to write back....
			// If we're overwriting and not writing to then end of file then
			// we need to write back as much as was there before.
			
			nBytesRead = inObj->variant.fileVariant.fileSize - ((chunk -1) * YAFFS_BYTES_PER_CHUNK);
			
			if(nBytesRead > YAFFS_BYTES_PER_CHUNK)
			{
				nBytesRead = YAFFS_BYTES_PER_CHUNK;
			}
			
			nToWriteBack = (nBytesRead > (start + n)) ? nBytesRead : (start +n);
			
		}
		else
		{
			nToCopy = YAFFS_BYTES_PER_CHUNK - start;
			nToWriteBack = YAFFS_BYTES_PER_CHUNK;
		}
	
		if(nToCopy != YAFFS_BYTES_PER_CHUNK)
		{
			// An incomplete start or end chunk (or maybe both start and end chunk)
			if(dev->nShortOpCaches > 0)
			{
				yaffs_ChunkCache *cache;
				// If we can't find the data in the cache, then load it up.
				cache = yaffs_FindChunkCache(inObj,chunk);
				if(!cache && yaffs_CheckSpaceForChunkCache(inObj->myDev))
				{
					cache = yaffs_GrabChunkCache(inObj->myDev);
					cache->object = inObj;
					cache->chunkId = chunk;
					cache->dirty = 0;
					yaffs_ReadChunkDataFromObject(inObj,chunk,cache->data);		
				}
			
				if(cache)
				{	
					yaffs_UseChunkCache(dev,cache,1);
					TEE_MemMove(&cache->data[start],buffer,nToCopy);
					cache->nBytes = nToWriteBack;
				}
				else
				{
					chunkWritten = -1; // fail the write
				}
			}
			else
			{
				// An incomplete start or end chunk (or maybe both start and end chunk)
				// Read into the local buffer then copy, then copy over and write back.
		
				yaffs_ReadChunkDataFromObject(inObj,chunk,dev->localBuffer);
				
				TEE_MemMove(&dev->localBuffer[start],buffer,nToCopy);
			
				chunkWritten = yaffs_WriteChunkDataToObject(inObj,chunk,dev->localBuffer,nToWriteBack,0);
			
				//T(("Write with readback to chunk %d %d  start %d  copied %d wrote back %d\n",chunk,chunkWritten,start, nToCopy, nToWriteBack));
			}
			
		}
		else
		{
			
#ifdef CONFIG_YAFFS_WINCE
			// Under WinCE can't do direct transfer. Need to use a local buffer.
			// This is because we otherwise screw up WinCE's memory mapper
			TEE_MemMove(dev->localBuffer,buffer,YAFFS_BYTES_PER_CHUNK);
			chunkWritten = yaffs_WriteChunkDataToObject(inObj,chunk,dev->localBuffer,YAFFS_BYTES_PER_CHUNK,0);
#else
			// A full chunk. Write directly from the supplied buffer.
			chunkWritten = yaffs_WriteChunkDataToObject(inObj,chunk,buffer,YAFFS_BYTES_PER_CHUNK,0);
#endif
			// Since we've overwritten the cached data, we better invalidate it.
			yaffs_InvalidateChunkCache(inObj,chunk);
			//T(("Write to chunk %d %d\n",chunk,chunkWritten));
		}
		
		if(chunkWritten >= 0)
		{
			n -= nToCopy;
			offset += nToCopy;
			buffer += nToCopy;
			nDone += nToCopy;
		}
		
	}
	
	// Update file object
	
	if((startOfWrite + nDone) > inObj->variant.fileVariant.fileSize)
	{
		inObj->variant.fileVariant.fileSize = (startOfWrite + nDone);
	}
	
	inObj->dirty = 1;
	
	return nDone;
}


int yaffs_ResizeFile(yaffs_Object *inObj, int newSize)
{
	int i;
	int chunkId;
	int oldFileSize = inObj->variant.fileVariant.fileSize;
	int sizeOfPartialChunk = newSize % YAFFS_BYTES_PER_CHUNK;
	
	yaffs_Device *dev = inObj->myDev;
	

	yaffs_FlushFilesChunkCache(inObj);	
	yaffs_InvalidateWholeChunkCache(inObj);
	
	if(inObj->variantType != YAFFS_OBJECT_TYPE_FILE)
	{
		return yaffs_GetFileSize(inObj);
	}
	
	if(newSize < oldFileSize)
	{
		
		int lastDel = 1 + (oldFileSize-1)/YAFFS_BYTES_PER_CHUNK;
		
		int startDel = 1 + (newSize + YAFFS_BYTES_PER_CHUNK - 1)/
							YAFFS_BYTES_PER_CHUNK;

		// Delete backwards so that we don't end up with holes if
		// power is lost part-way through the operation.
		for(i = lastDel; i >= startDel; i--)
		{
			// NB this could be optimised somewhat,
			// eg. could retrieve the tags and write them without
			// using yaffs_DeleteChunk

			chunkId = yaffs_FindAndDeleteChunkInFile(inObj,i,NULL);
			if(chunkId < (dev->internalStartBlock * 32) || chunkId >= ((dev->internalEndBlock+1) * 32))
			{
				//T(("Found daft chunkId %d for %d\n",chunkId,i));
			}
			else
			{
				inObj->nDataChunks--;
				yaffs_DeleteChunk(dev,chunkId,1);
			}
		}
		
		
		if(sizeOfPartialChunk != 0)
		{
			int lastChunk = 1+ newSize/YAFFS_BYTES_PER_CHUNK;
			
			// Got to read and rewrite the last chunk with its new size.
			// NB Got to zero pad to nuke old data
			yaffs_ReadChunkDataFromObject(inObj,lastChunk,dev->localBuffer);
			TEE_MemFill(dev->localBuffer + sizeOfPartialChunk,0, YAFFS_BYTES_PER_CHUNK - sizeOfPartialChunk);

			yaffs_WriteChunkDataToObject(inObj,lastChunk,dev->localBuffer,sizeOfPartialChunk,1);
				
		}
		
		inObj->variant.fileVariant.fileSize = newSize;
		
		yaffs_PruneFileStructure(dev,&inObj->variant.fileVariant);
		
		return newSize;
		
	}
	else
	{
		return oldFileSize;
	}
}


off_t yaffs_GetFileSize(yaffs_Object *obj)
{
	obj = yaffs_GetEquivalentObject(obj);
	
	switch(obj->variantType)
	{
		case YAFFS_OBJECT_TYPE_FILE: 
			return obj->variant.fileVariant.fileSize;
		case YAFFS_OBJECT_TYPE_SYMLINK:
			return TEE_StrLen(obj->variant.symLinkVariant.alias);
		default:
			return 0;
	}
}



// yaffs_FlushFile() updates the file's
// objectId in NAND

int yaffs_FlushFile(yaffs_Object *inObj, int updateTime)
{
	int retVal;
	if(inObj->dirty)
	{
		//T(("flushing object header\n"));
		
		yaffs_FlushFilesChunkCache(inObj);
		if(updateTime)
		{
#ifdef CONFIG_YAFFS_WINCE
			yfsd_WinFileTimeNow(inObj->win_mtime);
#else
			inObj->yst_mtime = Y_CURRENT_TIME;
#endif
		}

		retVal = (yaffs_UpdateObjectHeader(inObj,NULL,0) >= 0)? YAFFS_OK : YAFFS_FAIL;
	}
	else
	{
		retVal = YAFFS_OK;
	}
	
	return retVal;
	
}


static int yaffs_DoGenericObjectDeletion(yaffs_Object *inObj)
{

	// First off, invalidate the file's data in the cache, without flushing.
	yaffs_InvalidateWholeChunkCache(inObj);
	
	yaffs_RemoveObjectFromDirectory(inObj);
	yaffs_DeleteChunk(inObj->myDev,inObj->chunkId,1);
	inObj->chunkId = -1;
#if 0
#ifdef __KERNEL__
	if(inObj->myInode)
	{
		inObj->myInode->u.generic_ip = NULL;
		inObj->myInode = NULL;
	}
#endif
#endif
	yaffs_FreeObject(inObj);
	return YAFFS_OK;

}

// yaffs_DeleteFile deletes the whole file data
// and the inode associated with the file.
// It does not delete the links associated with the file.
static int yaffs_UnlinkFile(yaffs_Object *inObj)
{

#ifdef CONFIG_YAFFS_DISABLE_BACKGROUND_DELETION

	// Delete the file data & tnodes

	 yaffs_DeleteWorker(inObj, inObj->variant.fileVariant.top, inObj->variant.fileVariant.topLevel, 0,NULL);
	 

	yaffs_FreeTnode(inObj->myDev,inObj->variant.fileVariant.top);
	
	return  yaffs_DoGenericObjectDeletion(inObj);
#else
	int retVal;
	int immediateDeletion=0;
	retVal = yaffs_ChangeObjectName(inObj, inObj->myDev->unlinkedDir,NULL,0);
	if(1 || // Ignore the result of the change name. This will only fail
			// if the disk was completely full (an error condition)
			// We ignore the error so we can delete files to recover the disk
	   retVal == YAFFS_OK)
	{
		//inObj->unlinked = 1;
		//inObj->myDev->nUnlinkedFiles++;
		//inObj->renameAllowed = 0;
#ifdef __KERNEL__
		if(!inObj->myInode)
		{
			// Might be open at present,
			// Caught by delete_inode in yaffs_fs.c
			immediateDeletion = 1;

		}
#else
		if(inObj->inUse <= 0)
		{
			immediateDeletion = 1;

		}
#endif
		
		if(immediateDeletion)
		{
			T(YAFFS_TRACE_TRACING,(TSTR("yaffs: immediate deletion of file %d" TENDSTR),inObj->objectId));
			inObj->deleted=1;
			inObj->myDev->nDeletedFiles++;
			yaffs_SoftDeleteFile(inObj);
		}
	
	}
	return retVal;

	
#endif
}

int yaffs_DeleteFile(yaffs_Object *inObj)
{
	int retVal = YAFFS_OK;
	
	if(inObj->nDataChunks > 0)
	{
		// Use soft deletion
		if(!inObj->unlinked)
		{
			retVal = yaffs_UnlinkFile(inObj);
		}
		if(retVal == YAFFS_OK && 
		inObj->unlinked &&
		!inObj->deleted)
		{
			inObj->deleted = 1;
			inObj->myDev->nDeletedFiles++;
			yaffs_SoftDeleteFile(inObj);
		}
		return inObj->deleted ? YAFFS_OK : YAFFS_FAIL;	
	}
	else
	{
		// The file has no data chunks so we toss it immediately
		yaffs_FreeTnode(inObj->myDev,inObj->variant.fileVariant.top);
		inObj->variant.fileVariant.top = NULL;
		yaffs_DoGenericObjectDeletion(inObj);	
		
		return YAFFS_OK;	
	}
}

static int yaffs_DeleteDirectory(yaffs_Object *inObj)
{
	//First check that the directory is empty.
	if(fs_list_empty(&inObj->variant.directoryVariant.children))
	{
		return  yaffs_DoGenericObjectDeletion(inObj);
	}
	
	return YAFFS_FAIL;
	
}

static int yaffs_DeleteSymLink(yaffs_Object *inObj)
{
	YFREE(inObj->variant.symLinkVariant.alias);

	return  yaffs_DoGenericObjectDeletion(inObj);
}

static int yaffs_DeleteHardLink(yaffs_Object *inObj)
{
	// remove this hardlink from the list assocaited with the equivalent
	// object
	fs_list_del(&inObj->hardLinks);
	return  yaffs_DoGenericObjectDeletion(inObj);	
}


static void yaffs_AbortHalfCreatedObject(yaffs_Object *obj)
{
	switch(obj->variantType)
	{
		case YAFFS_OBJECT_TYPE_FILE: yaffs_DeleteFile(obj); break;
		case YAFFS_OBJECT_TYPE_DIRECTORY: yaffs_DeleteDirectory(obj); break;
		case YAFFS_OBJECT_TYPE_SYMLINK: yaffs_DeleteSymLink(obj); break;
		case YAFFS_OBJECT_TYPE_HARDLINK: yaffs_DeleteHardLink(obj); break;
		case YAFFS_OBJECT_TYPE_SPECIAL: yaffs_DoGenericObjectDeletion(obj); break;
		case YAFFS_OBJECT_TYPE_UNKNOWN: break; // should not happen.
	}
}


static int yaffs_UnlinkWorker(yaffs_Object *obj)
{

	
	if(obj->variantType == YAFFS_OBJECT_TYPE_HARDLINK)
	{
		return  yaffs_DeleteHardLink(obj);
	}
	else if(!fs_list_empty(&obj->hardLinks))
	{	
		// Curve ball: We're unlinking an object that has a hardlink.
		//
		//	This problem arises because we are not strictly following
		//  The Linux link/inode model.
		//
		// We can't really delete the object.
		// Instead, we do the following:
		// - Select a hardlink.
		// - Unhook it from the hard links
		// - Unhook it from its parent directory (so that the rename can work)
		// - Rename the object to the hardlink's name.
		// - Delete the hardlink
		
		
		yaffs_Object *hl;
		int retVal;
		char name[YAFFS_MAX_NAME_LENGTH+1];
		
		hl = fs_list_entry(obj->hardLinks.next,yaffs_Object,hardLinks);
		
		fs_list_del_init(&hl->hardLinks);
		fs_list_del_init(&hl->siblings);
		
		yaffs_GetObjectName(hl,name,YAFFS_MAX_NAME_LENGTH+1);
		
		retVal = yaffs_ChangeObjectName(obj, hl->parent, name,0);
		
		if(retVal == YAFFS_OK)
		{
			retVal = yaffs_DoGenericObjectDeletion(hl);
		}
		return retVal;
				
	}
	else
	{
		switch(obj->variantType)
		{
			case YAFFS_OBJECT_TYPE_FILE:
				return yaffs_UnlinkFile(obj);
				break;
			case YAFFS_OBJECT_TYPE_DIRECTORY:
				return yaffs_DeleteDirectory(obj);
				break;
			case YAFFS_OBJECT_TYPE_SYMLINK:
				return yaffs_DeleteSymLink(obj);
				break;
			case YAFFS_OBJECT_TYPE_SPECIAL:
				return yaffs_DoGenericObjectDeletion(obj);
				break;
			case YAFFS_OBJECT_TYPE_HARDLINK:
			case YAFFS_OBJECT_TYPE_UNKNOWN:
			default:
				return YAFFS_FAIL;
		}
	}
}

int yaffs_Unlink(yaffs_Object *dir, const char *name)
{
	yaffs_Object *obj;
	
	 obj = yaffs_FindObjectByName(dir,name);
	 
	 if(obj && obj->unlinkAllowed)
	 {
	 	return yaffs_UnlinkWorker(obj);
	 }
	 
	 return YAFFS_FAIL;
	
}

//////////////// Initialisation Scanning /////////////////



// For now we use the SmartMedia check.
// We look at the blockStatus byte in the first two chunks
// These must be 0xFF to pass as OK.
// todo: this function needs to be modifyable foir different NAND types
// and different chunk sizes.  Suggest make this into a per-device configurable
// function.
static int yaffs_IsBlockBad(yaffs_Device *dev, int blk)
{
	static yaffs_Spare spare;
	
	yaffs_ReadChunkFromNAND(dev,blk * dev->nChunksPerBlock,NULL,&spare,1);
#if 1
	if(yaffs_CountBits(spare.blockStatus) < 7)
	{
		return 1;
	}
#else
	if(spare.blockStatus != 0xFF)
	{
		return 1;
	}
#endif
	yaffs_ReadChunkFromNAND(dev,blk * dev->nChunksPerBlock + 1,NULL,&spare,1);

#if 1
	if(yaffs_CountBits(spare.blockStatus) < 7)
	{
		return 1;
	}
#else
	if(spare.blockStatus != 0xFF)
	{
		return 1;
	}
#endif
	
	return 0;
	
}

static int yaffs_Scan(yaffs_Device *dev)
{
	yaffs_Spare spare;
	yaffs_Tags tags;
	int blk;
	int chunk;
	int c;
	int deleted;
	yaffs_BlockState state;
	yaffs_Object *hardList = NULL;
	yaffs_Object *hl;
	yaffs_BlockInfo *bi;

	
	yaffs_ObjectHeader *oh;
	yaffs_Object *inObj;
	yaffs_Object *parent;
	
	uint8_t chunkData[YAFFS_BYTES_PER_CHUNK];
	
	
	// Scan all the blocks...
	
	for(blk = dev->internalStartBlock; blk <= dev->internalEndBlock; blk++)
	{
		deleted = 0;
		bi = yaffs_GetBlockInfo(dev,blk);
		yaffs_ClearChunkBits(dev,blk);
		bi->pagesInUse = 0;
		bi->softDeletions = 0;
		state = YAFFS_BLOCK_STATE_SCANNING;
		
		
		if(yaffs_IsBlockBad(dev,blk))
		{
			state = YAFFS_BLOCK_STATE_DEAD;
			T(YAFFS_TRACE_BAD_BLOCKS,(TSTR("block %d is bad" TENDSTR),blk));
		}
		
		// Read each chunk in the block.
		
		for(c = 0; c < dev->nChunksPerBlock && 
				   state == YAFFS_BLOCK_STATE_SCANNING; c++)
		{
			// Read the spare area and decide what to do
			chunk = blk * dev->nChunksPerBlock + c;
			
			yaffs_ReadChunkFromNAND(dev,chunk,NULL,&spare,1);

			
			// This block looks ok, now what's in this chunk?
			yaffs_GetTagsFromSpare(dev,&spare,&tags);
			
			if(yaffs_CountBits(spare.pageStatus) < 6)
			{
				// A deleted chunk
				deleted++;
				dev->nFreeChunks ++;
				//T((" %d %d deleted\n",blk,c));
			}
			else if(tags.objectId == YAFFS_UNUSED_OBJECT_ID)
			{
				// An unassigned chunk in the block
				// This means that either the block is empty or 
				// this is the one being allocated from
				
				if(c == 0)
				{
					// the block is unused
					state = YAFFS_BLOCK_STATE_EMPTY;
					dev->nErasedBlocks++;
				}
				else
				{
					// this is the block being allocated from
					T(YAFFS_TRACE_SCAN,(TSTR(" Allocating from %d %d" TENDSTR),blk,c));
					state = YAFFS_BLOCK_STATE_ALLOCATING;
					dev->allocationBlock = blk;
					dev->allocationPage = c;
				}

				dev->nFreeChunks += (dev->nChunksPerBlock - c);
			}
			else if(tags.chunkId > 0)
			{
				int endpos;
				// A data chunk.
				yaffs_SetChunkBit(dev,blk,c);
				bi->pagesInUse++;
								
				inObj = yaffs_FindOrCreateObjectByNumber(dev,tags.objectId,YAFFS_OBJECT_TYPE_FILE);
				// PutChunkIntoFIle checks for a clash (two data chunks with
				// the same chunkId).
				yaffs_PutChunkIntoFile(inObj,tags.chunkId,chunk,1);
				endpos = (tags.chunkId - 1)* YAFFS_BYTES_PER_CHUNK + tags.byteCount;
				if(inObj->variant.fileVariant.scannedFileSize <endpos)
				{
					inObj->variant.fileVariant.scannedFileSize = endpos;
#ifndef CONFIG_YAFFS_USE_HEADER_FILE_SIZE
						inObj->variant.fileVariant.fileSize = 	
							inObj->variant.fileVariant.scannedFileSize;
#endif

				}
				//T((" %d %d data %d %d\n",blk,c,tags.objectId,tags.chunkId));	
			}
			else
			{
				// chunkId == 0, so it is an ObjectHeader.
				// Thus, we read in the object header and make the object
				yaffs_SetChunkBit(dev,blk,c);
				bi->pagesInUse++;
							
				yaffs_ReadChunkFromNAND(dev,chunk,chunkData,NULL,1);
				
				oh = (yaffs_ObjectHeader *)chunkData;
				
				inObj = yaffs_FindOrCreateObjectByNumber(dev,tags.objectId,oh->type);
				
				if(inObj->valid)
				{
					// We have already filled this one. We have a duplicate and need to resolve it.
					
					unsigned existingSerial = inObj->serial;
					unsigned newSerial = tags.serialNumber;
					
					if(((existingSerial+1) & 3) == newSerial)
					{
						// Use new one - destroy the exisiting one
						yaffs_DeleteChunk(dev,inObj->chunkId,1);
						inObj->valid = 0;
					}
					else
					{
						// Use existing - destroy this one.
						yaffs_DeleteChunk(dev,chunk,1);
					}
				}
				
				if(!inObj->valid &&
				   (tags.objectId == YAFFS_OBJECTID_ROOT ||
				    tags.objectId == YAFFS_OBJECTID_LOSTNFOUND))
				{
					// We only load some info, don't fiddle with directory structure
					inObj->valid = 1;
					inObj->variantType = oh->type;
	
					inObj->yst_mode  = oh->yst_mode;
#ifdef CONFIG_YAFFS_WINCE
					inObj->win_atime[0] = oh->win_atime[0];
					inObj->win_ctime[0] = oh->win_ctime[0];
					inObj->win_mtime[0] = oh->win_mtime[0];
					inObj->win_atime[1] = oh->win_atime[1];
					inObj->win_ctime[1] = oh->win_ctime[1];
					inObj->win_mtime[1] = oh->win_mtime[1];
#else
					inObj->yst_uid   = oh->yst_uid;
					inObj->yst_gid   = oh->yst_gid;
					inObj->yst_atime = oh->yst_atime;
					inObj->yst_mtime = oh->yst_mtime;
					inObj->yst_ctime = oh->yst_ctime;
					inObj->yst_rdev = oh->yst_rdev;
#endif
					inObj->chunkId  = chunk;

				}
				else if(!inObj->valid)
				{
					// we need to load this info
				
					inObj->valid = 1;
					inObj->variantType = oh->type;
	
					inObj->yst_mode  = oh->yst_mode;
#ifdef CONFIG_YAFFS_WINCE
					inObj->win_atime[0] = oh->win_atime[0];
					inObj->win_ctime[0] = oh->win_ctime[0];
					inObj->win_mtime[0] = oh->win_mtime[0];
					inObj->win_atime[1] = oh->win_atime[1];
					inObj->win_ctime[1] = oh->win_ctime[1];
					inObj->win_mtime[1] = oh->win_mtime[1];
#else
					inObj->yst_uid   = oh->yst_uid;
					inObj->yst_gid   = oh->yst_gid;
					inObj->yst_atime = oh->yst_atime;
					inObj->yst_mtime = oh->yst_mtime;
					inObj->yst_ctime = oh->yst_ctime;
					inObj->yst_rdev = oh->yst_rdev;
#endif
					inObj->chunkId  = chunk;

					yaffs_SetObjectName(inObj,oh->name);
					inObj->dirty = 0;
							
					// directory stuff...
					// hook up to parent
	
					parent = yaffs_FindOrCreateObjectByNumber(dev,oh->parentObjectId,YAFFS_OBJECT_TYPE_DIRECTORY);
					if(parent->variantType == YAFFS_OBJECT_TYPE_UNKNOWN)
					{
						// Set up as a directory
						parent->variantType = YAFFS_OBJECT_TYPE_DIRECTORY;
						FS_INIT_LIST_HEAD(&parent->variant.directoryVariant.children);
					}
					else if(parent->variantType != YAFFS_OBJECT_TYPE_DIRECTORY)
					{
						// Hoosterman, another problem....
						// We're trying to use a non-directory as a directory
						
						T(YAFFS_TRACE_ERROR, (TSTR("yaffs tragedy: attempting to use non-directory as a directory in scan. Put in lost+found." TENDSTR)));
						parent = dev->lostNFoundDir;

					}
				
					yaffs_AddObjectToDirectory(parent,inObj);
					if(parent == dev->unlinkedDir)
					{
						inObj->deleted = 1; // If it is unlinked at start up then it wants deleting
						dev->nDeletedFiles++;
					}
				
					// Note re hardlinks.
					// Since we might scan a hardlink before its equivalent object is scanned
					// we put them all in a list.
					// After scanning is complete, we should have all the objects, so we run through this
					// list and fix up all the chains.		
	
					switch(inObj->variantType)
					{
						case YAFFS_OBJECT_TYPE_UNKNOWN: 	// Todo got a problem
							break;
						case YAFFS_OBJECT_TYPE_FILE:
#ifdef CONFIG_YAFFS_USE_HEADER_FILE_SIZE
							inObj->variant.fileVariant.fileSize = oh->fileSize;
#endif
							break;
						case YAFFS_OBJECT_TYPE_HARDLINK:
							inObj->variant.hardLinkVariant.equivalentObjectId = oh->equivalentObjectId;
							inObj->hardLinks.next = (struct fs_list_head *)hardList;
							hardList = inObj;
							break;
						case YAFFS_OBJECT_TYPE_DIRECTORY:	// Do nothing
							break;
						case YAFFS_OBJECT_TYPE_SPECIAL:	// Do nothing
							break;
						case YAFFS_OBJECT_TYPE_SYMLINK: 	// Do nothing
							inObj->variant.symLinkVariant.alias = yaffs_CloneString(oh->alias);
							break;
					}
					//T((" %d %d header %d \"%s\" type %d\n",blk,c,tags.objectId,oh->name,inObj->variantType));	
				}
			}
		}
		
		if(state == YAFFS_BLOCK_STATE_SCANNING)
		{
			// If we got this far while scanning, then the block is fully allocated.
			state = YAFFS_BLOCK_STATE_FULL;	
		}
		
		bi->blockState = state;
		
		// Now let's see if it was dirty
		if(	bi->pagesInUse == 0 &&
	        bi->blockState == YAFFS_BLOCK_STATE_FULL)
	    {
	    	yaffs_BlockBecameDirty(dev,blk);
	    }

	}
	
	// Fix up the hard link chains.
	// We should now have scanned all the objects, now it's time to add these 
	// hardlinks.
	while(hardList)
	{
		hl = hardList;
		hardList = (yaffs_Object *)(hardList->hardLinks.next);
		
		inObj = yaffs_FindObjectByNumber(dev,hl->variant.hardLinkVariant.equivalentObjectId);
		
		if(inObj)
		{
			// Add the hardlink pointers
			hl->variant.hardLinkVariant.equivalentObject=inObj;
			fs_list_add(&hl->hardLinks,&inObj->hardLinks);
		}
		else
		{
			//Todo Need to report/handle this better.
			// Got a problem... hardlink to a non-existant object
			hl->variant.hardLinkVariant.equivalentObject=NULL;
			FS_INIT_LIST_HEAD(&hl->hardLinks);
			
		}
		
	}
	
	{
		struct fs_list_head *i;	
		struct fs_list_head *n;
			
		yaffs_Object *l;
		// Soft delete all the unlinked files
		fs_list_for_each_safe(i,n,&dev->unlinkedDir->variant.directoryVariant.children)
		{
			if(i)
			{
				l = fs_list_entry(i, yaffs_Object,siblings);
				if(l->deleted)
				{
					yaffs_SoftDeleteFile(l);		
				}
			}
		}	
	}
	
	
	return YAFFS_OK;
}


////////////////////////// Directory Functions /////////////////////////


static void yaffs_AddObjectToDirectory(yaffs_Object *directory, yaffs_Object *obj)
{

	if(obj->siblings.prev == NULL)
	{
		// Not initialised
		FS_INIT_LIST_HEAD(&obj->siblings);
		
	}
	else if(!fs_list_empty(&obj->siblings))
	{
		// If it is holed up somewhere else, un hook it
		fs_list_del_init(&obj->siblings);
	}
	// Now add it
	fs_list_add(&obj->siblings,&directory->variant.directoryVariant.children);
	obj->parent = directory;
	
	if(directory == obj->myDev->unlinkedDir)
	{
		obj->unlinked = 1;
		obj->myDev->nUnlinkedFiles++;
		obj->renameAllowed = 0;
	}
}

static void yaffs_RemoveObjectFromDirectory(yaffs_Object *obj)
{
	fs_list_del_init(&obj->siblings);
	obj->parent = NULL;
}

yaffs_Object *yaffs_FindObjectByName(yaffs_Object *directory,const char *name)
{
	int sum;
	
	struct fs_list_head *i;
	char buffer[YAFFS_MAX_NAME_LENGTH+1];
	
	yaffs_Object *l;
	
	sum = yaffs_CalcNameSum(name);
	
	fs_list_for_each(i,&directory->variant.directoryVariant.children)
	{
		if(i)
		{
			l = fs_list_entry(i, yaffs_Object,siblings);
		
			// Special case for lost-n-found
			if(l->objectId == YAFFS_OBJECTID_LOSTNFOUND)
			{
				if(yaffs_strcmp(name,YAFFS_LOSTNFOUND_NAME) == 0)
				{
					return l;
				}
			}
			else if(yaffs_SumCompare(l->sum, sum))
			{
				// Do a real check
				yaffs_GetObjectName(l,buffer,YAFFS_MAX_NAME_LENGTH);
				if(yaffs_strcmp(name,buffer) == 0)
				{
					return l;
				}
			
			}			
		}
	}
	
	return NULL;
}


int yaffs_ApplyToDirectoryChildren(yaffs_Object *theDir,int (*fn)(yaffs_Object *))
{
	struct fs_list_head *i;	
	yaffs_Object *l;
	
	
	fs_list_for_each(i,&theDir->variant.directoryVariant.children)
	{
		if(i)
		{
			l = fs_list_entry(i, yaffs_Object,siblings);
			if(l && !fn(l))
			{
				return YAFFS_FAIL;
			}
		}
	}
	
	return YAFFS_OK;

}


// GetEquivalentObject dereferences any hard links to get to the
// actual object.

yaffs_Object *yaffs_GetEquivalentObject(yaffs_Object *obj)
{
	if(obj && obj->variantType == YAFFS_OBJECT_TYPE_HARDLINK)
	{
		// We want the object id of the equivalent object, not this one
		obj = obj->variant.hardLinkVariant.equivalentObject;
	}
	return obj;

}

int yaffs_GetObjectName(yaffs_Object *obj,char *name,int buffSize)
{
	TEE_MemFill(name,0,buffSize);
	
	if(obj->objectId == YAFFS_OBJECTID_LOSTNFOUND)
	{
		TEE_StrnCpy(name,YAFFS_LOSTNFOUND_NAME,buffSize - 1);
	}
	else if(obj->chunkId <= 0)
	{
		char locName[20];
		// make up a name
		sw_sprintf(locName,"%s%d",YAFFS_LOSTNFOUND_PREFIX,obj->objectId);
		TEE_StrnCpy(name,locName,buffSize - 1);

	}
#ifdef CONFIG_YAFFS_SHORT_NAMES_IN_RAM
	else if(obj->shortName[0])
	{
		sw_strcpy(name,obj->shortName);
	}
#endif
	else
	{
		uint8_t buffer[YAFFS_BYTES_PER_CHUNK];
		yaffs_ObjectHeader *oh = (yaffs_ObjectHeader *)buffer;

		TEE_MemFill(buffer,0,YAFFS_BYTES_PER_CHUNK);
	
		if(obj->chunkId >= 0)
		{
			yaffs_ReadChunkFromNAND(obj->myDev,obj->chunkId,buffer,NULL,1);
		}
		TEE_StrnCpy(name,oh->name,buffSize - 1);
	}
	
	return TEE_StrLen(name);
}

int yaffs_GetObjectFileLength(yaffs_Object *obj)
{
	
	// Dereference any hard linking
	obj = yaffs_GetEquivalentObject(obj);
	
	if(obj->variantType == YAFFS_OBJECT_TYPE_FILE)
	{
		return obj->variant.fileVariant.fileSize;
	}
	if(obj->variantType == YAFFS_OBJECT_TYPE_SYMLINK)
	{
		return TEE_StrLen(obj->variant.symLinkVariant.alias);
	}
	else
	{
		// Only a directory should drop through to here
		return YAFFS_BYTES_PER_CHUNK;
	}	
}

int yaffs_GetObjectLinkCount(yaffs_Object *obj)
{
	int count = 0; 
	struct fs_list_head *i;
	
	if(!obj->unlinked)
	{
		count++;	// the object itself
	}
	fs_list_for_each(i,&obj->hardLinks)
	{
		count++;	// add the hard links;
	}
	return count;
	
}


int yaffs_GetObjectInode(yaffs_Object *obj)
{
	obj = yaffs_GetEquivalentObject(obj);
	
	return obj->objectId;
}

unsigned yaffs_GetObjectType(yaffs_Object *obj)
{
	obj = yaffs_GetEquivalentObject(obj);
	
	switch(obj->variantType)
	{
		case YAFFS_OBJECT_TYPE_FILE:		return DT_REG; break;
		case YAFFS_OBJECT_TYPE_DIRECTORY:	return DT_DIR; break;
		case YAFFS_OBJECT_TYPE_SYMLINK:		return DT_LNK; break;
		case YAFFS_OBJECT_TYPE_HARDLINK:	return DT_REG; break;
		case YAFFS_OBJECT_TYPE_SPECIAL:		
			if(S_ISFIFO(obj->yst_mode)) return DT_FIFO;
			if(S_ISCHR(obj->yst_mode)) return DT_CHR;
			if(S_ISBLK(obj->yst_mode)) return DT_BLK;
			if(S_ISSOCK(obj->yst_mode)) return DT_SOCK;
		default: return DT_REG; break;
	}
}

char *yaffs_GetSymlinkAlias(yaffs_Object *obj)
{
	obj = yaffs_GetEquivalentObject(obj);
	if(obj->variantType == YAFFS_OBJECT_TYPE_SYMLINK)
	{
		return yaffs_CloneString(obj->variant.symLinkVariant.alias);
	}
	else
	{
		return yaffs_CloneString("");
	}
}

#ifndef CONFIG_YAFFS_WINCE

int yaffs_SetAttributes(yaffs_Object *obj, struct iattr *attr)
{
	unsigned int valid = attr->ia_valid;
	
	if(valid & ATTR_MODE) obj->yst_mode = attr->ia_mode;
	if(valid & ATTR_UID) obj->yst_uid = attr->ia_uid;
	if(valid & ATTR_GID) obj->yst_gid = attr->ia_gid;
	
	if(valid & ATTR_ATIME) obj->yst_atime = Y_TIME_CONVERT(attr->ia_atime);
	if(valid & ATTR_CTIME) obj->yst_ctime = Y_TIME_CONVERT(attr->ia_ctime);
	if(valid & ATTR_MTIME) obj->yst_mtime = Y_TIME_CONVERT(attr->ia_mtime);

	
	if(valid & ATTR_SIZE) yaffs_ResizeFile(obj,attr->ia_size);
	
	yaffs_UpdateObjectHeader(obj,NULL,1);
	
	return YAFFS_OK;
	
}

int yaffs_GetAttributes(yaffs_Object *obj, struct iattr *attr)
{
	unsigned int valid = 0;
	
	attr->ia_mode = obj->yst_mode;	valid |= ATTR_MODE;
	attr->ia_uid = obj->yst_uid;		valid |= ATTR_UID;
	attr->ia_gid = obj->yst_gid;		valid |= ATTR_GID;
	
	
	Y_TIME_CONVERT(attr->ia_atime) = obj->yst_atime;	valid |= ATTR_ATIME;
	Y_TIME_CONVERT(attr->ia_ctime) = obj->yst_ctime;	valid |= ATTR_CTIME;
	Y_TIME_CONVERT(attr->ia_mtime) = obj->yst_mtime;	valid |= ATTR_MTIME;

	attr->ia_size = yaffs_GetFileSize(obj); valid |= ATTR_SIZE;
	
	attr->ia_valid = valid;
	
	return YAFFS_OK;
	
}

#endif

int yaffs_DumpObject(yaffs_Object *obj)
{
//	uint8_t buffer[YAFFS_BYTES_PER_CHUNK];
	char name[257];
//	yaffs_ObjectHeader *oh = (yaffs_ObjectHeader *)buffer;

//	memset(buffer,0,YAFFS_BYTES_PER_CHUNK);
	
//	if(obj->chunkId >= 0)
//	{
//		yaffs_ReadChunkFromNAND(obj->myDev,obj->chunkId,buffer,NULL);
//	}
	
	yaffs_GetObjectName(obj,name,256);
	
	T(YAFFS_TRACE_ALWAYS,(TSTR("Object %d, inode %d \"%s\"\n dirty %d valid %d serial %d sum %d chunk %d type %d size %d\n" TENDSTR),
			obj->objectId,yaffs_GetObjectInode(obj), name, obj->dirty, obj->valid, obj->serial, 
			obj->sum, obj->chunkId, yaffs_GetObjectType(obj), yaffs_GetObjectFileLength(obj)));

#if 0
	YPRINTF(("Object %d \"%s\"\n dirty %d valid %d serial %d sum %d chunk %d\n",
			obj->objectId, oh->name, obj->dirty, obj->valid, obj->serial, 
			obj->sum, obj->chunkId));
		switch(obj->variantType)
	{
		case YAFFS_OBJECT_TYPE_FILE: 
			YPRINTF((" FILE length %d\n",obj->variant.fileVariant.fileSize));
			break;
		case YAFFS_OBJECT_TYPE_DIRECTORY:
			YPRINTF((" DIRECTORY\n"));
			break;
		case YAFFS_OBJECT_TYPE_HARDLINK: //todo
		case YAFFS_OBJECT_TYPE_SYMLINK:
		case YAFFS_OBJECT_TYPE_UNKNOWN:
		default:
	}
#endif
	
	return YAFFS_OK;
}


///////////////////////// Initialisation code ///////////////////////////



int yaffs_GutsInitialise(yaffs_Device *dev)
{
	unsigned x;
	int bits;
	int extraBits;
	int nBlocks;

	if(	dev->nBytesPerChunk != YAFFS_BYTES_PER_CHUNK ||	
		dev->nChunksPerBlock < 2 ||
		dev->nReservedBlocks < 2 ||
		dev->startBlock < 0 ||
		dev->endBlock <= 0 ||
		dev->endBlock <= (dev->startBlock + dev->nReservedBlocks)
	  )
	{
		//these parameters must be set before stating yaffs
		// Other parameters internalStartBlock,
		return YAFFS_FAIL;
	}


	
	if(!yaffs_CheckStructures())
	{
		T(YAFFS_TRACE_ALWAYS,(TSTR("yaffs_CheckStructures failed\n" TENDSTR)));
		return YAFFS_FAIL;
	}

	if(dev->isMounted)
	{
		T(YAFFS_TRACE_ALWAYS,(TSTR("yaffs: device already mounted\n" TENDSTR)));
		return YAFFS_FAIL;
	}

	dev->isMounted = 1;

	if(dev->startBlock < 0 ||
	   (dev->endBlock - dev->startBlock) < 10)
	{
		T(YAFFS_TRACE_ALWAYS,(TSTR("startBlock %d or endBlock %d invalid\n" TENDSTR),
				dev->startBlock, dev->endBlock));
		return YAFFS_FAIL;
	}
	
	
	// Do we need to add an offset to use block 0?
	
	dev->internalStartBlock = dev->startBlock;
	dev->internalEndBlock = dev->endBlock;
	dev->blockOffset = 0;
	dev->chunkOffset = 0;
	
	if(dev->startBlock == 0)
	{
		dev->internalStartBlock++;
		dev->internalEndBlock++;
		dev->blockOffset++;
		dev->chunkOffset = dev->nChunksPerBlock;		
	}
	
	
	nBlocks = dev->internalEndBlock - dev->internalStartBlock + 1;
	

		
	// OK now calculate a few things for the device
	// Calculate chunkGroupBits. 
	// We need to find the next power of 2 > than internalEndBlock
	
	x = dev->nChunksPerBlock * (dev->internalEndBlock+1);
	
	for(bits = extraBits = 0; x > 1; bits++)
	{
		if(x & 1) extraBits++;
		x >>= 1;
	}

	if(extraBits > 0) bits++;
	
	
	// Level0 Tnodes are 16 bits, so if the bitwidth of the
	// chunk range we're using is greater than 16 we need 
	// to figure out chunk shift and chunkGroupSize
	if(bits <= 16) 
	{
		dev->chunkGroupBits = 0;
	}
	else
	{
		dev->chunkGroupBits = bits - 16;
	}
	
	dev->chunkGroupSize = 1 << dev->chunkGroupBits;

	if(dev->nChunksPerBlock < dev->chunkGroupSize)
	{
		// We have a problem because the soft delete won't work if
		// the chunk group size > chunks per block.
		// This can be remedied by using larger "virtual blocks".
		
		return YAFFS_FAIL;
	}

	
	
	
	// More device initialisation
	dev->garbageCollections = 0;
	dev->passiveGarbageCollections = 0;
	dev->currentDirtyChecker = 0;
	dev->bufferedBlock = -1;
	dev->doingBufferedBlockRewrite = 0;
	dev->nDeletedFiles = 0;
	dev->nBackgroundDeletions=0;
	dev->nUnlinkedFiles = 0;
	dev->eccFixed=0;
	dev->eccUnfixed=0;
	dev->tagsEccFixed=0;
	dev->tagsEccUnfixed=0;
	dev->nErasedBlocks=0;
	
	dev->localBuffer = YMALLOC(dev->nBytesPerChunk);
	

	
	
	yaffs_InitialiseBlocks(dev,nBlocks);
	
	yaffs_InitialiseTnodes(dev);

	yaffs_InitialiseObjects(dev);
	
	if(dev->nShortOpCaches > 0)
	{ 
		int i;
		
		if(dev->nShortOpCaches >  YAFFS_MAX_SHORT_OP_CACHES)
		{
			dev->nShortOpCaches = YAFFS_MAX_SHORT_OP_CACHES;
		}
		
		dev->srCache = YMALLOC( dev->nShortOpCaches * sizeof(yaffs_ChunkCache));
		
		for(i=0; i < dev->nShortOpCaches; i++)
		{
			dev->srCache[i].object = NULL;
			dev->srCache[i].lastUse = 0;
			dev->srCache[i].dirty = 0;
		}
		dev->srLastUse = 0;
	}

	dev->cacheHits = 0;
	
	
	// Initialise the unlinked, root and lost and found directories
	dev->lostNFoundDir = dev->rootDir = dev->unlinkedDir = NULL;
	
	dev->unlinkedDir = yaffs_CreateFakeDirectory(dev,YAFFS_OBJECTID_UNLINKED, S_IFDIR);

	dev->rootDir = yaffs_CreateFakeDirectory(dev,YAFFS_OBJECTID_ROOT,YAFFS_ROOT_MODE | S_IFDIR);
	dev->lostNFoundDir = yaffs_CreateFakeDirectory(dev,YAFFS_OBJECTID_LOSTNFOUND,YAFFS_LOSTNFOUND_MODE | S_IFDIR);
	yaffs_AddObjectToDirectory(dev->rootDir,dev->lostNFoundDir);
	
		
	// Now scan the flash.	
	yaffs_Scan(dev);
	
	// Zero out stats
	dev->nPageReads = 0;
    dev->nPageWrites =  0;
	dev->nBlockErasures = 0;
	dev->nGCCopies = 0;
	dev->nRetriedWrites = 0;
	dev->nRetiredBlocks = 0;

	
	return YAFFS_OK;
		
}

void yaffs_Deinitialise(yaffs_Device *dev)
{
	if(dev->isMounted)
	{
	
		yaffs_DeinitialiseBlocks(dev);
		yaffs_DeinitialiseTnodes(dev);
		yaffs_DeinitialiseObjects(dev);
		if(dev->nShortOpCaches > 0)
			YFREE(dev->srCache);
		YFREE(dev->localBuffer);
		dev->isMounted = 0;
	}
	
}

#if 0

int  yaffs_GetNumberOfFreeChunks(yaffs_Device *dev)
{
	int nFree = dev->nFreeChunks - (dev->nChunksPerBlock * YAFFS_RESERVED_BLOCKS);
	
	struct fs_list_head *i;	
	yaffs_Object *l;
	
	
	// To the free chunks add the chunks that are in the deleted unlinked files.
	fs_list_for_each(i,&dev->unlinkedDir->variant.directoryVariant.children)
	{
		l = fs_list_entry(i, yaffs_Object,siblings);
		if(l->deleted)
		{
			nFree++;
			nFree += l->nDataChunks;
		}
	}
	
	
	// printf("___________ nFreeChunks is %d nFree is %d\n",dev->nFreeChunks,nFree);	

	if(nFree < 0) nFree = 0;

	return nFree;	
	
}

#endif

int  yaffs_GetNumberOfFreeChunks(yaffs_Device *dev)
{
	int nFree;
	int pending;
	int b;
	int nDirtyCacheChunks=0;
	
	yaffs_BlockInfo *blk;
	
	struct fs_list_head *i;	
	yaffs_Object *l;
	
	for(nFree = 0, b = dev->internalStartBlock; b <= dev->internalEndBlock; b++)
	{
		blk = yaffs_GetBlockInfo(dev,b);
		
		switch(blk->blockState)
		{
			case YAFFS_BLOCK_STATE_EMPTY:
			case YAFFS_BLOCK_STATE_ALLOCATING: 
			case YAFFS_BLOCK_STATE_FULL: nFree += (dev->nChunksPerBlock - blk->pagesInUse); break;
			default: break;
		}
	}
	
	
	pending = 0;
	
	// To the free chunks add the chunks that are in the deleted unlinked files.
	fs_list_for_each(i,&dev->unlinkedDir->variant.directoryVariant.children)
	{
		if(i)
		{
			l = fs_list_entry(i, yaffs_Object,siblings);
			if(l->deleted)
			{
				pending++;
				pending += l->nDataChunks;
			}
		}
	}
	
	
	
	//printf("___________ really free is %d, pending %d, nFree is %d\n",nFree,pending, nFree+pending);
	
	if(nFree != dev->nFreeChunks) 
	{
	//	printf("___________Different! really free is %d, nFreeChunks %d\n",nFree dev->nFreeChunks);
	}

	nFree += pending;
	
	// Now count the number of dirty chunks in the cache and subtract those
	
	{
		int i;
		for(i = 0; i < dev->nShortOpCaches; i++)
		{
			if(dev->srCache[i].dirty) nDirtyCacheChunks++;
		}
	}
	
	nFree -= nDirtyCacheChunks;
	
	nFree -= ((dev->nReservedBlocks + 1) * dev->nChunksPerBlock);
	
	if(nFree < 0) nFree = 0;

	return nFree;	
	
}



/////////////////// YAFFS test code //////////////////////////////////

#define yaffs_CheckStruct(structure,syze, name) \
           if(sizeof(structure) != syze) \
	       { \
	         T(YAFFS_TRACE_ALWAYS,(TSTR("%s should be %d but is %d\n" TENDSTR),name,syze,sizeof(structure))); \
	         return YAFFS_FAIL; \
		   }
		 
		 
static int yaffs_CheckStructures(void)
{
	yaffs_CheckStruct(yaffs_Tags,8,"yaffs_Tags")
	yaffs_CheckStruct(yaffs_TagsUnion,8,"yaffs_TagsUnion")
	yaffs_CheckStruct(yaffs_Spare,16,"yaffs_Spare")
#ifndef CONFIG_YAFFS_TNODE_LIST_DEBUG
	yaffs_CheckStruct(yaffs_Tnode,2* YAFFS_NTNODES_LEVEL0,"yaffs_Tnode")
#endif
	yaffs_CheckStruct(yaffs_ObjectHeader,512,"yaffs_ObjectHeader")
	
	
	return YAFFS_OK;
}

#if 0
void yaffs_GutsTest(yaffs_Device *dev)
{
	
	if(yaffs_CheckStructures() != YAFFS_OK)
	{
		T(YAFFS_TRACE_ALWAYS,(TSTR("One or more structures malformed-- aborting\n" TENDSTR)));
		return;
	}
	
	yaffs_TnodeTest(dev);
	yaffs_ObjectTest(dev);	
}
#endif







