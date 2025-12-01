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

    // TODO

    fclose(fp);
    return 0;
}