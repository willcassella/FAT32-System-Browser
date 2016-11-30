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
	printf("exit - exit the program\n");
	printf("\n");
}

static void cmd_ls(struct FAT32_file_t* cwdir)
{
    // Seek to the beginning of the directory file (but ahead of the parent directory entry)
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
		for (size_t i = 0; i < 8; ++i)
		{
			if (entry.name[i] == ' ')
			{
				break;
			}

			putchar(entry.name[i]);
		}

		// Print the extension
		if (entry.ext[0] != ' ')
		{
			putchar('.');

			for (size_t i = 0; i < 3; ++i)
			{
				if (entry.ext[i] == ' ')
				{
					break;
				}

				putchar(entry.ext[i]);
			}
		}

		putchar('\n');
    }
}

static struct FAT32_file_t* cmd_cd(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Look for the entry in this directory
    if (!FAT32_dir_get_entry(cwdir, path, &entry))
    {
        printf("Error, %s is not a directory\n", path);
        return cwdir;
    }

    // Make sure the entry is a subdirectory
    if ((entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY) == 0)
    {
        printf("Error, %s is not a directory\n", path);
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
		printf("Error, %s is not an entry in this directory.", path);
		return;
    }

    // If the entry is a subdirectory
    if (entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY)
    {
        printf("Error, %s is a subdirectory and may not be opened for reading.", path);
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
    FAT32_fclose(file);
}

static void cmd_mkdir(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Create an entry in the current directory
    FAT32_dir_new_entry(cwdir, path, FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY, &entry);

    // Open the directory
    //struct FAT32_file_t* subdir = FAT32_dir_open_entry(&entry);

    // Create an entry for the parent
    //FAT32_cluster_address_t parentAddress = FAT32_faddress(cwdir);
 //   struct FAT32_directory_entry_t parentEntry;
	//parentEntry.name;
 //   parentEntry.attribs = FAT32_DIR_ENTRY_ATTRIB_HIDDEN | FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY;
 //   parentEntry.first_cluster_index_high = parentAddress.index_high;
 //   parentEntry.first_cluster_index_low = parentAddress.index_low;

 //   // Write it to the directory file
 //   FAT32_fwrite(&parentEntry, sizeof(FAT32_directory_entry_t), 1, subdir);

 //   // Close the file
 //   FAT32_fclose(subdir);
 //   return cwdir;
}

static void cmd_new(struct FAT32_file_t* cwdir, const char* path)
{
    // Check if the entry already exists
    struct FAT32_directory_entry_t entry;
	if (FAT32_dir_get_entry(cwdir, path, &entry))
	{
		printf("Error: %s already exists in this directory\n", path);
		return;
	}

	FAT32_dir_new_entry(cwdir, path, 0, &entry);
}

static void cmd_rm(struct FAT32_file_t* cwdir, const char* path)
{
    if (!FAT32_dir_remove_entry(cwdir, path))
    {
        printf("Error: %s does not name a directory entry\n", path);
    }
}

static void cmd_write(struct FAT32_file_t* cwdir, const char* path, const char* write)
{
	// Delete the file if it already exists
    struct FAT32_directory_entry_t entry;
    if(FAT32_dir_get_entry(cwdir, path, &entry))
    {
		FAT32_dir_remove_entry(cwdir, path);
    }

    FAT32_dir_new_entry(cwdir, path, 0, &entry);

	// Open it as a file
    struct FAT32_file_t* file = FAT32_dir_open_entry(&entry);
    FAT32_fwrite(write, 1, strlen(write), file);
	FAT32_dir_close_entry(&entry, file);

	// Save the entry
	FAT32_fwrite(&entry, sizeof(entry), 1, cwdir);
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
			char arg1[256];
            scanf("%s", arg0);
			fgets(arg1, 256, stdin);
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
            //cmd_stat(cwdir, arg0);
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
            printf("'%s' is not a recognized command.\n", cmd);
        }
    }

    // Close the current directory
    FAT32_fclose(cwdir);
}
