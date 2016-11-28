// main.c

#include <stdio.h>
#include "../include/FAT32Directory.h"

struct FAT32_file_t* cmd_ls(struct FAT32_file_t* cwdir, const char* unused)
{
    // Seek to the beginning of the directory file (but ahead of the parent directory entry)
    FAT32_fseek(cwdir, FAT32_SEEK_SET, sizeof(FAT32_directory_entry_t));

    struct FAT32_directory_entry_t entry;
    while (FAT32_fread(&entry, sizeof(entry), 1, cwdir))
    {
        printf("%.8s.%.3s\n", entry.name, entry.ext);
    }

    return cwdir;
}

void cmd_help()
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
    printf("stat - print the stats of current file/directory\n");
    printf("help - print this menu\n")
    printf("exit - exit the program\n");
}

struct FAT32_file_t* cmd_cd(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Look for the entry in this directory
    if (!get_entry(cwdir, path, &entry))
    {
        printf("Error, %s is not a directory\n", path);
        return cwdir;
    }

    // Make sure the entry is a subdirectory
    if (entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY == 0)
    {
        printf("Error, %s is not a directory\n", path);
        return cwdir;
    }

    // Close the current directory
    FAT32_fclose(cwdir);

    // Open the subdirectory
    return open_entry(&entry);
}

struct FAT32_file_t* cmd_open(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Look for the entry in this directory
    if (!get_entry(cwdir, path, &entry))
    {
        printf("Error, %s is not an entry in this directory.")
        return cwdir;
    }

    // If the entry is a subdirectory
    if (entry.attribs & FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY)
    {
        printf("Error, %s is a subdirectory and may not be opened for reading.");
        return cwdir;
    }

    // Print all contents of the file to the screen
    struct FAT32_file_t* file = open_entry(&entry);
    char buff[256];
    while (FAT32_fread(buff, sizeof(char), 256, file))
    {
        printf("%.256", buff);
    }

    // Close the file
    FAT32_fclose(file);
    return cwdir;
}

struct FAT32_file_t* cmd_mkdir(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Create an entry in the current directory
    new_entry(cwdir, path, FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY, &entry);

    // Open the directory
    struct FAT32_file_t* subdir = open_entry(entry);

    // Create an entry for the parent
    struct FAT32_cluster_address_t parentAddress = FAT32_faddress(cwdir);
    struct FAT32_directory_entry_t parentEntry;
    parentEntry.name = "..";
    parentEntry.attribs = FAT32_DIR_ENTRY_ATTRIB_HIDDEN | FAT32_DIR_ENTRY_ATTRIB_SUBDIRECTORY;
    parentEntry.first_cluster_index_high = parentAddress.index_high;
    parentEntry.first_cluster_index_low = parentAddress.index_low;

    // Write it to the directory file
    FAT32_fwrite(&parentEntry, sizeof(FAT32_directory_entry_t), 1, subdir);

    // Close the file
    FAT32_fclose(subdir);
    return cwdir;
}

struct FAT32_file_t* cmd_new(struct FAT32_file_t* cwdir, const char* path)
{
    struct FAT32_directory_entry_t entry;

    // Create an entry
    new_entry(cwdir, path, 0, &entry);
    return cwdir;
}

struct FAT32_file_t* cmd_rm(struct FAT32_file_t* cwdir, const char* path)
{
    if (!remove_entry(cwdir, path))
    {
        printf("Error: %s does not name a directory entry\n", path);
    }

    return cwdir;
}

struct FAT32_file_t* cmd_write(struct FAT32_file_t* cwdir, const char* path, const char* write)
{
    struct FAT32_directory_entry_t entry;
    if(!FAT32_dir_get_entry(cwdir, path, &entry))
    {
        FAT32_new_entry(cwdir, path, 0, &entry);
    } 
    struct FAT32_file_t* file = FAT32_dir_open_entry(entry);   
    FAT32_fwrite(write, sizeof(char), strlen(write), file);
    FAT32_dir_close_entry(entry, file);
    return cwdir;
}

int main()
{
    char input[256];

    // Open the root directory
    struct FAT32_file_t* cwdir = FAT32_fopen(FAT32_get_root());
    cmd_help();

    while (1)
    {
        printf("$ ");
        scanf("%s", input);

        if (input[0] == 'l' && input[1] == 's')
        {
            cwdir = cmd_ls(cwdir, NULL);
        }
        else if (input[0] == 'c' && input[1] == 'd')
        {
            // Get directory argument
            scanf("%s", input);
            cwdir = cmd_ls(cwdir, input);
        }
        else if (input[0] == 'o' && input[1] == 'p' && input[2] == 'e' && input[3] == 'n')
        {
            // Get directory input
            scanf("%s", input);
            cwdir = cmd_open(cwdir, input);
        }
        else if(input[0] == 'n' && input[1] == 'e' && input [2] == 'w')
        {
            // Create new file/directory
            scanf("%s", input);
            cwdir = cmd_new(cwdir, input);
        }
        else if(input[0] == 'm' && input[1] == 'k' && input[2] == 'd' && input[3] == 'i' && input[4] == 'r')
        {
            // Make a new directory
            scanf("%s", input);
            cwdir - cmd_mkdir(cwdir, input);
        }
        else if(input[0] == 'w' && input[1] == 'r' && input[2] == 'i' && input[3] == 't' && input[4] == 'e')
        {
            // Write to a file
            char arg[16];
            scanf("%s", arg);
            scanf("%s", input);
            cwdir = cmd_write(cwdir, arg, input);
        }
        else if(input[0] == 'r' && input[1] == 'm')
        {
            // Remove a file/directory
            scanf("%s", input);
            cwdir = cmd_rm(cwdir, input);
        }
        else if(input[0]== 's' && input[1] == 't' && input[2] == 'a' && input[3] == 't')
        {
            // Print the stats of the current file/directory
            scanf("%s", input);
            //cwdir = cmd_stat(input);
        }
        else if(input[0]== 'h' && input[1] == 'e' && input[2] == 'l' && input[3] == 'p') 
        {
            // Print the help menu
            cmd_help();
        }
        else if (input[0] == 'e' && input[1] == 'x' && input[2] == 'i' && input[3] == 't')
        {
            // Exit the program
            return 0;
        }
        else
        {
            // Invalid command
            printf("'%s' is not a recognized command.\n", input);
        }
    }

    // Close the current directory
    FAT32_fclose(cwdir);
}