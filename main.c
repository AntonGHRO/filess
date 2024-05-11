#include "snapshot/snapshot.h"
#include <sys/wait.h>
#include <errno.h>

snapshot old[10];
snapshot new;
unsigned susFilesCount;
pid_t pid[10];
pid_t pid_returned;
int status;
int i, j;
int iOutput;
int iSafe;
int fd[10][2];

unsigned noPermsDetection(char **argv) {
    unsigned susFilesCountChild = 0;

    // No perms detection for regular files
    for(j = 0; j < new.entries_count; j ++) {
        if(new.entry_type[j] == __S_IFREG && !new.entry_data[j].owner_perms && !new.entry_data[j].group_perms && !new.entry_data[j].owner_perms) {
            fprintf(stderr, "Detected possible threat from file <%s>.\n", new.entry_name[j]);
            susFilesCountChild ++;

            // Creating pipe to pass the strings from stdout from the shell to the main process
            int pfd[2];
            if(pipe(pfd) < 0)
                fprintf(stderr, "Failed to create pipe for child for shell script for <%s>.\n", new.entry_name[j]);

            // Creating child process that will get replaced by exec
            int child = fork();

            // Child code
            if(child == 0) {
                close(pfd[0]); // Close reading end in child
                dup2(pfd[1], STDOUT_FILENO); // Redirect writing end to stdout
                close(pfd[1]);

                chmod(new.entry_name[j], 0777);
                
                if(execlp("bash", "bash", "./verify_for_malicious.sh", new.entry_name[j], NULL) == -1)
                    exit(-1);
            } else {
                int status; // -1 failed to exec, 1 safe, 0 not safe
                wait(&status);

                // Reading from script
                char received[BUFSIZ] = {0};
                close(pfd[1]); // Close writing end in parent

                if(read(pfd[0], received, BUFSIZ) == -1)
                    fprintf(stderr, "Failed to read from shell script for <%s>.\n", new.entry_name[j]);

                fprintf(stderr, "Received from shell script for <%s>: %s", new.entry_name[j], received);

                close(pfd[0]); // Close reading end in parent

                if (WIFSIGNALED(status)) {
                    fprintf(stderr, "Malware analysis script for <%s> terminated by signal: %d\n", new.entry_name[j], WTERMSIG(status));
                } else {
                    if(WEXITSTATUS(status) == 255)
                        fprintf(stderr, "Failed to run malware analysis script for file <%s>.\n", new.entry_name[j]);
                    else {
                        if(WIFEXITED(status)) {
                            // If not safe, move in safe directory
                            if(WEXITSTATUS(status) == 1) {
                                char new_path[PATH_MAX];
                                char old_path[PATH_MAX];
                                sprintf(new_path, "%s/%s", argv[iSafe], new.entry_name[j]);
                                sprintf(old_path, "%s/%s", argv[i], new.entry_name[j]);
                                fprintf(stderr, "File <%s> is not safe. Moving it to safe directory <%s>. New path: <%s>.\n", old_path, argv[iSafe], new_path);
                                if(rename(old_path, new_path) == -1)
                                    fprintf(stderr, "Failed to move file <%s> to safe directory <%s>.\n", old_path, argv[iSafe]);
                                chmod(new_path, 0000);
                            // If safe, do nothing
                            } else fprintf(stderr, "File <%s> is safe.\n", new.entry_name[j]);
                        }
                        // Abnormal termination from child
                        else fprintf(stderr, "Malware analysis script for <%s> terminated abnormally.\n", new.entry_name[j]);
                    }
                }
            }

            chmod(new.entry_name[j], 0000);
        }
    }

    return susFilesCountChild;
}

