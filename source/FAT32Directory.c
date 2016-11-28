// FAT32Directory.c

#include "../include/FAT32Directory.h"

int get_entry(struct FAT32_file_t* dir, const char* name, struct FAT32_directory_entry_t* outEntry)
{
    // Seek to the beginning of the directory file
    FAT32_fseek(dir, FAT32_SEEK_SET, 0);

    // Loop until the entry is found
    while (FAT32_fread(outEntry, sizeof(FAT32_directory_entry_t), 1 dir))
    {
        if (outEntry.name == name)
        {
            return 1;
        }
    }

    return 0;
}

struct FAT32_file_t* open_entry(struct FAT32_directory_entry_t* entry)
{
    // Construct the address
    FAT32_cluster_address_t address{
        .index_high = entry->first_cluster_index_high,
        .index_low = entry->first_cluster_index_low
    };

    // Open the file
    return FAT32_fopen(address);
}
