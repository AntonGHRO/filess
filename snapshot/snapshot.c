#include "snapshot.h"

snapshot create_snapshot(const char *name) {
    // Open dir
    DIR *dir = opendir(name);
    snapshot sn = { 0 };

    if(dir == NULL) {
        perror("%s ");
        return sn;
    }

    // Get dir stat via fd
    int fd = dirfd(dir);
    struct stat st;
    fstat(fd, &st);

    sn.dir_data.inode_number = st.st_ino;
    sn.dir_data.size = st.st_size;
    sn.dir_data.access_time = st.st_atime;
    sn.dir_data.modify_time = st.st_mtime;
    sn.dir_data.owner_perms = st.st_mode & S_IRWXU;
    sn.dir_data.group_perms = st.st_mode & S_IRWXG;
    sn.dir_data.other_perms = st.st_mode & S_IRWXO;

    sn.entries_count = 0;

    struct dirent entry;
    struct dirent *aux;
    char path[PATH_MAX];
    struct stat estat;

    while( (aux = readdir(dir)) != NULL ) {
        entry = *aux;
        snprintf(path, PATH_MAX, "%s/%s", name, entry.d_name);
        stat(path, &estat);

        if(entry.d_name[0] == '.') continue;
        if( !( (estat.st_mode & __S_IFMT) & (__S_IFDIR | __S_IFREG) ) ) continue; // handle recursively

        sn.entry_data[sn.entries_count].inode_number = estat.st_ino;
        strncpy(sn.entry_name[sn.entries_count], entry.d_name, 64);
        sn.entry_data[sn.entries_count].size = estat.st_size;
        sn.entry_data[sn.entries_count].access_time = estat.st_atime;
        sn.entry_data[sn.entries_count].modify_time = estat.st_mtime;
        sn.entry_type[sn.entries_count] = estat.st_mode & __S_IFMT;
        sn.entry_data[sn.entries_count].owner_perms = estat.st_mode & S_IRWXU;
        sn.entry_data[sn.entries_count].group_perms = estat.st_mode & S_IRWXG;
        sn.entry_data[sn.entries_count].other_perms = estat.st_mode & S_IRWXO;

        //if((sn.entry_type[sn.entries_count] & __S_IFMT) & __S_IFDIR)
            //create_snapshot(path);

        sn.entries_count ++;
    }

    snprintf(path, PATH_MAX, "%s%s", name, "/.snapshot");
    int snfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if( write(snfd, &sn, sizeof(snapshot)) != sizeof(snapshot)) {
        // problem
    }
    else close(snfd);

    closedir(dir);
    return sn;
}

snapshot read_snapshot(const char *name) {
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/.snapshot", name); // name/.snapshot
    int fd = open(path, O_RDONLY);

    if(fd == -1) fprintf(stderr, "Could not open path: <%s>.\n", path);

    snapshot sn;

    if( read(fd, &sn, sizeof(snapshot)) != sizeof(snapshot) ) {
        // problem
    }

    close(fd);
    return sn;
}

void print_snapshot(snapshot sn) {
    fprintf(stderr, "Directory Snapshot\n");
    print_metadata(0, sn.dir_data, RESET);

    fprintf(stderr, "[Entries %u]\n\n", sn.entries_count);

    for(int i = 0; i < sn.entries_count; i ++) {
        fprintf(stderr, "%u. %s %s\n", i + 1, (sn.entry_type[i] & __S_IFMT) & __S_IFREG ? "REGFILE" : "DIRECTORY", sn.entry_name[i]);

        print_metadata(0, sn.entry_data[i], RESET);
        fputc('\n', stderr);
    }
}

// Print new, but highlight the differences
void print_snapshot_comp(unsigned tab_count, snapshot old, snapshot new, const char *dirname) {
    // Assume that they have the same inode number
    fputc('\n', stderr);
    _print_tabs(tab_count);
    fprintf(stderr, "\e[7m\e[5m\e[3mEvaluating for %s%s%s\e[0m\e[25m\e[27m\n\n", CYN, !strcmp(dirname, ".") ? "Current Directory" : dirname, RESET);

    print_metadata_comp(tab_count, old.dir_data, new.dir_data);

    fputc('\n', stderr);
    _print_tabs(tab_count);
    fprintf(stderr, "[Entries %s%u%s]\n\n", old.entries_count != new.entries_count ? GRN : RESET, new.entries_count, RESET);

    int i, j;
    unsigned missing[MAX_ENTRIES] = { 0 };

    for(i = 0; i < new.entries_count; i ++) {
        for(j = 0; j < old.entries_count; j ++) {
            if(old.entry_data[j].inode_number == new.entry_data[i].inode_number) { // Searches the new's in old's
                _print_tabs(tab_count);
                fprintf(stderr, "[STILL]\t\t%u. %s %s%s%s\n",
                    i + 1, (new.entry_type[i] & __S_IFMT) & __S_IFREG ? "REGFILE" : "DIRECTORY",
                    strcmp(old.entry_name[j], new.entry_name[i]) != 0 ? GRN : RESET, new.entry_name[i], RESET);

                print_metadata_comp(tab_count, old.entry_data[j], new.entry_data[i]);

                // DELETE START
                if((new.entry_type[i] & __S_IFMT) & __S_IFDIR) {
                    char path[PATH_MAX];
                    sprintf(path, "%s/%s", dirname, new.entry_name[i]);

                    snapshot snaux_old = read_snapshot(path);
                    snapshot snaux_new = create_snapshot(path);

                    // Already uploaded the new snapshot when you created the parent directory snapshot.. meaning that it will never detect any changes
                    print_snapshot_comp(tab_count + 1, snaux_old, snaux_new, path);
                }
                // DELETE END

                fputc('\n', stderr);
                
                missing[j] = 1;
                
                break;
            }
        }
        // New files
        if(j == old.entries_count) {
            _print_tabs(tab_count);
            fprintf(
                stderr, "%s[NEW]%s\t\t%u. %s %s%s%s\n", BLU, RESET, i + 1,
                (new.entry_type[i] & __S_IFMT) & __S_IFREG ? "REGFILE" : "DIRECTORY",
                BLU, new.entry_name[i], RESET
            );

            print_metadata(tab_count, new.entry_data[i], BLU);
            fputc('\n', stderr);
        }
    }

    // Missing: files were deleted
    for(j = 0; j < old.entries_count; j ++) {
        if(missing[j] == 0) {
            _print_tabs(tab_count);
            fprintf(
                stderr, "[%sDELETED%s]\tX. %s %s%s%s\n", RED, RESET,
                (old.entry_type[j] & __S_IFMT) & __S_IFREG ? "REGFILE" : "DIRECTORY",
                RED, old.entry_name[j], RESET
            );

            print_metadata(tab_count, old.entry_data[j], RED);
            fputc('\n', stderr);
        }
    }
}
