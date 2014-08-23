
 
#include "yaffsfs.h"
#include "yaffs_guts.h"
#include "yaffscfg.h"
#include "ydirectenv.h"

#define YAFFSFS_MAX_SYMLINK_DEREFERENCES 5

#ifndef NULL
#define NULL ((void *)0)
#endif


const char *yaffsfs_c_version="$Id: yaffsfs.c,v 1.8 2005/08/31 09:21:12 charles Exp $";

static yaffsfs_DeviceConfiguration *yaffsfs_configurationList;


struct yaffsfs_ObjectListEntry
{
	int objectId;
	struct yaffsfs_ObjectListEntry *next;
};


typedef struct
{
	uint32_t magic;
	yaffs_dirent de;
	struct yaffsfs_ObjectListEntry *list;
	char name[NAME_MAX+1];
	
} yaffsfs_DirectorySearchContext;


typedef struct
{
	uint8_t  inUse:1;		// this handle is in use
	uint8_t  readOnly:1;	// this handle is read only
	uint8_t  append:1;		// append only
	uint8_t  exclusive:1;	// exclusive
	uint32_t position;		// current position in file
	yaffs_Object *obj;	// the object
}yaffsfs_Handle;


static yaffs_Object *yaffsfs_FindObject(yaffs_Object *relativeDirectory, const char *path, int symDepth);



static yaffsfs_Handle yaffsfs_handle[YAFFSFS_N_HANDLES];
static int yaffsfs_InitHandles(void)
{
	int i;
	for(i = 0; i < YAFFSFS_N_HANDLES; i++)
	{
		yaffsfs_handle[i].inUse = 0;
		yaffsfs_handle[i].obj = NULL;
	}
	return 0;
}

yaffsfs_Handle *yaffsfs_GetHandlePointer(int h)
{
	if(h < 0 || h >= YAFFSFS_N_HANDLES)
	{
		return NULL;
	}
	
	return &yaffsfs_handle[h];
}

yaffs_Object *yaffsfs_GetHandleObject(int handle)
{
	yaffsfs_Handle *h = yaffsfs_GetHandlePointer(handle);

	if(h && h->inUse)
	{
		return h->obj;
	}
	
	return NULL;
}

static int yaffsfs_GetHandle(void)
{
	int i;
	yaffsfs_Handle *h;
	
	for(i = 0; i < YAFFSFS_N_HANDLES; i++)
	{
		h = yaffsfs_GetHandlePointer(i);
		if(!h)
		{
			// todo bug: should never happen
		}
		if(!h->inUse)
		{
			TEE_MemFill(h,0,sizeof(yaffsfs_Handle));
			h->inUse=1;
			return i;
		}
	}
	return -1;
}

static int yaffsfs_PutHandle(int handle)
{
	yaffsfs_Handle *h = yaffsfs_GetHandlePointer(handle);
	
	if(h)
	{
		h->inUse = 0;
		h->obj = NULL;
	}
	return 0;
}


int yaffsfs_Match(char a, char b)
{
	// case sensitive
	return (a == b);
}

static yaffs_Device *yaffsfs_FindDevice(const char *path, char **restOfPath)
{
	yaffsfs_DeviceConfiguration *cfg = yaffsfs_configurationList;
	const char *leftOver;
	const char *p;
	
	while(cfg && cfg->prefix && cfg->dev)
	{
		leftOver = path;
		p = cfg->prefix;
		while(*p && *leftOver && yaffsfs_Match(*p,*leftOver))
		{
			p++;
			leftOver++;
		}
		if(!*p && (!*leftOver || *leftOver == '/'))
		{
			// Matched prefix
			*restOfPath = (char *)leftOver;
			return cfg->dev;
		}
		cfg++;
	}
	return NULL;
}

static yaffs_Object *yaffsfs_FindRoot(const char *path, char **restOfPath)
{

	yaffs_Device *dev;
	
	dev= yaffsfs_FindDevice(path,restOfPath);
	if(dev && dev->isMounted)
	{
		return dev->rootDir;
	}
	return NULL;
}

static yaffs_Object *yaffsfs_FollowLink(yaffs_Object *obj,int symDepth)
{

	while(obj && obj->variantType == YAFFS_OBJECT_TYPE_SYMLINK)
	{
		char *alias = obj->variant.symLinkVariant.alias;
						
		if(*alias == '/')
		{
			// Starts with a /, need to scan from root up
			obj = yaffsfs_FindObject(NULL,alias,symDepth++);
		}
		else
		{
			// Relative to here, so use the parent of the symlink as a start
			obj = yaffsfs_FindObject(obj->parent,alias,symDepth++);
		}
	}
	return obj;
}

