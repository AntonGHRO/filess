#include "metadata.h"

void _print_tabs(unsigned tab_count) {
    unsigned aux = tab_count;
    aux *= TAB_MUL;
    fputc(' ', stderr);
    while( aux -- ) fputs("- - ", stderr);
    fprintf(stderr, "\e[3m|%u|\e[0m ", tab_count);
}

void print_metadata(unsigned tab_count, metadata data, const char *color) {
    _print_tabs(tab_count);
    fprintf(stderr, "[Inode]\t\t%s%u%s\n", color, data.inode_number, RESET);
    _print_tabs(tab_count);
    fprintf(stderr, "[Size]\t\t%s%u%s\n", color, data.size, RESET);
    _print_tabs(tab_count);
    fprintf(stderr, "[Access Time]\t%s%s%s", color, ctime(&data.access_time), RESET);
    _print_tabs(tab_count);
    fprintf(stderr, "[Modify Time]\t%s%s%s", color, ctime(&data.modify_time), RESET);
    _print_tabs(tab_count);
    fprintf(stderr, "[Permissions]\t");

    fputs(color, stderr);
    fputc( data.owner_perms & __S_IREAD ? 'r' : '-', stderr);
    fputc( data.owner_perms & __S_IWRITE ? 'w' : '-', stderr);
    fputc( data.owner_perms & __S_IEXEC ? 'x' : '-', stderr);
    fputc(' ', stderr);

    fputc( data.group_perms & __S_IREAD ? 'r' : '-', stderr);
    fputc( data.group_perms & __S_IWRITE ? 'w' : '-', stderr);
    fputc( data.group_perms & __S_IEXEC ? 'x' : '-', stderr);
    fputc(' ', stderr);

    fputc( data.other_perms & __S_IREAD ? 'r' : '-', stderr);
    fputc( data.other_perms & __S_IWRITE ? 'w' : '-', stderr);
    fputc( data.other_perms & __S_IEXEC ? 'x' : '-', stderr);
    fputc('\n', stderr);
    fprintf(stderr, "%s", RESET);
}

void print_metadata_comp(unsigned tab_count, metadata old, metadata new) {
    // Assuming that they have the same inode number
    _print_tabs(tab_count);
    fprintf(stderr, "[Inode]\t\t%u\n", new.inode_number);
    _print_tabs(tab_count);
    fprintf(stderr, "[Size]\t\t%s%u%s\n", old.size != new.size ? GRN : RESET, new.size, RESET);
    _print_tabs(tab_count);
    fprintf(stderr, "[Access Time]\t%s%s%s", old.access_time != new.access_time ? GRN : RESET, ctime(&new.access_time), RESET);
    _print_tabs(tab_count);
    fprintf(stderr, "[Modify Time]\t%s%s%s", old.modify_time != new.modify_time ? GRN : RESET, ctime(&new.modify_time), RESET);
    _print_tabs(tab_count);
    fprintf(stderr, "[Permissions]\t");

    fprintf(stderr, "%s%c%s",
        (old.owner_perms & __S_IREAD) != (new.owner_perms & __S_IREAD) ? GRN : RESET,
        new.owner_perms & __S_IREAD ? 'r' : '-', RESET );
    fprintf(stderr, "%s%c%s",
        (old.owner_perms & __S_IWRITE) != (new.owner_perms & __S_IWRITE) ? GRN : RESET,
        new.owner_perms & __S_IWRITE ? 'w' : '-', RESET );
    fprintf(stderr, "%s%c%s ",
        (old.owner_perms & __S_IEXEC) != (new.owner_perms & __S_IEXEC) ? GRN : RESET,
        new.owner_perms & __S_IEXEC ? 'x' : '-', RESET );

    fprintf(stderr, "%s%c%s",
        (old.group_perms & __S_IREAD) != (new.group_perms & __S_IREAD) ? GRN : RESET,
        new.group_perms & __S_IREAD ? 'r' : '-', RESET );
    fprintf(stderr, "%s%c%s",
        (old.group_perms & __S_IWRITE) != (new.group_perms & __S_IWRITE) ? GRN : RESET,
        new.group_perms & __S_IWRITE ? 'w' : '-', RESET );
    fprintf(stderr, "%s%c%s ",
        (old.group_perms & __S_IEXEC) != (new.group_perms & __S_IEXEC) ? GRN : RESET,
        new.group_perms & __S_IEXEC ? 'x' : '-', RESET );

    fprintf(stderr, "%s%c%s",
        (old.other_perms & __S_IREAD) != (new.other_perms & __S_IREAD) ? GRN : RESET,
        new.other_perms & __S_IREAD ? 'r' : '-', RESET );
    fprintf(stderr, "%s%c%s",
        (old.other_perms & __S_IWRITE) != (new.other_perms & __S_IWRITE) ? GRN : RESET,
        new.other_perms & __S_IWRITE ? 'w' : '-', RESET );
    fprintf(stderr, "%s%c%s\n",
        (old.other_perms & __S_IEXEC) != (new.other_perms & __S_IEXEC) ? GRN : RESET,
        new.other_perms & __S_IEXEC ? 'x' : '-', RESET );
}
