/* REQUIRED - can edit/delete and add ur own description so that he can grade and know who did what.
** CSC 310 Final Project 
** Authors: Samantha Woodburn, Adelina Chocho, Reeya Patel, Megan Mohr 
** 
** Program to read in a QFS disk image and extract and a specified file from 
** the disk image, writing it to the current working directory. It should take 
** three comman line argument: name of the disk image, name of the file to be 
** extracted, and the name to call it in the current working directory. 
**
*/


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

int main(int argc, char *argv[]) {
	//correct num of arguments 
    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <disk image file> <file to read> <output file>\n",
                argv[0]);
        return 1;
    }

    const char *img_name   = argv[1];
    const char *qfs_fname  = argv[2];  // name inside QFS
    const char *out_name   = argv[3];  // name in current directory

//open disk image
    FILE *fp = fopen(img_name, "rb");
    if (!fp) {
        perror("fopen disk image");
        return 2;
    }

#ifdef DEBUG
    printf("Opened disk image: %s\n", img_name);
#endif

//read superblock - first 32 btes of disk image
    superblock_t sb;
    if (fread(&sb, sizeof(superblock_t), 1, fp) != 1) {
        fprintf(stderr, "Error: failed to read superblock\n");
        fclose(fp);
        return 3;
    }

#ifdef DEBUG
    printf("Superblock:\n");
    printf("  bytes_per_block = %u\n", sb.bytes_per_block);
    printf("  total_blocks    = %u\n", sb.total_blocks);
    printf("  total_direntries= %u\n", sb.total_direntries);
    printf("  label           = \"%s\"\n", sb.label);
#endif

//read all of the 255 enteries 
    direntry_t dir[255];
    if (fread(dir, sizeof(direntry_t), 255, fp) != 255) {
        fprintf(stderr, "Error: failed to read directory entries\n");
        fclose(fp);
        return 4;
    }

//search for requested filename
direntry_t *entry = NULL;
for (int i = 0; i < 255; i++) {
	//empty directory have filename[0]

    if (dir[i].filename[0] == '\0')
        continue;  // empty entry
        
//compare the directory file with rewuested file 
    if (strcmp(dir[i].filename, qfs_fname) == 0) {
        entry = &dir[i];
        break;
    }
}

    if (!entry) {
        fprintf(stderr,
                "Error: file \"%s\" not found in directory.\n", qfs_fname);
        fclose(fp);
        return 5;
    }

#ifdef DEBUG
    printf("Found file \"%s\" in directory.\n", qfs_fname);
    printf("  starting_block = %u\n", entry->starting_block);
    printf("  file_size      = %u bytes\n", entry->file_size);
    printf("  block_size     = %u bytes\n", sb.bytes_per_block);
#endif

// open the output file 
    FILE *out = fopen(out_name, "wb");
    if (!out) {
        perror("fopen output file");
        fclose(fp);
        return 6;
    }

#ifdef DEBUG
    printf("Opened output file: %s\n", out_name);
#endif

//calculate where the QSF begins 
    const long DATA_START =
        (long)sizeof(superblock_t) + 255L * (long)sizeof(direntry_t);

    
    const uint32_t DATA_BYTES_PER_BLOCK = sb.bytes_per_block - 3;
//checks
    if (sb.bytes_per_block > 2048) {
        fprintf(stderr, "Error: unexpected block size %u\n", sb.bytes_per_block);
        fclose(out);
        fclose(fp);
        return 7;
    }

    uint8_t buffer[2048];  // enough for max block size
    uint32_t bytes_left = entry->file_size;
    uint16_t current_block = entry->starting_block;

//follow the linked list of blocks
    while (bytes_left > 0) {
        if (current_block >= sb.total_blocks) {
            fprintf(stderr, "Error: block number %u out of range\n", current_block);
            break;
        }

//Compute byte offset of this block in the image
        long block_offset = DATA_START
                          + (long)current_block * (long)sb.bytes_per_block;

        if (fseek(fp, block_offset, SEEK_SET) != 0) {
            perror("fseek to block");
            break;
        }

//free or busy flag byte
        uint8_t is_free;
        if (fread(&is_free, 1, 1, fp) != 1) {
            fprintf(stderr, "Error: failed to read free/busy flag\n");
            break;
        }

#ifdef DEBUG
        printf("Reading block %u (is_free=%u, bytes_left=%u)\n",
               current_block, is_free, bytes_left);
#endif

// determine how mnay bytes to read from this block
        uint32_t this_block_bytes =
            (bytes_left < DATA_BYTES_PER_BLOCK) ? bytes_left : DATA_BYTES_PER_BLOCK;

        if (fread(buffer, 1, this_block_bytes, fp) != this_block_bytes) {
            fprintf(stderr, "Error: failed to read block payload\n");
            break;
        }

        if (fwrite(buffer, 1, this_block_bytes, out) != this_block_bytes) {
            fprintf(stderr, "Error: failed to write to output file\n");
            break;
        }

        bytes_left -= this_block_bytes;

//if finished copying - stop 
        if (bytes_left == 0) {
            break;
        }

//read the next block frmo the last bytes of block 
        uint16_t next_block;
        long next_offset = block_offset + sb.bytes_per_block - 2;

        if (fseek(fp, next_offset, SEEK_SET) != 0) {
            perror("fseek to next-block pointer");
            break;
        }

        if (fread(&next_block, sizeof(uint16_t), 1, fp) != 1) {
            fprintf(stderr, "Error: failed to read next-block pointer\n");
            break;
        }

#ifdef DEBUG
        printf("Next block = %u\n", next_block);
#endif

        current_block = next_block;
    }

    fclose(out);
    fclose(fp);

#ifdef DEBUG
    printf("Finished extracting \"%s\" to \"%s\".\n", qfs_fname, out_name);
#endif

    return 0;
}


