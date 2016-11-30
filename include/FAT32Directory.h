// FAT32Directory.h
#pragma once

#include "FAT32.h"

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
    uint16_t seconds: 5;

    /* Minutes (0-59). */
    uint16_t minutes: 6;

    /* Hours (0-23). */
    uint16_t hours: 5;
};

struct FAT32_date_t
{
    /* Day (1-31). */
    uint16_t day: 5;

    /* Month (1-12). */
    uint16_t month: 4;

    /* Year (0-119). 0 = 1980, 199=2099. */
    uint16_t year: 7;
};

struct FAT32_directory_entry_t
{
    /* The name of the file. Long names are not supported. */
    char name[8];

    /* The file extension. */
    char ext[3];

    /* The file attributes. */
    FAT32_dir_entry_attribs_t attribs;

    /* Unused byte. */
    uint8_t unused;

    /* Time the file was created, fine resultion. Measured in multiples of 10ms. */
    uint8_t create_time_fine;

    /* Time the file was created. */
    struct FAT32_time_t create_time;

    /* Date the file was created. */
    struct FAT32_date_t create_date;

    /* Date the file was last accessed. */
    struct FAT32_date_t last_access_date;

    /* High two bytes of the first cluster address. */
    uint16_t first_cluster_index_high;

    /* Time the file was last modified. */
    struct FAT32_time_t last_modified_time;

    /* Date the file was last modified. */
    struct FAT32_date_t last_modified_date;

    /* The low two bytes of the first cluster address. */
    uint16_t first_cluster_index_low;

    /* The size of the file, in bytes. */
    uint32_t size;
};

/* Returns the cluster address contained in a directory entry. */
FAT32_cluster_address_t FAT32_dir_get_entry_address(const struct FAT32_directory_entry_t* entry);

/* Sets the cluster address contained in a directory entry. */
void FAT32_dir_set_entry_address(struct FAT32_directory_entry_t* entry, FAT32_cluster_address_t address);

/* The size of a character array required to hold a null-terminated, formatted directory name. */
#define FAT32_DIR_NAME_LEN 13

/* Returns a formatted name of the directory entry. */
void FAT32_dir_get_entry_name(const struct FAT32_directory_entry_t* entry, char* outName);

/* Sets the name of a directory entry. */
void FAT32_dir_set_entry_name(struct FAT32_directory_entry_t* entry, const char* name);

/* Searches for the first directory entry that matches the given name. */
int FAT32_dir_get_entry(struct FAT32_file_t* dir, const char* name, struct FAT32_directory_entry_t* outEntry);

/* Searches for the first directory entry that has the given cluster address. */
int FAT32_dir_get_entry_by_address(struct FAT32_file_t* dir, FAT32_cluster_address_t address, struct FAT32_directory_entry_t* outEntry);

/* Opens a file containing the directory entry. */
struct FAT32_file_t* FAT32_dir_open_entry(struct FAT32_directory_entry_t* entry);

/* Closes a file handle for the given entry. */
void FAT32_dir_close_entry(struct FAT32_directory_entry_t* entry, struct FAT32_file_t* file);

/* Creates a new file with the given name and attributes in the given directory file. */
int FAT32_dir_new_entry(struct FAT32_file_t* dir, const char* name, FAT32_dir_entry_attribs_t attribs, struct FAT32_directory_entry_t* outEntry);

/* Deletes a file with the given name and attributes from the given directory file. */
int FAT32_dir_remove_entry(struct FAT32_file_t* dir, const char* name);

/* Clears the contents of the given entry. */
void FAT32_dir_clear_entry(struct FAT32_directory_entry_t* entry);
