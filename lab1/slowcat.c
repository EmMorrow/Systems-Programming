// in command line ./slowcat filename
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
int main(int argc, char *argv[]) {
    FILE *fp;
    char line[20];
    int sleep_time = 1;
    char* slowcat_sleep_time = getenv("SLOWCAT_SLEEP_TIME");
    
    if (slowcat_sleep_time != NULL) {
        sleep_time = atoi(slowcat_sleep_time); 
    }

    if (argc > 1){
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            perror("Failed: ");
            return 1;
        }
    }

    else { // reading file from stdin 
        fp = stdin;
    }
    // Print the process ID of the current process to stderr (see the getpid() function).
    fprintf(stderr,"%d\n",getpid());

    while (fgets(line, sizeof(line), fp)) { 
        sleep(sleep_time); 
        fprintf(stdout, "%s", line);
    }
}