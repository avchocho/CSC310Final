/* OPTIONAL - can edit/delete and add ur own description so that he can grade and know who did what.
** CSC 310 Final Project 
** Authors: Samantha Woodburn, Adelina Chocho, Reeya Patel, Megan Mohr 
** 
** Program to read in a QFS disk image and delete a specified file from the disk image.
** It should take two command line arguements, one for the name of the disk image and one
** for the the file to be deleted.
**
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <disk image file> <file to remove>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb+");
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
