// FAT32Directory.c

#include <string.h>
#include <time.h>
#include "../include/FAT32Directory.h"

static void update_modification_datetime(struct FAT32_directory_entry_t* entry)
{
	// Get the current time and date
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	// Set date values
	entry->last_modified_date.year = tm.tm_year - 80;
	entry->last_modified_date.month = tm.tm_mon + 1;
	entry->last_modified_date.day = tm.tm_mday;

	// Set time values
	entry->last_modified_time.hours = tm.tm_hour;
	entry->last_modified_time.minutes = tm.tm_min;
	entry->last_modified_time.seconds = tm.tm_sec / 2;
}

static void update_access_date(struct FAT32_directory_entry_t* entry)
{
	// Get the current time and date
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	// Set date values
	entry->last_access_date.year = tm.tm_year - 80;
	entry->last_access_date.month = tm.tm_mon + 1;
	entry->last_access_date.day = tm.tm_mday;
}

FAT32_cluster_address_t FAT32_dir_get_entry_address(const struct FAT32_directory_entry_t* entry)
{
	FAT32_cluster_address_t result;
	result.index_high = entry->first_cluster_index_high;
	result.index_low = entry->first_cluster_index_low;

	return result;
}

void FAT32_dir_set_entry_address(struct FAT32_directory_entry_t* entry, FAT32_cluster_address_t address)
{
	entry->first_cluster_index_high = address.index_high;
	entry->first_cluster_index_low = address.index_low;
}

void FAT32_dir_get_entry_name(const struct FAT32_directory_entry_t* entry, char* outName)
{
	memset(outName, 0, FAT32_DIR_NAME_LEN);

	// Extract the name
	for (size_t i = 0; i < 8; ++i, ++outName)
	{
		if (entry->name[i] == ' ')
		{
			break;
		}

		*outName = entry->name[i];
	}

	// Extract the extension
	if (entry->ext[0] != ' ')
	{
		*outName = '.';
		++outName;
	}

	for (size_t i = 0; i < 3; ++i, ++outName)
	{
		if (entry->ext[i] == ' ')
		{
			break;
		}

		*outName = entry->ext[i];
	}
}

void FAT32_dir_set_entry_name(struct FAT32_directory_entry_t* entry, const char* name)
{
	memset(entry->name, ' ', 8);
	memset(entry->ext, ' ', 3);

	char* target = entry->name;
	for (; *name != 0; ++name)
	{
		if (*name == '.')
		{
			target = entry->ext;
		}
		else
		{
			*target = *name;
			++target;
		}
	}
}

int FAT32_dir_get_entry(struct FAT32_file_t* dir, const char* name, struct FAT32_directory_entry_t* outEntry)
{
    // Rewind the file
    FAT32_rewind(dir);

    // Loop until the entry is found
    while (FAT32_fread(outEntry, sizeof(struct FAT32_directory_entry_t), 1, dir))
    {
		// Get the name of the entry
		char entryName[FAT32_DIR_NAME_LEN];
		FAT32_dir_get_entry_name(outEntry, entryName);

        if (!strcmp(name, entryName))
        {
			// Rewind to the start of the entry
			FAT32_fseek(dir, -(long)sizeof(struct FAT32_directory_entry_t), FAT32_SEEK_CUR);
            return 1;
        }
    }

    return 0;
}

int FAT32_dir_get_entry_by_address(struct FAT32_file_t* dir, FAT32_cluster_address_t address, struct FAT32_directory_entry_t* outEntry)
{
	// Rewind the directory file
	FAT32_rewind(dir);

	// Loop until the tnry is found
	while (FAT32_fread(outEntry, sizeof(struct FAT32_directory_entry_t), 1, dir))
	{
		if (FAT32_dir_get_entry_address(outEntry).index == address.index)
		{
			return 1;
		}
	}

	return 0;
}

struct FAT32_file_t* FAT32_dir_open_entry(struct FAT32_directory_entry_t* entry)
{
    // Construct the address
	FAT32_cluster_address_t address = FAT32_dir_get_entry_address(entry);

	// If the entry is a subdirectory, open it with max size (directories are unsized)
	if (entry->attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY)
	{
		return FAT32_fopen(address, UINT32_MAX);
	}

	// Open the file, respecting its size
	update_access_date(entry);
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

	// If the file was modified, update the modification time
	if (FAT32_fmodified(file))
	{
		update_modification_datetime(entry);
	}

	// Close the file
	FAT32_fclose(file);
}

int FAT32_dir_new_entry(struct FAT32_file_t* dir, const char* name, FAT32_dir_entry_attribs_t attribs, struct FAT32_directory_entry_t* outEntry)
{
	// Seek to the beginning of the file
	FAT32_fseek(dir, 0, FAT32_SEEK_SET);

	// Loop until we either find an empty entry, or we run out of space
	long insertPos = 0;
	while (FAT32_fread(outEntry, sizeof(struct FAT32_directory_entry_t), 1, dir))
	{
		if (outEntry->name[0] == 0)
		{
			break;
		}

		insertPos = FAT32_ftell(dir);
	}

	// Set file properties
	FAT32_dir_set_entry_name(outEntry, name);
	outEntry->attribs = attribs;
	outEntry->size = 0;

	// Set initial datetime values
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	outEntry->create_date.year = tm.tm_year - 80;
	outEntry->create_date.month = tm.tm_mon + 1;
	outEntry->create_date.day = tm.tm_mday;
	outEntry->create_time.hours = tm.tm_hour;
	outEntry->create_time.minutes = tm.tm_min;
	outEntry->create_time.seconds = tm.tm_sec / 2;
	outEntry->last_modified_date = outEntry->create_date;
	outEntry->last_modified_time = outEntry->create_time;
	outEntry->last_access_date = outEntry->create_date;

	// Create a cluster chain for the file
	FAT32_dir_set_entry_address(outEntry, FAT32_new_cluster());

	// Rewind to where we'll insert the file
	FAT32_fseek(dir, insertPos, FAT32_SEEK_SET);

	// Write the entry
	FAT32_fwrite(outEntry, sizeof(struct FAT32_directory_entry_t), 1, dir);

	// Rewind again
	FAT32_fseek(dir, insertPos, FAT32_SEEK_SET);

	return 1;
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
			// If the subentry is not a real entry or a system entry
			if (subEntry.name[0] == 0 || subEntry.attribs & FAT32_DIR_ENTRY_ATTRIB_SYSTEM)
			{
				continue;
			}

			delete_entry(&subEntry);
		}

		FAT32_fclose(file);
	}

	// Free the cluster chain
	FAT32_free_cluster(FAT32_dir_get_entry_address(entry));
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

	// Make sure its not a system entry
	if (entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SYSTEM)
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

void FAT32_dir_clear_entry(struct FAT32_directory_entry_t* entry)
{
	// Delete the entry (not as bad as it sounds)
	delete_entry(entry);
	entry->size = 0;

	// Update the modification date
	update_modification_datetime(entry);

	// Reallocate the cluster chain
	FAT32_dir_set_entry_address(entry, FAT32_new_cluster());
}
