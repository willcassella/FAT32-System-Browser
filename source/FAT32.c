// FAT32.c

#include "../include/FAT32.h"

/* The number of bytes in a FAT32 cluster */
#define FAT32_CLUSTER_SIZE 32
#define FAT32_NUM_CLUSTERS 256
#define FAT32_TABLE_SIZE (sizeof(FAT32_cluster_address_t) * FAT32_NUM_CLUSTERS)
#define FAT32_DATA_SIZE (FAT32_CLUSTER_SIZE * FAT32_NUM_CLUSTERS)

typedef uint8_t HDByte_t;
HDByte_t FAT32_HARD_DRIVE[FAT32_TABLE_SIZE + FAT32_DATA_SIZE];

/* Returns the address stored in the File Allocation Table for the given address. */
static struct FAT32_cluster_address_t get_table_entry(struct FAT32_cluster_address_t address)
{
    return ((struct FAT32_cluster_address_t*)FAT32_HARD_DRIVE)[address.index];
}

/* Sets the address stored in the File Allocation Table for the given address. */
static void set_table_entry(struct FAT32_cluster_address_t address, struct FAT32_cluster_address_t value)
{
    ((struct FAT32_cluster_address_t*)FAT32_HARD_DRIVE)[address.index] = value;
}

static HDByte_t* get_data_entry(struct FAT32_cluster_address_t address)
{
    return FAT32_HARD_DRIVE[FAT32_TABLE_SIZE + address.index * FAT32_CLUSTER_SIZE];
}

static struct FAT32_cluster_address_t get_empty(void)
{
    struct FAT32_cluster_address_t result;

    for (result.index = 1; result.index < NUM_CLUSTERS; ++i)
    {
        if (table_entry(result).index == FAT32_CLUSTER_ADDRESS_NULL)
        {
            set_table_entry(address, FAT32_CLUSTER_ADDRESS_EOC);
            return result;
        }
    }
}

struct FAT32_file_t
{
    /* The address of the starting cluster of this file. */
    struct FAT32_cluster_address_t start_cluster;

    /* The address of the current cluster for this file. */
    struct FAT32_cluster_address_t current_cluster;

    /* How far in the chain the current cluster is. */
    size_t current_cluster_distance;

    /* The byte offset within the current cluster. */
    size_t offset;

    /* The byte offset into the final cluster that this file ends. */
    size_t end_offset;
};

struct FAT32_file_t* FAT32_fopen(struct FAT32_cluster_address_t address, size_t endOffset)
{
    // Create a file object
    struct FAT32_file_t* file = (struct FAT32_file_t*)malloc(sizeof(struct FAT32_file_t));
    file->start_cluster = address;
    file->current_cluster = address;
    file->current_cluster_distance = 0;
    file->offset = 0;
    file->end_offset = endOffset;

    return file;
}

int FAT32_fclose(struct FAT32_file_t* file)
{
    free(file);
    return 0;
}

size_t FAT32_fread(void* buffer, size_t size, size_t count, struct FAT32_file_t* file)
{
    // Fill the buffer with bytes
    byte* cBuffer = (byte*)buffer;
    size_t offset = 0;

    // For each byte to be read
    for (; offset < count * size; ++offset, ++file->offset)
    {
        // If we've reached the end of this file
        if (file->offset == file->end_offset && get_table_entry(file->current_cluster).index == FAT32_CLUSTER_ADDRESS_EOC)
        {
            break;
        }

        // If we're at the end of this cluster
        if (file->offset == FAT32_CLUSTER_SIZE)
        {
            file->current_cluster = table_entry(file->current_cluster);
            file->current_cluster_distance += 1;
            file->offset = 0;
        }

        cBuffer[offset] = data_entry(file->current_cluster)[file->offset];
    }

    return offset / size;
}

size_t FAT32_fwrite(const void* buffer, size_t size, size_t count, struct FAT32_file_t* file)
{
    // Read the buffer as bytes
    byte* cBuffer = (byte*)buffer;
    size_t offset = 0;

    for (; offset < count * size; ++offset, ++file->offset)
    {
        // If we've reached the end of this cluster
        if (file->offset == FAT32_CLUSTER_SIZE)
        {
            // Get the next cluster in the chain
            struct FAT32_cluster_address_t nextCluster = get_table_entry(file->current_cluster);

            // If we're at the last cluster in this chain
            if (nextCluster.index == FAT32_CLUSTER_ADDRESS_EOC)
            {
                // Create a new cluster
                nextCluster = get_empty();
                set_table_entry(file->current_cluster, nextCluster);
            }

            // Move to the next cluster
            file->current_cluster = nextCluster;
            file->current_cluster_distance += 1;
            file->offset = 0;
        }
    }

    // If we finished at the last cluster
    if (get_table_entry(file->current_cluster).index = FAT32_CLUSTER_ADDRESS_EOC)
    {
        file->end_offset = file->offset < file->end_offset ? file->end_offset : file->offset;
    }

    return offset / size;
}

int FAT32_fseek(struct FAT32_file_t* file, long offset, int origin)
{
    // If they're seeking to the beginning
    if (origin == FAT32_SEEK_SET)
    {
        file->current_cluster = file->start_cluster;
        file->offset = 0;
    }

    // Set the offset
    file->offset = offset % FAT32_CLUSTER_SIZE;
    offset -= file->offset;

    // Set the current cluster
    while (offset != 0)
    {
        file->current_cluster = table_entry(file->current_cluster);
        offset /= FAT32_CLUSTER_SIZE;
    }

    // Move back so we're not past the end
    file->offset = file->offset < file->end_offset ? file->offset : file->end_offset;

    return 0;
}
