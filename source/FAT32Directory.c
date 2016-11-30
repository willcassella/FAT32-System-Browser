// FAT32Directory.c

#include <string.h>
#include "../include/FAT32Directory.h"

struct FAT32_entry_name_t
{
	char name[8];
	char ext[3];
};

static struct FAT32_entry_name_t path_to_name(const char* path)
{
	struct FAT32_entry_name_t result;

	// Extract the name
	for (size_t i = 0; i < 8; ++i)
	{
		if (*path == '.' || *path == 0)
		{
			result.name[i] = ' ';
		}
		else
		{
			result.name[i] = *path;
			path += 1;
		}
	}

	// If we stopped adding the name because we reached a dot
	if (*path == '.')
	{
		path += 1;
	}

	// Extract the extension
	for (size_t i = 0; i < 3; ++i)
	{
		if (*path == 0)
		{
			result.ext[i] = ' ';
		}
		else
		{
			result.ext[i] = *path;
			path += 1;
		}
	}

	return result;
}

static struct FAT32_entry_name_t get_entry_name(const struct FAT32_directory_entry_t* entry)
{
	struct FAT32_entry_name_t result;
	memcpy(result.name, entry->name, 8);
	memcpy(result.ext, entry->ext, 3);

	return result;
}

static FAT32_cluster_address_t get_entry_address(const struct FAT32_directory_entry_t* entry)
{
	FAT32_cluster_address_t result;
	result.index_high = entry->first_cluster_index_high;
	result.index_low = entry->first_cluster_index_low;

	return result;
}

static void set_entry_name(struct FAT32_directory_entry_t* entry, const struct FAT32_entry_name_t* name)
{
	memcpy(entry->name, name->name, 8);
	memcpy(entry->ext, name->ext, 3);
}

static int compare_names(const struct FAT32_entry_name_t* lhs, const struct FAT32_entry_name_t* rhs)
{
	return strncmp(lhs->name, rhs->name, 8) || strncmp(lhs->ext, rhs->ext, 3);
}

static void delete_entry(struct FAT32_directory_entry_t* entry)
{
	// If the entry is a subdirectory
	if (entry->attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY)
	{
		// Open 'er up
		struct FAT32_file_t* file = FAT32_dir_open_entry(entry);

		// Delete all entries in the subdirectory
		struct FAT32_directory_entry_t subEntry;
		while (FAT32_fread(&subEntry, sizeof(subEntry), 1, file))
		{
			delete_entry(&subEntry);
		}

		FAT32_fclose(file);
	}

	// Free the cluster chain
	FAT32_free_cluster(get_entry_address(entry));
}

int FAT32_dir_get_entry(struct FAT32_file_t* dir, const char* name, struct FAT32_directory_entry_t* outEntry)
{
	// Get the name of the object
	const struct FAT32_entry_name_t pathName = path_to_name(name);

    // Seek to the beginning of the directory file
    FAT32_fseek(dir, FAT32_SEEK_SET, 0);

    // Loop until the entry is found
    while (FAT32_fread(outEntry, sizeof(struct FAT32_directory_entry_t), 1, dir))
    {
		const struct FAT32_entry_name_t entryName = get_entry_name(outEntry);

        if (compare_names(&pathName, &entryName) == 0)
        {
			// Rewind to the start of the entry
			FAT32_fseek(dir, -sizeof(struct FAT32_directory_entry_t), FAT32_SEEK_CUR);
            return 1;
        }
    }

    return 0;
}

struct FAT32_file_t* FAT32_dir_open_entry(struct FAT32_directory_entry_t* entry)
{
    // Construct the address
	FAT32_cluster_address_t address = get_entry_address(entry);

	// If the entry is a subdirectory, open it with max size (directories are unsized)
	if (entry->attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY)
	{
		return FAT32_fopen(address, UINT32_MAX);
	}

	// Open the file, respecting its size
	return FAT32_fopen(address, entry->size);
}

void FAT32_dir_close_entry(struct FAT32_directory_entry_t* entry, struct FAT32_file_t* file)
{
	// If the entry is not a directory, update the size
	if ((entry->attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY) == 0)
	{
		FAT32_fseek(file, 0, FAT32_SEEK_END);
		entry->size = FAT32_ftell(file);
	}

	// Close the file
	FAT32_fclose(file);
}

int FAT32_dir_new_entry(struct FAT32_file_t* dir, const char* name, FAT32_dir_entry_attribs_t attribs, struct FAT32_directory_entry_t* outEntry)
{
	// Create a name object for the given name
	const struct FAT32_entry_name_t nameObj = path_to_name(name);

	// Seek to the beginning of the file
	FAT32_fseek(dir, 0, FAT32_SEEK_SET);

	// Loop until we either find an empty entry, or we run out of space
	while (FAT32_fread(outEntry, sizeof(struct FAT32_directory_entry_t), 1, dir))
	{
		if (outEntry->name[0] == 0)
		{
			break;
		}
	}

	// Set file properties TODO
	set_entry_name(outEntry, &nameObj);
	outEntry->attribs = attribs;

	// Rewind back to the start of the entry
	FAT32_fseek(dir, -sizeof(struct FAT32_directory_entry_t), FAT32_SEEK_CUR);

	// Write the entry
	FAT32_fwrite(outEntry, sizeof(struct FAT32_directory_entry_t), 1, dir);
	return 1;
}

int FAT32_dir_remove_entry(struct FAT32_file_t* dir, const char* name)
{
	// Get the entry to be removed
	struct FAT32_directory_entry_t entry;

	// Search for the entry
	if (!FAT32_dir_get_entry(dir, name, &entry))
	{
		return 0;
	}

	// Delete it
	delete_entry(&entry);

	// Overwite the entry with zeroes
	memset(&entry, 0, sizeof(entry));
	FAT32_fwrite(&entry, sizeof(entry), 1, dir);
	return 1;
}
