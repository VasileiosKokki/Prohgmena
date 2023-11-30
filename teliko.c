#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>


int child_count; // Adjust this to your desired number of children
pid_t* child_pids; // Array to store child PIDs



// -------------------- Signal Handlers ------------------ //



void kill_children(int sig) {
    for (int i = 0; i < child_count; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGTERM);
        }
    }
}


void on_sig_term(int sig) {
    printf("I am child, my father just terminated me!\n");
    exit(0);
}

// -------------------- Signal Handlers ------------------ //




// -------------------- Functions ------------------ //


void parent_process(int pipe_fd, int done_pipe_fd, int i) {
    char message[100];
    // Format the message with the child number
    sprintf(message, "Hello child, I am your father and I call you: 'Slave %d'", i);
    write(pipe_fd, message, strlen(message));


    // Read the message from the pipe
    char done_msg[100];
    ssize_t bytes_read = read(done_pipe_fd, done_msg, sizeof(done_msg));
    if (bytes_read == -1) {
        perror("read");
        exit(1);
    }
    done_msg[bytes_read] = '\0';
    printf("%s\n", done_msg);
    // Find the position of "'" in the message
    char *colon_position = strchr(done_msg, '\'');
    char *message_after_colon;
    if (colon_position != NULL) {
        message_after_colon = colon_position + 1; // Skip "' " or "'"
    }
    char *quote_position = strchr(message_after_colon, '\'');
    if (quote_position != NULL) {
        *quote_position = '\0'; // Null-terminate the string at the single quote position
    }
    sprintf(message, "Done received from '%s'", message_after_colon);
    write(pipe_fd, message, strlen(message));
}





void child_process(int pipe_fd, int done_pipe_fd, int file_fd) {

    
    // Read the message from the pipe
    char message[100];
    ssize_t bytes_read = read(pipe_fd, message, sizeof(message));
    if (bytes_read == -1) {
        perror("read");
        exit(1);
    }
    message[bytes_read] = '\0';


    // Print child's name and PID to the file
    printf("%s\n", message);

    // Find the position of ":" in the message
    char *colon_position = strchr(message, ':');
    if (colon_position == NULL) {
        printf("Invalid message format: %s\n", message);
        exit(1);
    }

    // Extract the part of the message after ":"
    char *message_after_colon = colon_position + 2; // Skip ": " or ":"

    dprintf(file_fd, "%d ---> %s\n", getpid(), message_after_colon); // Use dprintf to write to the file descriptor

    char message2[100];
    // Format the message with the child number
    sprintf(message2, "Child %s is done", message_after_colon);
    // Notify the father process that printing is done
    write(done_pipe_fd, message2, strlen(message2));



    // Read the message from the pipe
    char message3[100];
    ssize_t bytes_read2 = read(pipe_fd, message3, sizeof(message3));
    if (bytes_read2 == -1) {
        perror("read");
        exit(1);
    }
    message3[bytes_read2] = '\0';


    // Print child's name and PID to the file
    printf("%s\n", message3);
    
}


// -------------------- Functions ------------------ //



// -------------------- Main ------------------ //


int main(int argc, char *argv[]) {
    
    
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file_name> <number_of_children>\n", argv[0]);
        return 1;
    }


    char *file_name = argv[1]; // File name specified by the user
    child_count = atoi(argv[2]); // Number of child processes specified by the user


    child_pids = (pid_t*)malloc(child_count * sizeof(pid_t));
    int pipes[child_count][2]; // Array of pipes for communication
    int done_pipes[child_count][2]; // Array of pipes for child completion notification
    int gonethroughere = 0;


    const char *semName = "myfilelockFork1";  // thetei to onoma tou shmaforou iso me myfilelockFork
    sem_t *sem_filelock;  // Dhlonei thn kleidaria
    sem_filelock = sem_open(semName, O_CREAT, 0600, 2);  // Anoigoume ton shmaforo me onoma semName (myfilelockFork) kai thn parametro O_CREAT
                                                         // pou ton dhmiourgei MONO an den iparxei , me dikaiomata gia ton xrhsth kai timh 1
    
    
    


    // Open the log file
    int file_fd=open(file_name, O_RDWR | O_CREAT, 00600);
    if (file_fd == -1) {
        perror("open");
        exit(1);
    }

    // Create pipes for communication with children
    for (int i = 0; i < child_count; i++) {
        if (pipe(pipes[i]) == -1 || pipe(done_pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    
    // Fork child processes
    for (int i = 0; i < child_count; i++) {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            exit(1);
        } else if (child_pid > 0) {
            // This is the father process
            if (gonethroughere == 0){
                gonethroughere = 1;
                dprintf(file_fd, "[PARENT] ---> %d\n", getpid());
                signal(SIGALRM,(void (*)(int))kill_children);
                alarm(10);
            }          
         

            close(pipes[i][0]); // Close the read end of the communication pipe
            close(done_pipes[i][1]); // Close the write end of the communication2 pipe
            child_pids[i] = child_pid;
            sem_wait(sem_filelock);
            parent_process(pipes[i][1], done_pipes[i][0], i);
            sem_post(sem_filelock);
            close(pipes[i][1]); // Close the read end of the communication pipe
            close(done_pipes[i][0]); // Close the write end of the communication2 pipe
            
        } else {
            
            // This is the child process
            close(pipes[i][1]); // Close the write end of the communication pipe
            close(done_pipes[i][0]); // Close the read end of the communication2 pipe
            sem_wait(sem_filelock);
            child_process(pipes[i][0], done_pipes[i][1], file_fd);
            sem_post(sem_filelock);
            close(pipes[i][0]); // Close the write end of the communication pipe
            close(done_pipes[i][1]); // Close the read end of the communication2 pipe

            signal(SIGTERM,(void (*)(int))on_sig_term);
            sleep(20);
            exit(0);
        }
    }
    

    
    signal(SIGINT,(void (*)(int))kill_children);  // Kaloume thn leitourgia sig_handler an dothei shma SIGINT
    for (int i = 0; i < child_count; i++) {
        waitpid(child_pids[i], NULL, 0);
    }
    printf("Children have been gracefully shut down\n");
    close(file_fd); // Close the file descriptor after the child_process 
    free(child_pids); // Free the dynamically allocated array when done
    sem_close(sem_filelock);  // Kleinei ton shmaforo
    sem_unlink(semName);  // Aposindeei to onoma tou shmaforou apo ton shmaforo
    return 0;

    
}