static yaffs_Object *yaffsfs_DoFindDirectory(yaffs_Object *startDir,const char *path,char **name,int symDepth)
{
	yaffs_Object *dir;
	char *restOfPath;
	char str[YAFFS_MAX_NAME_LENGTH+1];
	int i;
	
	if(symDepth > YAFFSFS_MAX_SYMLINK_DEREFERENCES)
	{
		return NULL;
	}
	
	if(startDir)
	{
		dir = startDir;
		restOfPath = (char *)path;
	}
	else
	{
		dir = yaffsfs_FindRoot(path,&restOfPath);
	}
	
	while(dir)
	{	
		while(*restOfPath == '/')
		{
			restOfPath++; // get rid of '/'
		}
		
		*name = restOfPath;
		i = 0;
		
		while(*restOfPath && *restOfPath != '/')
		{
			if (i < YAFFS_MAX_NAME_LENGTH)
			{
				str[i] = *restOfPath;
				str[i+1] = '\0';
				i++;
			}
			restOfPath++;
		}
		
		if(!*restOfPath)
		{
			// got to the end of the string
			return dir;
		}
		else
		{
			if(TEE_StrCmp(str,".") == 0)
			{
				// Do nothing
			}
			else if(TEE_StrCmp(str,"..") == 0)
			{
				dir = dir->parent;
			}
			else
			{
				dir = yaffs_FindObjectByName(dir,str);
				
				while(dir && dir->variantType == YAFFS_OBJECT_TYPE_SYMLINK)
				{
				
					dir = yaffsfs_FollowLink(dir,symDepth);
		
				}
				
				if(dir && dir->variantType != YAFFS_OBJECT_TYPE_DIRECTORY)
				{
					dir = NULL;
				}
			}
		}
	}
	// directory did not exist.
	return NULL;
}

static yaffs_Object *yaffsfs_FindDirectory(yaffs_Object *relativeDirectory,const char *path,char **name,int symDepth)
{
	return yaffsfs_DoFindDirectory(relativeDirectory,path,name,symDepth);
}
 
static yaffs_Object *yaffsfs_FindObject(yaffs_Object *relativeDirectory, const char *path,int symDepth)
{
	yaffs_Object *dir;
	char *name;
	
	dir = yaffsfs_FindDirectory(relativeDirectory,path,&name,symDepth);
	
	if(dir && *name)
	{
		return yaffs_FindObjectByName(dir,name);
	}
	
	return dir;
}



int yaffs_open(const char *path, int oflag, int mode)
{
	yaffs_Object *obj = NULL;
	yaffs_Object *dir = NULL;
	char *name;
	int handle = -1;
	yaffsfs_Handle *h = NULL;
	int alreadyOpen = 0;
	int alreadyExclusive = 0;
	int openDenied = 0;
	int symDepth = 0;
	int errorReported = 0;
	
	int i;
	
	
	yaffsfs_Lock();
	
	handle = yaffsfs_GetHandle();
	
	if(handle >= 0)
	{

		h = yaffsfs_GetHandlePointer(handle);
	
	
		// try to find the exisiting object
		obj = yaffsfs_FindObject(NULL,path,0);
		
		if(obj && obj->variantType == YAFFS_OBJECT_TYPE_SYMLINK)
		{
		
			obj = yaffsfs_FollowLink(obj,symDepth++);
		}

		if(obj)
		{
			// Check if the object is already in use
			alreadyOpen = alreadyExclusive = 0;
			
			for(i = 0; i <= YAFFSFS_N_HANDLES; i++)
			{
				
				if(i != handle &&
				   yaffsfs_handle[i].inUse &&
				    obj == yaffsfs_handle[i].obj)
				 {
				 	alreadyOpen = 1;
					if(yaffsfs_handle[i].exclusive)
					{
						alreadyExclusive = 1;
					}
				 }
			}

			if(((oflag & O_EXCL) && alreadyOpen) || alreadyExclusive)
			{
				openDenied = 1;
			}
			
			// Open should fail if O_CREAT and O_EXCL are specified
			if((oflag & O_EXCL) && (oflag & O_CREAT))
			{
				openDenied = 1;
				yaffsfs_SetError(-EEXIST);
				errorReported = 1;
			}
			
			// Check file permissions
			if( (oflag & (O_RDWR | O_WRONLY)) == 0 &&     // ie O_RDONLY
			   !(obj->yst_mode & S_IREAD))
			{
				openDenied = 1;
			}

			if( (oflag & O_RDWR) && 
			   !(obj->yst_mode & S_IREAD))
			{
				openDenied = 1;
			}

			if( (oflag & (O_RDWR | O_WRONLY)) && 
			   !(obj->yst_mode & S_IWRITE))
			{
				openDenied = 1;
			}
			
		}
		
		else if((oflag & O_CREAT))
		{
			// Let's see if we can create this file
			dir = yaffsfs_FindDirectory(NULL,path,&name,0);
			if(dir)
			{
				obj = yaffs_MknodFile(dir,name,mode,0,0);	
			}
			else
			{
				yaffsfs_SetError(-ENOTDIR);
			}
		}
		
		if(obj && !openDenied)
		{
			h->obj = obj;
			h->inUse = 1;
	    	h->readOnly = (oflag & (O_WRONLY | O_RDWR)) ? 0 : 1;
			h->append =  (oflag & O_APPEND) ? 1 : 0;
			h->exclusive = (oflag & O_EXCL) ? 1 : 0;
			h->position = 0;
			
			obj->inUse++;
			if((oflag & O_TRUNC) && !h->readOnly)
			{
				//todo truncate
				yaffs_ResizeFile(obj,0);
			}
			
		}
		else
		{
			yaffsfs_PutHandle(handle);
			if(!errorReported)
			{
				yaffsfs_SetError(-EACCESS);
				errorReported = 1;
			}
			handle = -1;
		}
		
	}
	
	yaffsfs_Unlock();
	
	return handle;		
}

