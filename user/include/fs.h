#ifndef _FS_H_
#define _FS_H_ 1

#include <stdint.h>

#define FMODE_R 0x4
#define FMODE_W 0x2
#define FMODE_X 0x1
#define FMODE_RW 0x6
#define FMODE_ALL 0x7

#define STMODE2FMODE(st_mode) (((st_mode) >> 6) & FMODE_ALL)


// File nodes (both in-memory and on-disk)

// Bytes per file system block - same as page size
#define BLOCK_SIZE PAGE_SIZE
#define BLOCK_SIZE_BIT (BLOCK_SIZE * 8)

// Maximum size of a filename (a single path component), including null
#define MAXNAMELEN 128

// Maximum size of a complete pathname, including null
#define MAXPATHLEN 1024

// Number of (direct) block pointers in a File descriptor
#define NDIRECT 10
#define NINDIRECT (BLOCK_SIZE / 4)

#define MAXFILESIZE (NINDIRECT * BLOCK_SIZE)

#define FILE_STRUCT_SIZE 256

struct File {
	char f_name[MAXNAMELEN]; // filename
	uint32_t f_size;	 // file size in bytes
	uint32_t f_type;	 // file type 文件类型，有普通文件 (FTYPE_REG) 和目录 (FTYPE_DIR) 两种
	uint32_t f_direct[NDIRECT];
	/*
		文件的直接指针，每个文件
		控制块设有 10 个直接指针，用来记录文件的数据块在磁盘上的位置。每个磁盘块的大小为 4KB，
		也就是说，这十个直接指针能够表示最大 40KB 的文件，而当文件的大小大于 40KB 时，就需要
		用到间接指针。f_indirect 指向一个间接磁盘块，用来存储指向文件内容的磁盘块的指针。
		对于普通的文件，其指向的磁盘块存储着文件内容，而对于目录文件来说，其指向的磁盘块
		存储着该目录下各个文件对应的文件控制块。
	*/
	uint32_t f_indirect;

	

	struct File *f_dir; // the pointer to the dir where this file is in, valid only in memory.

	uint32_t f_mode;
	char f_pad[FILE_STRUCT_SIZE - MAXNAMELEN - (4 + NDIRECT) * 4 - sizeof(void *)];
} __attribute__((aligned(4), packed));

// 一个block里有多少个文件
#define FILE2BLK (BLOCK_SIZE / sizeof(struct File))

// File types
#define FTYPE_REG 0 // Regular file
#define FTYPE_DIR 1 // Directory

// File system super-block (both in-memory and on-disk)

#define FS_MAGIC 0x68286097 // Everyone's favorite OS class

struct Super {
	uint32_t s_magic;   // Magic number: FS_MAGIC 魔数，为一个常量，用于标识该文件系统。
	uint32_t s_nblocks; // Total number of blocks on disk 记录本文件系统有多少个磁盘块，在本文件系统中为 1024
	struct File s_root; // Root directory node 根目录，其 f_type 为 FTYPE_DIR，f_name 为“/”。
};

#endif // _FS_H_
