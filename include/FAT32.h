// Fat32.h
#pragma once

#include <stdint.h>
#include <stddef.h>

/* Represents the address of a FAT32 cluster. */
typedef union
{
	struct
	{
		/* The first four bits are unused. */
		uint32_t : 4;

		/* The entire value of the index. */
		uint32_t index : 28;
	};

	struct
	{
		/* The first four bits are unused. */
		uint16_t : 4;

		/* The high two bytes of the index. */
		uint16_t index_high : 12;

		/* The low two bytes of the index. */
		uint16_t index_low;
	};

} FAT32_cluster_address_t;

/* A cluster address with this index indicates is treated as 'NULL'. */
#define FAT32_CLUSTER_ADDRESS_NULL 0x0000000

/* A cluster address with this index indicates that this cluster is the end of the cluster chain. */
#define FAT32_CLUSTER_ADDRESS_EOC 0xFFFFFF

struct FAT32_file_t;

/* Initializes the FAT32 file system. */
void FAT32_init(void);

/* Returns the cluster address of the root directory in the file system. */
FAT32_cluster_address_t FAT32_get_root(void);

/* Reserves an empty cluster, and returns the address to the caller. */
FAT32_cluster_address_t FAT32_new_cluster(void);

/* Frees all clusters in the chain given by 'address'. */
void FAT32_free_cluster(FAT32_cluster_address_t address);

/* Opens a FAT32 file, given its starting cluster address, and the size of the file. */
struct FAT32_file_t* FAT32_fopen(FAT32_cluster_address_t address, uint32_t size);

/* Closes a FAT32 file. */
int FAT32_fclose(struct FAT32_file_t* file);

/* Works the same was as normal 'fread'. */
size_t FAT32_fread(void* buffer, size_t size, size_t count, struct FAT32_file_t* file);

/* Works the same way as normal 'fwrite'. */
size_t FAT32_fwrite(const void* buffer, size_t size, size_t count, struct FAT32_file_t* file);

/* Sets the seek origin to the beginning of the file. */
#define FAT32_SEEK_SET -1

/* Sets the seek origin to the current position within the file. */
#define FAT32_SEEK_CUR 0

/* Sets the seek origin to the end of the file. */
#define FAT32_SEEK_END 1

/* Works the same was as normal 'fseek'. */
int FAT32_fseek(struct FAT32_file_t* file, long offset, int origin);

/* Returns the file handle to the start of the file. */
void FAT32_rewind(struct FAT32_file_t* file);

/* Returns the number of bytes in the file reader is. */
long FAT32_ftell(struct FAT32_file_t* file);

/* Returns the starting cluster address of the given file object. */
FAT32_cluster_address_t FAT32_faddress(struct FAT32_file_t* file);