int yaffs_close(int fd)
{
	yaffsfs_Handle *h = NULL;
	int retVal = 0;
	
	yaffsfs_Lock();

	h = yaffsfs_GetHandlePointer(fd);
	
	if(h && h->inUse)
	{
		// clean up
		yaffs_FlushFile(h->obj,1);
		h->obj->inUse--;
		if(h->obj->inUse <= 0 && h->obj->unlinked)
		{
			yaffs_DeleteFile(h->obj);
		}
		yaffsfs_PutHandle(fd);
		retVal = 0;
	}
	else
	{
		// bad handle
		yaffsfs_SetError(-EBADF);		
		retVal = -1;
	}
	
	yaffsfs_Unlock();
	
	return retVal;
}

int yaffs_read(int fd, void *buf, unsigned int nbyte)
{
	yaffsfs_Handle *h = NULL;
	yaffs_Object *obj = NULL;
	int pos = 0;
	int nRead = -1;
	int maxRead;
	
	yaffsfs_Lock();
	h = yaffsfs_GetHandlePointer(fd);
	obj = yaffsfs_GetHandleObject(fd);
	
	if(!h || !obj)
	{
		// bad handle
		yaffsfs_SetError(-EBADF);		
	}
	else if( h && obj)
	{
		pos=  h->position;
		if(yaffs_GetObjectFileLength(obj) > pos)
		{
			maxRead = yaffs_GetObjectFileLength(obj) - pos;
		}
		else
		{
			maxRead = 0;
		}

		if(nbyte > maxRead)
		{
			nbyte = maxRead;
		}

		
		if(nbyte > 0)
		{
			nRead = yaffs_ReadDataFromFile(obj,buf,pos,nbyte);
			if(nRead >= 0)
			{
				h->position = pos + nRead;
			}
			else
			{
				//todo error
			}
		}
		else
		{
			nRead = 0;
		}
		
	}
	
	yaffsfs_Unlock();
	
	
	return (nRead >= 0) ? nRead : -1;
		
}

int yaffs_write(int fd, const void *buf, unsigned int nbyte)
{
	yaffsfs_Handle *h = NULL;
	yaffs_Object *obj = NULL;
	int pos = 0;
	int nWritten = -1;
	
	yaffsfs_Lock();
	h = yaffsfs_GetHandlePointer(fd);
	obj = yaffsfs_GetHandleObject(fd);
	
	if(!h || !obj)
	{
		// bad handle
		yaffsfs_SetError(-EBADF);		
	}
	else if( h && obj && h->readOnly)
	{
		// todo error
	}
	else if( h && obj)
	{
		if(h->append)
		{
			pos =  yaffs_GetObjectFileLength(obj);
		}
		else
		{
			pos = h->position;
		}
		
		nWritten = yaffs_WriteDataToFile(obj,buf,pos,nbyte);
		
		if(nWritten >= 0)
		{
			h->position = pos + nWritten;
		}
		else
		{
			//todo error
		}
		
	}
	
	yaffsfs_Unlock();
	
	
	return (nWritten >= 0) ? nWritten : -1;

}

