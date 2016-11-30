// main.c

#include <stdio.h>
#include <string.h>
#include "../include/FAT32Directory.h"

static void cmd_help(void)
{
	printf("FAT32 File Explorer/Reader\n");
	printf("Enter one of the following valid commands:\n");
	printf("ls - list of files/directories\n");
	printf("cd - change directory\n");
	printf("open - open a file\n");
	printf("new - add new file\n");
	printf("mkdir - create a new directory\n");
	printf("write - write to a file\n");
	printf("rm - remove a file/directory\n");
	printf("stat - print the stats of file/directory\n");
	printf("help - print this menu\n");
	printf("disk - print a visualization of the state of the disk\n");
	printf("exit - exit the program\n");
	printf("\n");
}

static void cmd_ls(struct FAT32_file_t* cwdir)
{
    // Seek to the beginning of the directory file
    FAT32_fseek(cwdir, 0, FAT32_SEEK_SET);

    struct FAT32_directory_entry_t entry;
    while (FAT32_fread(&entry, sizeof(entry), 1, cwdir))
    {
		// Skip deleted entries
		if (entry.name[0] == 0)
		{
			continue;
		}

		// Print the name
		char name[FAT32_DIR_NAME_LEN];
		FAT32_dir_get_entry_name(&entry, name);
		printf("%s\n", name);
    }
}

static struct FAT32_file_t* cmd_cd(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Look for the entry in this directory
    if (!FAT32_dir_get_entry(cwdir, path, &entry))
    {
        printf("Error, '%s' is not a directory\n", path);
        return cwdir;
    }

    // Make sure the entry is a subdirectory
    if ((entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY) == 0)
    {
        printf("Error, '%s' is not a directory\n", path);
        return cwdir;
    }

    // Close the current directory
    FAT32_fclose(cwdir);

    // Open the subdirectory
    return FAT32_dir_open_entry(&entry);
}

static void cmd_open(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Look for the entry in this directory
    if (!FAT32_dir_get_entry(cwdir, path, &entry))
    {
		printf("Error, '%s' is not an entry in this directory\n", path);
		return;
    }

    // If the entry is a subdirectory
    if (entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY)
    {
        printf("Error, '%s' is a subdirectory and may not be opened for reading\n", path);
		return;
    }

    // Print all contents of the file to the screen
    struct FAT32_file_t* file = FAT32_dir_open_entry(&entry);
    char c;
    while (FAT32_fread(&c, 1, 1, file))
    {
		putchar(c);
    }

	printf("\n");

    // Close the file
	FAT32_dir_close_entry(&entry, file);

	// Write entry changes
	FAT32_fwrite(&entry, sizeof(entry), 1, cwdir);
}

static void cmd_mkdir(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Create an entry in the current directory
    FAT32_dir_new_entry(cwdir, path, FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY, &entry);

    // Open the directory
    struct FAT32_file_t* subdir = FAT32_dir_open_entry(&entry);

    // Create an entry for the parent
    struct FAT32_directory_entry_t parentEntry;
	FAT32_dir_set_entry_name(&parentEntry, "");
	FAT32_dir_set_entry_address(&parentEntry, FAT32_faddress(cwdir));
    parentEntry.attribs = FAT32_DIR_ENTRY_ATTRIB_SYSTEM | FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY;
	parentEntry.name[0] = '.';
	parentEntry.name[1] = '.';

    // Write it to the directory file
    FAT32_fwrite(&parentEntry, sizeof(parentEntry), 1, subdir);

    // Close the file
    FAT32_fclose(subdir);
}

static void cmd_new(struct FAT32_file_t* cwdir, const char* path)
{
    // Check if the entry already exists
    struct FAT32_directory_entry_t entry;
	if (FAT32_dir_get_entry(cwdir, path, &entry))
	{
		printf("Error: '%s' already exists in this directory\n", path);
		return;
	}

	FAT32_dir_new_entry(cwdir, path, 0, &entry);
}

static void cmd_rm(struct FAT32_file_t* cwdir, const char* path)
{
    if (!FAT32_dir_remove_entry(cwdir, path))
    {
        printf("Error: '%s' does not name a directory entry\n", path);
    }
}

static void cmd_write(struct FAT32_file_t* cwdir, const char* path, const char* write)
{
	// See if the file already exists
    struct FAT32_directory_entry_t entry;
    if(FAT32_dir_get_entry(cwdir, path, &entry))
    {
		// Make sure the file isn't a folder
		if (entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY)
		{
			printf("Error: '%s' is a directory, and may not be written to\n", path);
			return;
		}

		// Clear the existing contents of the file
		FAT32_dir_clear_entry(&entry);
    }
	else
	{
		// Create a new file
		FAT32_dir_new_entry(cwdir, path, 0, &entry);
	}

	// Open the entry
    struct FAT32_file_t* file = FAT32_dir_open_entry(&entry);
    FAT32_fwrite(write, 1, strlen(write), file);
	FAT32_dir_close_entry(&entry, file);

	// Save the entry
	FAT32_fwrite(&entry, sizeof(entry), 1, cwdir);
}

