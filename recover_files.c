/* REQUIRED - can edit/delete and add ur own description so that he can grade and know who did what.
** CSC 310 Final Project 
** Authors: Samantha Woodburn, Adelina Chocho, Reeya Patel, Megan Mohr 
** 
** Program to read in a QFS disk image and recover all files provided with a disk 
** image that had jpg files on it, and has been subsequently formatted with QFS.
**
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "qfs.h"

/* Check if bytes at data[i], data[i+1] are JPEG start (FF D8). */
static int is_jpeg_start(const uint8_t *data) {
    return (data[0] == 0xFF && data[1] == 0xD8);
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filesystem_image>\n", argv[0]);
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

    /* read superblock-first 32 bytes */
    superblock_t sb;
    if (fread(&sb, sizeof(superblock_t), 1, fp) != 1) {
        fprintf(stderr, "Error: could not read superblock\n");
        fclose(fp);
        return 3;
    }

    /* Verify QFS magic number (0x51 = 'Q') */
    if (sb.fs_type != 0x51) {
        fprintf(stderr, "Error: not a QFS filesystem (fs_type = 0x%02X)\n",
                sb.fs_type);
        fclose(fp);
        return 4;
    }

#ifdef DEBUG
    printf("Superblock:\n");
    printf("  bytes_per_block   = %u\n", sb.bytes_per_block);
    printf("  total_blocks      = %u\n", sb.total_blocks);
    printf("  total_direntries  = %u\n", sb.total_direntries);
#endif

    const long DATA_START =
        (long)sizeof(superblock_t) + 255L * (long)sizeof(direntry_t);

    if (sb.bytes_per_block < 4 || sb.bytes_per_block > 2048) {
        fprintf(stderr, "Error: unexpected block size %u\n", sb.bytes_per_block);
        fclose(fp);
        return 5;
    }

    const uint16_t BYTES_PER_BLOCK = sb.bytes_per_block;
    const uint32_t DATA_BYTES      = BYTES_PER_BLOCK - 3;  /* skip is_busy + next_block */

    /* Buffer for the data region of a block (max 2045 bytes). */
    uint8_t buffer[2048];

    int recovered_count = 0;

    /* Scan each data block looking for JPEG starts  */
    for (uint16_t blk = 0; blk < sb.total_blocks; blk++) {

        /* Seek to the start of this block in the image. */
        long block_offset = DATA_START + (long)blk * (long)BYTES_PER_BLOCK;
        if (fseek(fp, block_offset, SEEK_SET) != 0) {
            perror("fseek block");
            break;
        }

        /* First byte is is_busy flag, which we IGNORE for recovery.*/
        uint8_t is_busy;
        if (fread(&is_busy, 1, 1, fp) != 1) {
            fprintf(stderr, "Error: could not read busy flag for block %u\n", blk);
            break;
        }

        /* Read the first two data bytes to check for JPEG header. */
        uint8_t first_two[2];
        if (fread(first_two, 1, 2, fp) != 2) {
            /* Reached EOF unexpectedly */
            break;
        }

        if (!is_jpeg_start(first_two)) {
            /* Not a JPEG start block; continue to next block. */
            continue;
        }

#ifdef DEBUG
        printf("Found JPEG start in block %u\n", blk);
#endif

        /* We have found the first block of a JPEG. Open output file. */
        char outname[64];
        snprintf(outname, sizeof(outname),
                 "recovered_file_%d.jpg", recovered_count++);

        FILE *out = fopen(outname, "wb");
        if (!out) {
            perror("fopen recovered_file");
            break;
        }

#ifdef DEBUG
        printf("Recovering to %s\n", outname);
#endif

        /* We will now follow the linked list of data blocks starting at blk,
         * copying bytes until we see the JPEG end marker FF D9. 
         */
        uint16_t current_block = blk;
        int done = 0;
        int last_byte = -1;  /* used to detect FF D9 across block boundaries */

        while (!done) {

            /* Seek to current block again to read full data & next pointer. */
            block_offset = DATA_START + (long)current_block * (long)BYTES_PER_BLOCK;
            if (fseek(fp, block_offset, SEEK_SET) != 0) {
                perror("fseek current_block");
                break;
            }

            /* Skip busy flag. */
            if (fread(&is_busy, 1, 1, fp) != 1) {
                fprintf(stderr, "Error: could not reread busy flag\n");
                break;
            }

            /* Read the data region of this block. */
            if (fread(buffer, 1, DATA_BYTES, fp) != DATA_BYTES) {
                fprintf(stderr, "Error: could not read data bytes\n");
                break;
            }

            /* Read the next_block pointer (2 bytes). */
            uint16_t next_block;
            if (fread(&next_block, sizeof(uint16_t), 1, fp) != 1) {
                fprintf(stderr, "Error: could not read next_block pointer\n");
                break;
            }

            /* Stream the data bytes into the output file until we see FF D9. */
            for (uint32_t i = 0; i < DATA_BYTES; i++) {
                uint8_t byte = buffer[i];

                /* Always write the byte just read. */
                if (fwrite(&byte, 1, 1, out) != 1) {
                    fprintf(stderr, "Error: failed to write to %s\n", outname);
                    done = 1;
                    break;
                }

                /* Check for JPEG end marker (FF D9) possibly across blocks. */
                if (last_byte == 0xFF && byte == 0xD9) {
                    done = 1;
                    break;
                }

                last_byte = byte;
            }

            if (done) {
                break;
            }

            /* Move to the next block in the chain. */
            if (next_block >= sb.total_blocks) {
#ifdef DEBUG
                printf("next_block %u out of range; stopping chain\n", next_block);
#endif
                break;
            }

            current_block = next_block;
        }

        fclose(out);
    }

    fclose(fp);

#ifdef DEBUG
    printf("Finished recovery. Total JPEGs recovered: %d\n", recovered_count);
#endif

    return 0;
}
