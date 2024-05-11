#ifndef MD_H
#define MD_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <linux/limits.h>
#include <string.h>
#include <time.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define TAB_MUL 4

typedef struct metadata {
    unsigned inode_number;
    unsigned size;
    time_t access_time;
    time_t modify_time;
    unsigned owner_perms;
    unsigned group_perms;
    unsigned other_perms;
} metadata;

void print_metadata(unsigned tab_count, metadata data, const char *color);
void print_metadata_comp(unsigned tab_count, metadata old, metadata new);

// Not for user
void _print_tabs(unsigned tab_count);

#endif
