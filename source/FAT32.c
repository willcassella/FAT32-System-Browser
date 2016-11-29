// FAT32.c

#include <string.h>
#include <stdlib.h>
#include "../include/FAT32.h"

/* The number of bytes in a FAT32 cluster */
#define FAT32_CLUSTER_SIZE 32
#define FAT32_NUM_CLUSTERS 256
#define FAT32_TABLE_SIZE (sizeof(FAT32_cluster_address_t) * FAT32_NUM_CLUSTERS)
#define FAT32_DATA_SIZE (FAT32_CLUSTER_SIZE * FAT32_NUM_CLUSTERS)

/* Type used to represent a byte on the hard drive. */
typedef uint8_t HDByte_t;

/* Virtual Hard drive object. */
HDByte_t FAT32_HARD_DRIVE[FAT32_TABLE_SIZE + FAT32_DATA_SIZE];

/* Returns the address stored in the File Allocation Table for the given address. */
static FAT32_cluster_address_t get_table_entry(FAT32_cluster_address_t address)
{
    return ((FAT32_cluster_address_t*)FAT32_HARD_DRIVE)[address.index];
}

/* Sets the address stored in the File Allocation Table for the given address. */
static void set_table_entry(FAT32_cluster_address_t address, FAT32_cluster_address_t value)
{
    ((FAT32_cluster_address_t*)FAT32_HARD_DRIVE)[address.index] = value;
}

static HDByte_t* get_data_entry(FAT32_cluster_address_t address)
{
    return &FAT32_HARD_DRIVE[FAT32_TABLE_SIZE + address.index * FAT32_CLUSTER_SIZE];
}

FAT32_cluster_address_t FAT32_new_cluster(void)
{
    FAT32_cluster_address_t result;

    // For each cluster address
    for (result.index = 1; result.index < FAT32_NUM_CLUSTERS; ++result.index)
    {
        // If the FAT value for this address is NULL, it's unused
        if (get_table_entry(result).index == FAT32_CLUSTER_ADDRESS_NULL)
        {
            // Set the value as the End of Chain value
            FAT32_cluster_address_t resultValue;
            resultValue.index = FAT32_CLUSTER_ADDRESS_EOC;
            set_table_entry(result, resultValue);

			// Zero out the hard drive bytes
			memset(get_data_entry(result), 0, FAT32_CLUSTER_SIZE);

            break;
        }
    }

    return result;
}

FAT32_cluster_address_t FAT32_get_root(void)
{
	FAT32_cluster_address_t result;
	result.index = 1;
	return result;
}

struct FAT32_file_t
{
    /* The address of the starting cluster of this file. */
    FAT32_cluster_address_t start_cluster;

    /* The address of the current cluster for this file. */
    FAT32_cluster_address_t current_cluster;

    /* How far in the chain the current cluster is. */
    uint32_t current_cluster_distance;

    /* The byte offset within the current cluster. */
    uint32_t cluster_offset;

    /* The semantic size of the file, in bytes. */
    uint32_t size;
};

struct FAT32_file_t* FAT32_fopen(FAT32_cluster_address_t address, uint32_t size)
{
    // Create a file object
    struct FAT32_file_t* file = (struct FAT32_file_t*)malloc(sizeof(struct FAT32_file_t));
    file->start_cluster = address;
    file->current_cluster = address;
    file->current_cluster_distance = 0;
    file->cluster_offset = 0;
    file->size = size;

    return file;
}

void FAT32_free_cluster(FAT32_cluster_address_t address)
{
	FAT32_cluster_address_t nextAddr;

	do
	{
		// Get the address of the next cluster
		nextAddr = get_table_entry(address);

		// Null out this one
		FAT32_cluster_address_t value;
		value.index = FAT32_CLUSTER_ADDRESS_NULL;
		set_table_entry(address, value);
	}
	while (nextAddr.index != FAT32_CLUSTER_ADDRESS_EOC);
}

int FAT32_fclose(struct FAT32_file_t* file)
{
    free(file);
    return 0;
}

size_t FAT32_fread(void* buffer, size_t size, size_t count, struct FAT32_file_t* file)
{
    // Fill the buffer with bytes
    uint32_t offset = 0;

    // For each byte to be read
    for (; offset < count * size; ++offset, ++file->cluster_offset)
    {
        // If we've reached the end of this file
        if (FAT32_ftell(file) > file->size)
        {
            break;
        }

        // If we're at the end of this cluster
        if (file->cluster_offset == FAT32_CLUSTER_SIZE)
        {
            file->current_cluster = get_table_entry(file->current_cluster);
            file->current_cluster_distance += 1;
            file->cluster_offset = 0;
        }

		// Read the byte
        ((HDByte_t*)buffer)[offset] = get_data_entry(file->current_cluster)[file->cluster_offset];
    }

    return offset / size;
}

size_t FAT32_fwrite(const void* buffer, size_t size, size_t count, struct FAT32_file_t* file)
{
    // Read the buffer as bytes
    uint32_t offset = 0;

    for (; offset < count * size; ++offset, ++file->cluster_offset)
    {
        // If we've reached the end of this cluster
        if (file->cluster_offset == FAT32_CLUSTER_SIZE)
        {
            // Get the next cluster in the chain
            FAT32_cluster_address_t nextCluster = get_table_entry(file->current_cluster);

            // If we're at the last cluster in this chain
            if (nextCluster.index == FAT32_CLUSTER_ADDRESS_EOC)
            {
                // Create a new cluster
                nextCluster = FAT32_new_cluster();
                set_table_entry(file->current_cluster, nextCluster);
            }

            // Move to the next cluster
            file->current_cluster = nextCluster;
            file->current_cluster_distance += 1;
            file->cluster_offset = 0;
        }

		// Write the byte
		get_data_entry(file->current_cluster)[file->cluster_offset] = ((const HDByte_t*)buffer)[offset];
    }

    // Update the size of the file
    const long pos = FAT32_ftell(file);
    file->size = pos > file->size ? pos : file->size;

    return offset / size;
}

int FAT32_fseek(struct FAT32_file_t* file, long offset, int origin)
{
    // If they're seeking to the beginning
    if (origin == FAT32_SEEK_SET)
    {
        file->current_cluster = file->start_cluster;
        file->cluster_offset = 0;
    }

    // Set the offset
    file->cluster_offset = offset % FAT32_CLUSTER_SIZE;
    offset -= file->cluster_offset;

    // Set the current cluster
    while (offset != 0)
    {
        file->current_cluster = get_table_entry(file->current_cluster);
        offset /= FAT32_CLUSTER_SIZE;
    }

    // Move back so we're not past the end
    //file->offset = file->offset < file->end_offset ? file->offset : file->end_offset;

    return 0;
}

long FAT32_ftell(struct FAT32_file_t* file)
{
    return file->current_cluster_distance * FAT32_CLUSTER_SIZE + file->cluster_offset;
}

FAT32_cluster_address_t FAT32_faddress(struct FAT32_file_t* file)
{
    return file->start_cluster;
}
