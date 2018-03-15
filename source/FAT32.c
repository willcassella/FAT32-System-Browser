// FAT32.c

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include "../include/FAT32.h"

/* The number of bytes in a FAT32 cluster */
#define FAT32_CLUSTER_SIZE 8
#define FAT32_NUM_CLUSTERS 64
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

void FAT32_init(void)
{
	// Set the root cluster table entry to the EOC code
	FAT32_cluster_address_t rootCluster = FAT32_get_root();
	FAT32_cluster_address_t value;
	value.index = FAT32_CLUSTER_ADDRESS_EOC;
	set_table_entry(rootCluster, value);
}

FAT32_cluster_address_t FAT32_get_root(void)
{
	FAT32_cluster_address_t result;
	result.index = 1;
	return result;
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
            break;
        }
    }

	// Make sure we didn't run out of clusters
	assert(result.index != FAT32_NUM_CLUSTERS /* All out of clusters! */);

	// Set the value as the EOC value
	FAT32_cluster_address_t resultValue;
	resultValue.index = FAT32_CLUSTER_ADDRESS_EOC;
	set_table_entry(result, resultValue);

	// Zero out the hard drive bytes
	memset(get_data_entry(result), 0, FAT32_CLUSTER_SIZE);

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

    /* The readable size of the file, in bytes. */
    uint32_t size;

	/* Stores whether the file has been modified. */
	int modified;
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
	file->modified = 0;

    return file;
}

void FAT32_free_cluster(FAT32_cluster_address_t address)
{
	FAT32_cluster_address_t nextAddr;

	while (address.index != FAT32_CLUSTER_ADDRESS_EOC)
	{
		// Get the address of the next cluster
		nextAddr = get_table_entry(address);

		// Null out this one
		FAT32_cluster_address_t value;
		value.index = FAT32_CLUSTER_ADDRESS_NULL;
		set_table_entry(address, value);

		// Move to the next address
		address = nextAddr;
	}
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
        if (FAT32_ftell(file) >= file->size)
        {
            break;
        }

        // If we're at the end of this cluster
        if (file->cluster_offset >= FAT32_CLUSTER_SIZE)
        {
			// Get the next cluster in the chain
			FAT32_cluster_address_t nextCluster = get_table_entry(file->current_cluster);

			// If we're already at the end of the chain
			if (nextCluster.index == FAT32_CLUSTER_ADDRESS_EOC)
			{
				break;
			}

			// Move to the next cluster
			file->current_cluster = nextCluster;
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
	// Mark the file as being modified
	file->modified = 1;

    uint32_t offset = 0;
    for (; offset < count * size; ++offset, ++file->cluster_offset)
    {
        // If we've reached the end of this cluster
        if (file->cluster_offset >= FAT32_CLUSTER_SIZE)
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

static void seek_forward(struct FAT32_file_t* file, long distance)
{
	// While there's still more to go
	while (distance > 0 && FAT32_ftell(file) < file->size)
	{
		// Move forward
		file->cluster_offset += 1;
		distance -= 1;

		// If we've reached the end of this cluster
		if (file->cluster_offset >= FAT32_CLUSTER_SIZE)
		{
			// Get the next cluster
			FAT32_cluster_address_t nextCluster = get_table_entry(file->current_cluster);

			// If we're at the end of the chain
			if (nextCluster.index == FAT32_CLUSTER_ADDRESS_EOC)
			{
				break;
			}

			// Move to the next cluster
			file->current_cluster = nextCluster;
			file->current_cluster_distance += 1;
			file->cluster_offset = 0;
		}
	}
}

int FAT32_fseek(struct FAT32_file_t* file, long offset, int origin)
{
    // If they're seeking forward or to an origin
	if (offset >= 0)
	{
		switch (origin)
		{
		case FAT32_SEEK_SET:
			FAT32_rewind(file);
			seek_forward(file, offset);
			return 0;

		case FAT32_SEEK_CUR:
			seek_forward(file, offset);
			return 0;

		case FAT32_SEEK_END:
			seek_forward(file, LONG_MAX);
			return 0;

		default:
			return 1;
		}
	}

	long target;

	// If they're seeking backward
	if (offset < 0)
	{
		switch (origin)
		{
		case FAT32_SEEK_SET:
			FAT32_rewind(file);
			return 0;

		case FAT32_SEEK_CUR:
			target = FAT32_ftell(file) + offset;
			FAT32_rewind(file);
			seek_forward(file, target);
			return 0;

		case FAT32_SEEK_END:
			seek_forward(file, LONG_MAX);
			target = FAT32_ftell(file) + offset;
			FAT32_rewind(file);
			seek_forward(file, target);
			return 0;

		default:
			return 1;
		}
	}

    return 1;
}

void FAT32_rewind(struct FAT32_file_t* file)
{
	// Just go back to the beginning
	file->current_cluster = file->start_cluster;
	file->current_cluster_distance = 0;
	file->cluster_offset = 0;
}

long FAT32_ftell(const struct FAT32_file_t* file)
{
    return file->current_cluster_distance * FAT32_CLUSTER_SIZE + file->cluster_offset;
}

FAT32_cluster_address_t FAT32_faddress(const struct FAT32_file_t* file)
{
    return file->start_cluster;
}

int FAT32_fmodified(const struct FAT32_file_t* file)
{
	return file->modified;
}

void FAT32_print_disk(void)
{
	FAT32_cluster_address_t address;
	address.index = 0;

	// For each cluster
	for (; address.index < FAT32_NUM_CLUSTERS; ++address.index)
	{
		HDByte_t cluster[FAT32_CLUSTER_SIZE];
		memset(cluster, ' ', FAT32_CLUSTER_SIZE);

		// If the cluster contains actual data
		if (get_table_entry(address).index != FAT32_CLUSTER_ADDRESS_NULL)
		{
			memcpy(cluster, get_data_entry(address), FAT32_CLUSTER_SIZE);

			// Remove unwanted characters
			for (size_t i = 0; i < FAT32_CLUSTER_SIZE; ++i)
			{
				const char c = cluster[i];
				if (c == '\a' || c == '\b' || c == '\e' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
				{
					cluster[i] = ' ';
				}
			}
		}

		// Print the contents
		printf("[");
		fwrite(cluster, 1, FAT32_CLUSTER_SIZE, stdout);
		printf("]\n");
	}
}
