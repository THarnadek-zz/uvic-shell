#include  <stdio.h>
#include <stdlib.h>
#include  <sys/types.h>
#include <errno.h>     //required for stderr 
#include <string.h>
//#include <unistd.h>  //Uncomment if your on GCC5

#define INPUT_BUFFER 80
#define MAX_ARGS 9
#define MAX_PROMPT_LEN 10
#define MAX_DIRS 10

void parse(char* parseme, const char* delim, char** output) {
    char* token;
    int i;
    for(i = 0; (token = strsep(&parseme, delim)); i++) {        
        token[strcspn(token, "\n")] = 0;
        output[i] = token;
    }

    //Make sure array is null terminated 
    //for execpe
    output[i] = NULL;
}


int  execute_cmd(char *argv[])
{
     pid_t  pid;
     int    status;

     //We need to get the full path
     char fullpath[INPUT_BUFFER];
     
     //For your assignment, you need load the
     //path(s) from .uvshrc
     char* path = "/bin/";
     
     //Full path to my executable
     strcat(fullpath, path);
     strcat(fullpath, argv[0]);
     argv[0] = fullpath;

     //Empty envp variable
     char* const envp[] = {NULL};
       
     //Just some test logging
     int i;
     for(i = 0; argv[i] != NULL; ++i) 
        printf("LOG: %s\n", argv[i]);
     

     // Fork a child process 
     if ((pid = fork()) < 0) { //Failed!     
          fprintf(stderr, "*** ERROR: forking child process failed\n");
          return -1; 
     }
     //Successful!
     else if (pid == 0) {         
          //Execute the command.  This will fail if the executable
          //does not exist or isn't an executable
          if (execve(fullpath, argv, envp) < 0) {     
               fprintf(stderr, "*** ERROR: exec failed\n");
               
               //If you want to return a return value to status
               //then you need to perform an exit()
               //note, you have to bitshift the returned result by 8 
               //(see below)
               exit(1); 
          }

          /*
           * No matter what nothing in this section will run because
           *    a) the execve replaces the original child and exists
           *    b) the child exits in the error loop with a return code of 
           *       signed 1
           */
          
     }
     else { 
          //The parent will wait for the child (exec) to finish
          pid_t who_finished;
          while ((who_finished = wait(&status)) != pid);
          printf("pid = %d finished with exit code = %d\n", who_finished, status >> 8);
     }

     return 0;
}

int main( int argc, char *argv[] )
{
     char raw_input[INPUT_BUFFER];   
     char* exec_argv[MAX_ARGS];

     // And we go on, and on, and on
     for(;;) {                     
          // Make sure the output buffer is clear first 
		  fflush(stdout);
          
          // Display prompt
          printf("¯\\_(ツ)_/¯ ");  
		  
          //Read in from the command line
          fgets(raw_input, INPUT_BUFFER, stdin);
          printf("\n");
         
          //tokenize the input and store result in exec_argv
          parse(raw_input, " ", exec_argv);       
         
          //Do we need to exit?
          //Technically, you don't need to do this as exit
          //is a executable, but I like to do it as a sanity measure
          if (strncmp(exec_argv[0], "exit", 4) == 0)  
                break;
          
          //No, let's execute the command then 
          if (execute_cmd(exec_argv) < 0) 
              break;
     }

     return 0;
}

                