int main(int argc, char **argv) {
    // No arguments passed -> will print modifications of the current directory
    if(argc == 1)
    {
        old[0] = read_snapshot(".");
        new = create_snapshot(".");

        print_snapshot_comp(0, old[0], new, ".");
    // Otherwise -> will traverse the arguments and process them
    } else if (argc <= 13) // 13 Allowing for program name (1), -o output directory flag (2 with parameter) and 10 aditional directories
    {
        iOutput = -1;
        iSafe = -1;

        for(i = 1; i < argc; i ++) {
            if( strncmp(argv[i], "-o", strlen(argv[i])) == 0 ) {
                iOutput = ++ i;

                struct stat st = {0};
                if(stat(argv[iOutput], &st) == -1)
                    if(mkdir(argv[iOutput], 0777) != 0)
                        fprintf(stderr, "Failed to set output directory to <%s>.\n", argv[iOutput]);
                fprintf(stderr, "Succesfully set output directory to: %s\n", argv[iOutput]);
                
                continue;
            } else if(strncmp(argv[i], "-s", strlen(argv[i])) == 0) {
                iSafe = ++ i;

                struct stat st = {0};
                if(stat(argv[iSafe], &st) == -1)
                    if(mkdir(argv[iSafe], 0777) != 0)
                        fprintf(stderr, "Failed to set safe directory to <%s>.\n", argv[iSafe]);
                fprintf(stderr, "Succesfully set safe directory to: %s\n", argv[iSafe]);

                continue;
            }
            // Reading the old snapshots before overwriting them
            else old[i - 1] = read_snapshot(argv[i]);
        }

        // Traversing arguments and processing them in child processes
        for(i = 1; i < argc; i ++) {
            if(i == iOutput || i == iOutput - 1 || i == iSafe || i == iSafe - 1) continue;

            // Creating pipe for process i
            if(pipe(fd[i - 1]) != 0) {
                fprintf(stderr, "Pipe failed to create %d.\n", errno);
                exit(EXIT_FAILURE);
            }

            if( (pid[i - 1] = fork()) == -1 ) {
                fprintf(stderr, "Process number %d failed to create.\n", i);
                continue;
            }

            // Parent process
            if(pid[i - 1] != 0)
                fprintf(stderr, "Child with pid %d was created for argument <%s>.\n", pid[i - 1], argv[i]);
            // Child process
            else {
                // Create snapshot
                new = create_snapshot(argv[i]);
                if(new.entries_count >= 0) fprintf(stderr, "Snapshot for directory <%s> created succesfully.\n", argv[i]);
                else fprintf(stderr, "Snapshot for directory <%s> failed to create.\n", argv[i]);

                // Checking entries
                unsigned susFilesCountChild = noPermsDetection(argv);

                // Placing the snapshots in the output directory
                if(iOutput != -1) {
                    char path[PATH_MAX];
                    
                    sprintf(path, "%s/.snapshot_%s", argv[iOutput], argv[i]);
                    
                    int sn_file = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    
                    if(sn_file == -1)
                        fprintf(stderr, "Failed to create snapshot file in <%s> directory for <%s>\n", argv[iOutput], path);

                    if(write(sn_file, &new, sizeof(snapshot)) == -1)
                        fprintf(stderr, "Failed to write snapshot <%s> in output directory <%s>.\n", path, argv[iOutput]);
                    else fprintf(stderr, "Wrote snapshot <%s> in output directory <%s>.\n", path, argv[iOutput]);
                }

                // Sending susFilesCountChild through pipe
                
                // Closing reading end for child
                close(fd[i - 1][0]); // Close reading in child
                if(write(fd[i - 1][1], &susFilesCountChild, sizeof(unsigned)) == -1)
                    fprintf(stderr, "Child process for <%s> failed to write to parent process.\n", argv[i]);
                close(fd[i - 1][1]); // Close reading in child

                // Exit child
                exit(EXIT_SUCCESS);
            }
        }

        // Parent waiting for children to get back from school
        for(i = 1; i < argc; i ++) {
            if(i == iOutput || i == iOutput - 1 || i == iSafe || i == iSafe - 1) continue;

            pid_returned = wait(&status);

            int j;
            for(j = 0; j < 10; j ++)
                if(pid_returned == pid[j])
                    break;

            close(fd[j][1]); // Close writing in parent
            if(read(fd[j][0], &susFilesCount, sizeof(unsigned)) == -1)
                fprintf(stderr, "Failed to read from child process %d.\n", pid_returned);
            close(fd[j][0]); // Close reading in parent

            fprintf(stderr, "Child pid %d returned with status %d. Suspicious files count: %d\n", pid_returned, status, susFilesCount);

            if(iOutput != -1) {
                char path[PATH_MAX];
                sprintf(path, "%s/.snapshot_%s", argv[iOutput], argv[i]);
                int sn_file = open(path, O_RDONLY);

                if(sn_file == -1)
                        fprintf(stderr, "Failed to open snapshot file in <%s> directory for <%s>\n", argv[iOutput], path);

                if(read(sn_file, &new, sizeof(snapshot)) == -1)
                    fprintf(stderr, "Failed to read snapshot <%s> in output directory <%s>.\n", path, argv[iOutput]);
                else print_snapshot_comp(0, old[i - 1], new, argv[i]);
            }
        }
    } else
        fprintf(stderr, "Number of arguments should be between 1 and 13.\n");

    return 0;
}
