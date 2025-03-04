#include <unistd.h>    
#include <sys/types.h> 
#include <sys/wait.h> 
#include <stdio.h>    
#include <stdlib.h>    

void make_reader(){
    pid_t reader = fork();
    if(reader == 0){
        char* args[] = {"./client", "reader", NULL};
        execvp(args[0], args);
        exit(0);
    }
}

void make_writer(){
    pid_t writer = fork();
    if(writer == 0){
        char* args[] = {"./client", "writer", NULL};
        execvp(args[0], args);
        exit(0);
    }
}

int main(){

    for(int i = 0; i < 6; i++){
        if(i % 2 == 0){
            make_reader();
        }

        else{
            make_writer();
        }
    }

    wait(0);

}
