#include "yaffsfs.h"

void ListDir(const char *DirName)
{
	yaffs_DIR *d;
	yaffs_dirent *de;
	struct yaffs_stat s;
	char str[100];
			
	d = yaffs_opendir(DirName);
	
	if(!d)
	{
		printf("opendir failed\n");
	}
	else
	{
		printf("%s\n", DirName);
		while((de = yaffs_readdir(d)) != NULL)
		{
			sprintf(str,"%s/%s",DirName,de->d_name);
			yaffs_lstat(str,&s);
			printf("%20s        %8d     %5d    ", de->d_name, s.yst_size, s.yst_mode);
			
			switch(s.yst_mode & S_IFMT)
			{
				case S_IFREG: printf("data file"); break;
				case S_IFDIR: printf("directory"); break;
				case S_IFLNK: printf("symlink");   break;
				default: printf("unknown"); break;
			}
		
			printf("\n");		
		}
		
		yaffs_closedir(d);
	}
	
	printf("FreeSpace: %d\n\n", yaffs_freespace(DirName));
}


void main(void)
{
	int i = 0;
	int f = 0;
	char buf[100];
	struct yaffs_stat s;
	
	yaffs_StartUp();
	yaffs_mount("/c");
	yaffs_mount("/d");
/*
	yaffs_mkdir("/c/mydoc", 0);
	yaffs_mkdir("/d/data0", 0);
	yaffs_mkdir("/d/data1", 0);
	
	f = yaffs_open("/d/data0/file1.gsk", O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);
	yaffs_close(f);
	
	f = yaffs_open("/d/data0/file2.gsk", O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);
	yaffs_close(f);
*/
	yaffs_unlink("/d/data0/file1");
	ListDir("/d");
	ListDir("/d/data0");
}
