// FAT32.c

#include "../include/FAT32.h"

/* Represents the address of a FAT32 cluster. */
struct FAT32_cluster_address_t
{
    /* The first four bits of a 32-bit cluster address are unused by FAT32.  */
    uint16_t unused : 4;

    /* The high 12 bits of a cluster addres. */
    uint16_t index_high : 12;

    /* The remaining 28 bits represent the index of the cluster. */
    uint16_t index_low;
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
