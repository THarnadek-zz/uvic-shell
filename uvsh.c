#include  <stdio.h>
#include <stdlib.h>
#include  <sys/types.h>
#include <errno.h>     //required for stderr 
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <unistd.h>  //Uncomment if your on GCC5

#define INPUT_BUFFER 80
#define MAX_ARGS 9
#define MAX_PROMPT_LEN 10
#define MAX_DIRS 10
#define MAX_PATH 255
// SET TO 1 to get DEBUG msgs
#define DEBUG 1

//Global Variables
char* path;
//----------------

// just a quick error handler to save me some time
void uvsh_error(char* message){
    fprintf(stderr, "*** ERROR: %s\n",  message);
}

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

int do_out(char *argv[]){
  pid_t  pid;
  int delim = -1;
  int i = 0;
  int status;
  char *envp[] = { 0 };
  int fd;
  char *args[MAX_ARGS];



  if ((pid = fork()) == 0) {
        //check for delineator
    while(argv[i]!=NULL){
      if(strncmp(argv[i], "::", 2) == 0){
        delim = i;
      }
      ++i;
    }
    if(delim < 0){
      uvsh_error("do-out missing '::'. Usage: do-out <command> :: <destination>");
      exit(1);
    }
    if(argv[delim+1]==NULL){
      uvsh_error("do-out missing destionation. Usage: do-out <command> :: <destination>");
      exit(1);
    }
    if(i<4){
      uvsh_error("do-out missing command. Usage: do-out <command> :: <destination>");
      exit(1);
    }

    int j = 0;
    for(i = 1; i<delim;++i){
      args[j] = argv[i];
      j++;
    }
    // open output file
    printf("child starting\n");
    if(DEBUG == 1){
      printf("attempting to open argv[i+1]: %s\n", argv[delim+1]);  
    }
    fd = open(argv[i+1], O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if (fd == -1) {
        fprintf(stderr, "cannot open %s for writing\n", argv[delim+1]);
        exit(1);
    }else{
      fprintf(stdout, "opened %s for do_out\n", argv[delim+1]);
    }

    // complete path for execve
    char fullpath[INPUT_BUFFER]; 
    //Full path to my executable
    strcat(fullpath, path);
    strcat(fullpath, args[0]);
    args[0] = fullpath;


    dup2(fd, 1);
    dup2(fd, 2); 
    execve(args[0], args, envp);
    printf("child is in a bad place\n");
  }
  waitpid(pid, &status, 0);
  if(DEBUG == 1)
    printf("pid = %d finished with exit code = %d\n", pid, status >> 8);
  return 0;
}

int  execute_cmd(char *argv[])
{
    pid_t  pid;
    int    status;

    //We need to get the full path
    char fullpath[INPUT_BUFFER];
     
     //Full path to my executable
     strcat(fullpath, path);
     strcat(fullpath, argv[0]);
     argv[0] = fullpath;

     //Empty envp variable
     char* const envp[] = {NULL};
       
     //Just some test logging
    if(DEBUG == 1){
		  int i;
		  for(i = 0; argv[i] != NULL; ++i) 
		    printf("LOG: %s\n", argv[i]);
	  }

    // Fork a child process 
    if ((pid = fork()) < 0) { //Failed!     
      uvsh_error("forking child process failed");
      return -1; 
    }
    //Successful!
    else if (pid == 0) {         
      //Execute the command.  This will fail if the executable
      //does not exist or isn't an executable
      if (execve(fullpath, argv, envp) < 0) {     
        uvsh_error("exec failed");
        exit(1);

        //If you want to return a return value to status
        //then you need to perform an exit()
        //note, you have to bitshift the returned result by 8 
        //(see below)
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
          waitpid(pid, &status, 0);
  	   if(DEBUG == 1)
  		  printf("pid = %d finished with exit code = %d\n", pid, status >> 8);
      }

    return 0;
}

int main( int argc, char *argv[] )
{
     char raw_input[INPUT_BUFFER];   
     char* exec_argv[MAX_ARGS];
     int result = 0;
     //---------
     // get path
     //--------- 
    FILE* fp = NULL;
    if(!(fp = fopen(".uvshrc", "r"))){
      uvsh_error(".uvshrc not found");
      printf("Exiting, did not find .uvshrc");
      exit(1);
    }
    //255 max address length
    char line[255];
    if(fgets(line, sizeof(line),fp)){
      path = line;
      if(DEBUG==1)
        printf("Path retrieved from .uvshrc : %s\n", path);
    }else{
      path = "/bin/";
    }
    fclose (fp);

    //----------
    // main loop
    //----------
    for(;;) {                     
          // Make sure the output buffer is clear first 
		  fflush(stdout);
          
          // Display prompt
          printf("$ ");  
		  
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

          // Check if this is a do-out , pipe, or regular
          if(strncmp(exec_argv[0], "do-out", 6) == 0)
            result = do_out(exec_argv);
          else
            result = execute_cmd(exec_argv);

          //something bad happened so exit the loop
          if (result < 0) 
              break;
     }

     return 0;
}

                
