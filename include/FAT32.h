// Fat32.h
#pragma once

#include <stdint.h>

/* Represents the address of a FAT32 cluster. */
struct FAT32_cluster_address_t
{
    /* The first four bits of a 32-bit cluster address are unused by FAT32.  */
    uint32_t unused : 4;

    uint32_t index : 28;
};

/* A cluster address with this index indicates is treated as 'NULL'. */
#define FAT32_CLUSTER_ADDRESS_NULL 0x00000000

/* A cluster address with this index indicates that this cluster is the end of the cluster chain. */
#define FAT32_CLUSTER_ADDRESS_EOC 0xFFFFFF0F

struct FAT32_file_t;

/* Returns the cluster address of the root directory in the file system. */
struct FAT32_cluster_address_t FAT32_get_root(void);

/* Opens a FAT32 file, given its starting cluster address. */
struct FAT32_file_t* FAT32_fopen(struct FAT32_cluster_address_t address, size_t endOffset);

/* Returns the starting cluster address of the given file object. */
struct FAT32_cluster_address_t FAT32_faddress(struct FAT32_file_t* file);

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
