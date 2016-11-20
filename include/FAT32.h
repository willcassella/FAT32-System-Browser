// Fat32.h
#pragma once

#include <stdint.h>

/* Represents the address of a FAT32 cluster. */
struct FAT32_cluster_address_t
{
    /* The first four bits of a 32-bit cluster address are unused by FAT32.  */
    uint16_t unused : 4;

    /* The high 12 bits of a cluster addres. */
    uint16_t index_high : 12;

    /* The remaining 16 bits of the index of the cluster. */
    uint16_t index_low;
};

struct FAT32_file_t
{
    void* location;
    size_t size;
};

struct FAT32_file_t* FAT32_fopen(const char* dir);
void FAT32_fclose(struct FAT32_file_t* file);

enum
{
    /* If this bit is set, the operating system will not allow a file to be opened for modification. */
    FAT32_DIR_ENTRY_ATTRIB_READ_ONLY = 0x01,

    /* Hides files or directories from normal directory views. */
    FAT32_DIR_ENTRY_ATTRIB_HIDDEN = 0x02,

    /* Indicates that the file belongs to the system and must not be
    * physically moved (e.g., during defragmentation), because there may be
    * references into the file using absolute addressing bypassing the file
    * system (boot loaders, kernel images, swap files, extended attributes, etc.).*/
    FAT32_DIR_ENTRY_ATTRIB_SYSTEM = 0x04,

    /* Indicates that the cluster-chain associated with this entry gets interpreted as subdirectory instead of as a file.
    * Subdirectories have a filesize entry of zero. */
    FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY = 0x010,
};
typedef uint8_t FAT32_dir_entry_attribs_t;

struct FAT32_time_t
{
    /* Seconds (0-29), measured in multiples of 2. */
    uint16_t seconds: 4;

    /* Minutes (0-59). */
    uint16_t minutes: 5;

    /* Hours (0-23). */
    uint16_t hours: 7;
};

struct FAT32_date_t
{
    /* Day (1-31). */
    uint16_t day: 4;

    /* Month (1-12). */
    uint16_t month: 3;

    /* Year (0-119). 0 = 1980, 199=2099. */
    uint16_t year: 8;
};

struct FAT32_filesystem_t;

/* Creates a virtual FAT32 filesystem with the given size (number of bytes). */
struct FAT32_filesystem_t* FAT32_create_filesystem(size_t size);

/* Frees the given FAT32 filesystem. */
void FAT32_free_filesystem(struct FAT32_filesystem_t* filesystem);

/* Returns the cluster address of the root directory in the file system. */
struct FAT32_cluster_address_t FAT32_get_root(struct FAT32_filesystem_t* filesystem);

/* Returns the creation time of a file entry, high resolution. Measured in multiples of 10 ms. */
uint8_t FAT32_get_creation_time_fine(struct FAT32_directory_entry_t* file);

/* Returns the creation time value for a directory entry. */
struct FAT32_time_t FAT32_get_creation_time(struct FAT32_directory_entry_t* file);

/* Returns the creation date value for a directory entry. */
struct FAT32_date_t FAT32_get_creation_date(struct FAT32_directory_entry_t* file);

struct FAT32_directory_entry_t* FAT32_get_directory_entry(struct FAT32_cluster_address_t directory, const char* name);
