#ifndef SN_H
#define SN_H

#include "metadata.h"

#define MAX_ENTRIES 1024

typedef struct snapshot {
    metadata dir_data;

    int entries_count;

    metadata entry_data[MAX_ENTRIES];
    char entry_name[MAX_ENTRIES][64];
    unsigned entry_type[MAX_ENTRIES];
} snapshot;

snapshot create_snapshot(const char *name);
snapshot read_snapshot(const char *name);

void print_snapshot(snapshot sn);
void print_snapshot_comp(unsigned tab_count, snapshot old, snapshot new, const char *dirname);

#endif