static void cmd_stat(struct FAT32_file_t* cwdir, const char* path)
{
	// Get the entry
	struct FAT32_directory_entry_t entry;
	if (!FAT32_dir_get_entry(cwdir, path, &entry))
	{
		printf("Error: '%s' does not name a directory entry\n", path);
		return;
	}

	// Make sure the file isn't a directory
	if (entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY)
	{
		printf("Error: 'stat' is not supported for directories\n");
		return;
	}

	// Get the file's name
	char name[FAT32_DIR_NAME_LEN];
	FAT32_dir_get_entry_name(&entry, name);

	printf("Name: '%s'\n", name);
	printf("Size: %u\n", entry.size);

	printf("Created: %u/%u/%u %u:%u:%u\n", entry.create_date.month, entry.create_date.day, entry.create_date.year + 1980,
		entry.create_time.hours, entry.create_time.minutes, entry.create_time.seconds * 2);

	printf("Last modification: %u/%u/%u %u:%u:%u\n", entry.last_modified_date.month, entry.last_modified_date.day, entry.last_modified_date.year + 1980,
		entry.last_modified_time.hours, entry.last_modified_time.minutes, entry.last_modified_time.seconds * 2);

	printf("Last access: %u/%u/%u\n", entry.last_access_date.month, entry.last_access_date.day, entry.last_access_date.year + 1980);
}

static void print_tree_trace(struct FAT32_file_t* cwdir)
{
	// Get the parent directory
	struct FAT32_directory_entry_t parentEntry;
	if (!FAT32_dir_get_entry(cwdir, "..", &parentEntry))
	{
		// This is the root
		printf("/");
		return;
	}

	// Open the parent directory
	struct FAT32_file_t* parentDir = FAT32_dir_open_entry(&parentEntry);
	print_tree_trace(parentDir);

	// Find this directory in the parent
	struct FAT32_directory_entry_t myEntry;
	FAT32_dir_get_entry_by_address(parentDir, FAT32_faddress(cwdir), &myEntry);

	// Print current name
	char myName[FAT32_DIR_NAME_LEN];
	FAT32_dir_get_entry_name(&myEntry, myName);
	printf("%s/", myName);

	// Close the parent file
	FAT32_fclose(parentDir);
}

int main()
{
    // Open the root directory
	FAT32_init();
    struct FAT32_file_t* cwdir = FAT32_fopen(FAT32_get_root(), 0);
    cmd_help();

    while (1)
    {
		char cmd[16];
		print_tree_trace(cwdir);
        printf("$ ");
        scanf("%s", cmd);

        if (!strcmp(cmd, "ls"))
        {
            cmd_ls(cwdir);
        }
        else if (!strcmp(cmd, "cd"))
        {
            // Get directory argument
			char arg0[16];
            scanf("%s", arg0);
            cwdir = cmd_cd(cwdir, arg0);
        }
        else if (!strcmp(cmd, "open"))
        {
            // Get directory input
            scanf("%s", cmd);
			cmd_open(cwdir, cmd);
        }
        else if (!strcmp(cmd, "new"))
        {
            // Create new file
			char arg0[16];
            scanf("%s", arg0);
            cmd_new(cwdir, arg0);
        }
        else if (!strcmp(cmd, "mkdir"))
        {
            // Make a new directory
			char arg0[16];
            scanf("%s", arg0);
            cmd_mkdir(cwdir, arg0);
        }
        else if (!strcmp(cmd, "write"))
        {
            // Write to a file
            char arg0[16];
			char arg1[1024];
            scanf("%s", arg0);
			fgets(arg1, 1024, stdin);

			// Remove the traling newline character
			arg1[strcspn(arg1, "\n")] = 0;

			// Write
            cmd_write(cwdir, arg0, arg1);
        }
        else if (!strcmp(cmd, "rm"))
        {
            // Remove a file/directory
			char arg0[16];
            scanf("%s", arg0);
            cmd_rm(cwdir, arg0);
        }
        else if (!strcmp(cmd, "stat"))
        {
            // Print the stats of the current file/directory
			char arg0[16];
            scanf("%s", arg0);
            cmd_stat(cwdir, arg0);
        }
		else if (!strcmp(cmd, "disk"))
		{
			// Print the state of the disk
			FAT32_print_disk();
		}
        else if (!strcmp(cmd, "help"))
        {
            // Print the help menu
            cmd_help();
        }
        else if (!strcmp(cmd, "exit"))
        {
            // Exit the program
            break;
        }
        else
        {
            // Invalid command
            printf("'%s' is not a recognized command\n", cmd);
        }
    }

    // Close the current directory
    FAT32_fclose(cwdir);
}