off_t yaffs_lseek(int fd, off_t offset, int whence) 
{
	yaffsfs_Handle *h = NULL;
	yaffs_Object *obj = NULL;
	int pos = -1;
	int fSize = -1;
	
	yaffsfs_Lock();
	h = yaffsfs_GetHandlePointer(fd);
	obj = yaffsfs_GetHandleObject(fd);
	
	if(!h || !obj)
	{
		// bad handle
		yaffsfs_SetError(-EBADF);		
	}
	else if(whence == SEEK_SET)
	{
		if(offset >= 0)
		{
			pos = offset;
		}
	}
	else if(whence == SEEK_CUR)
	{
		if( (h->position + offset) >= 0)
		{
			pos = (h->position + offset);
		}
	}
	else if(whence == SEEK_END)
	{
		fSize = yaffs_GetObjectFileLength(obj);
		if(fSize >= 0 && (fSize + offset) >= 0)
		{
			pos = fSize + offset;
		}
	}
	
	if(pos >= 0)
	{
		h->position = pos;
	}
	else
	{
		// todo error
	}

	
	yaffsfs_Unlock();
	
	return pos;
}


int yaffsfs_DoUnlink(const char *path,int isDirectory) 
{
	yaffs_Object *dir = NULL;
	yaffs_Object *obj = NULL;
	char *name;
	int result = YAFFS_FAIL;
	
	yaffsfs_Lock();

	obj = yaffsfs_FindObject(NULL,path,0);
	dir = yaffsfs_FindDirectory(NULL,path,&name,0);
	if(!dir)
	{
		yaffsfs_SetError(-ENOTDIR);
	}
	else if(!obj)
	{
		yaffsfs_SetError(-ENOENT);
	}
	else if(!isDirectory && obj->variantType == YAFFS_OBJECT_TYPE_DIRECTORY)
	{
		yaffsfs_SetError(-EISDIR);
	}
	else if(isDirectory && obj->variantType != YAFFS_OBJECT_TYPE_DIRECTORY)
	{
		yaffsfs_SetError(-ENOTDIR);
	}
	else if(isDirectory && obj->variantType != YAFFS_OBJECT_TYPE_DIRECTORY)
	{
		yaffsfs_SetError(-ENOTDIR);
	}
	else
	{
		result = yaffs_Unlink(dir,name);
		
		if(result == YAFFS_FAIL && isDirectory)
		{
			yaffsfs_SetError(-ENOTEMPTY);
		}
	}
	
	yaffsfs_Unlock();
	
	// todo error
	
	return (result == YAFFS_FAIL) ? -1 : 0;
}
int yaffs_rmdir(const char *path) 
{
	return yaffsfs_DoUnlink(path,1);
}

int yaffs_unlink(const char *path) 
{
	return yaffsfs_DoUnlink(path,0);
}

int yaffs_mount(const char *path)
{
	int retVal=-1;
	int result=YAFFS_FAIL;
	yaffs_Device *dev=NULL;
	char *dummy;
	
	yaffsfs_Lock();
	dev = yaffsfs_FindDevice(path,&dummy);
	if(dev)
	{
		if(!dev->isMounted)
		{
			result = yaffs_GutsInitialise(dev);
			if(result == YAFFS_FAIL)
			{
				// todo error - mount failed
				yaffsfs_SetError(-ENOMEM);
			}
			retVal = result ? 0 : -1;
			
		}
		else
		{
			//todo error - already mounted.
			yaffsfs_SetError(-EBUSY);
		}
	}
	else
	{
		// todo error - no device
		yaffsfs_SetError(-ENODEV);
	}
	yaffsfs_Unlock();
	return retVal;
	
}

int yaffs_unmount(const char *path)
{
	int retVal=-1;
	yaffs_Device *dev=NULL;
	char *dummy;
	
	yaffsfs_Lock();
	dev = yaffsfs_FindDevice(path,&dummy);
	if(dev)
	{
		if(dev->isMounted)
		{
			int i;
			int inUse;
			for(i = inUse = 0; i < YAFFSFS_N_HANDLES && !inUse; i++)
			{
				if(yaffsfs_handle[i].inUse && yaffsfs_handle[i].obj->myDev == dev)
				{
					inUse = 1; // the device is in use, can't unmount
				}
			}
			
			if(!inUse)
			{
				yaffs_Deinitialise(dev);
					
				retVal = 0;
			}
			else
			{
				// todo error can't unmount as files are open
				yaffsfs_SetError(-EBUSY);
			}
			
		}
		else
		{
			//todo error - not mounted.
			yaffsfs_SetError(-EINVAL);
			
		}
	}
	else
	{
		// todo error - no device
		yaffsfs_SetError(-ENODEV);
	}	
	yaffsfs_Unlock();
	return retVal;
	
}

void yaffs_initialise(yaffsfs_DeviceConfiguration *cfgList)
{
	
	yaffsfs_DeviceConfiguration *cfg;
	
	yaffsfs_configurationList = cfgList;
	
	yaffsfs_InitHandles();
	
	cfg = yaffsfs_configurationList;
	
	while(cfg && cfg->prefix && cfg->dev)
	{
		cfg->dev->isMounted = 0;
		cfg++;
	}
	
	
}

