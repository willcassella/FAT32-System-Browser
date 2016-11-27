// FAT32.c

#include "../include/FAT32.h"

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
