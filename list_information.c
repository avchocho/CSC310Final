/* REQUIRED - sam worked on this file- opening the input file and reading the disk, then printing the information of the superblock followed by the information about each directory entry
** CSC 310 Final Project 
** Authors: Samantha Woodburn, Adelina Chocho, Reeya Patel, Megan Mohr 
** 
** Program to read in a QFS disk image and list information about the disk 
** image and list information about the disk image from the superblock
** and its directory contents.
**
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk image file>\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 2;
    }

#ifdef DEBUG
    printf("Opened disk image: %s\n", argv[1]);
#endif

	// initialize and read in superblock
	struct superblock sb;
	if(fread(&sb, sizeof(struct superblock), 1, fp) != 1){
		fprintf(stderr, "Could not read superblock\n");
		fclose(fp);
		return 3;
	}
	
	// ensure correct number for file type
	if(sb.fs_type != 0x51){
		fprintf(stderr, "magic number is not 51");
		fclose(fp);
		return 4;
	}
	
	// print disk information for the given disk
	printf("QFS disk information for %s\n", argv[1]);
    // print block size
    printf("Block size: %u\n", sb.bytes_per_block);
    // print total number of blocks
    printf("Total number of blocks: %u\n", sb.total_blocks);
    // print number of free blocks
    printf("Total number of free blocks: %u\n", sb.available_blocks);
    // print number of directory entries
    printf("Total number of directories: %u\n", sb.total_direntries);
    // print number of free directory entries
    printf("Total number of free directories: %u\n", sb.available_direntries);
   
    // printing directory contents:
    printf("\nDirectory Contents\n");
	// struct for the directory
	struct direntry de;	
	int total_entries = sb.total_direntries;
	int free_entries = 0;
	// for each directory entry
	for(int i = 0; i < total_entries; i++){
		// error handeling
		if(fread(&de, sizeof(struct direntry),1, fp) != 1){
			fprintf(stderr, "Could not read directory entry %d\n",i);
			fclose(fp);
			return 5;
		}
		// if file name = \0 then the directory is free
		if(de.filename[0] == '\0'){
			free_entries++;
			continue;
		}
		//ensure file name is not null
		char file_name[24];
		memcpy(file_name, de.filename, 23);
		file_name[23] = '\0';
		// print file name, file size, andstarting block number
		printf("File: %-23s, Size: %10u bytes, Start Block: %5u\n", file_name, de.file_size, de.starting_block);
	}
    

    fclose(fp);
    return 0;
}
