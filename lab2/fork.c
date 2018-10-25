#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
void exec(int argc, char *argv[]) {
	char *newenviron[] = { NULL };

	printf("Program \"%s\" has pid %d. Sleeping.\n", argv[0], getpid());
	sleep(1);

	if (argc <= 1) {
		printf("No program to exec.  Exiting...\n");
		exit(0);
	}

	printf("Running exec of \"%s\"\n", argv[1]);
	execve(argv[1], &argv[1], newenviron);
	printf("End of program \"%s\".\n", argv[0]);
}

int main(int argc, char *argv[]) {
        int pid;
        FILE * fp;
        FILE * fpin = {0};
        FILE * fpout = {0};
        char str[60];
        int descriptor;

        int fd[2] = {0};
        pipe(fd);
        printf("Starting program; process has pid %d\n", getpid());

        fp = fopen("fork-output.txt","w");
        fprintf(fp,"%s","BEFORE FORK\n");

        if ((pid = fork()) < 0) {
                fprintf(stderr, "Could not fork()");
                exit(1);
        }

        /* BEGIN SECTION A */
        fprintf(fp,"%s","SECTION A\n");
        printf("Section A;  pid %d\n", getpid());
        //sleep(30);

        /* END SECTION A */
        if (pid == 0) {
                /* BEGIN SECTION B */
                close(fd[0]);
                fpout = fdopen(fd[1],"w");
				if(fpout == NULL) {
   					fprintf(stderr, "Error - fdopen(child)\n");
   					return 1;
  				}
                fprintf(fpout,"%s","hello from section b\n");
                fprintf(fp,"%s","SECTION B\n");
                printf("Section B\n");
                //sleep(30);
                //sleep(30);
                printf("Section B done sleeping\n");
		fclose(fpout);
                descriptor = fileno(fp);
                descriptor = dup2(descriptor,descriptor);
                fp = fdopen(descriptor,"w");
		exec(argc, argv);
                exit(0);

                /* END SECTION B */
        } else {
                /* BEGIN SECTION C */
                //wait(NULL);
                // fclose(fpout);
		close(fd[1]);
                fpin = fdopen(fd[0],"r");
                if (fpin == NULL) {
                        fprintf(stderr, "Error - fdopen(parent)\n");
                        return 1;
                }
                if(fgets(str,60,fpin) != NULL) {
                        printf("%s",str);
                }

                fprintf(fp,"%s","SECTION C\n");
                printf("Section C\n");
                //sleep(30);
                printf("Section C done sleeping\n");
                fclose(fpin);
                exit(0);

                /* END SECTION C */
        }
        /* BEGIN SECTION D */
        fprintf(fp,"%s","SECTION D\n");
        printf("Section D\n");
        //sleep(30);

        /* END SECTION D */
}