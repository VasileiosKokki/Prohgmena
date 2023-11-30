#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#define RANDOM_STRING_LENGTH 2000  // Dhlonei to megethos tou tixaiou string
#define NTHREADS 4  // Dhlonei ton arithmo ton threads


int charfrequency[26] = {0};  // Pinakas int 26 theseon gia to oliko frequency arxikopoieitai me 0
pthread_mutex_t mymutex=PTHREAD_MUTEX_INITIALIZER;  // Arxikopoihsh tou mutex
int myOffset = 0;  // Metavlhth pou dhlonei to offset, einai global oste na mporei na fanei se ola ta threads
int pid; // Global metavlhth gia ton signal handler
int flag = 0; // Arxikopoihsh tou flag se 0 gia ton signal handler


void * thread_func(void * args)  // Leitourgia pou ektelei to kathe thread
{
  char partial[500];  // Pinakas char 500 theseon gia thn anagnosh apo arxeio
  int *argptr = args;  // Pointer argptr pou deixnei sto argument pou tou pername otan to kaloume (dieuthinsi tou filedescriptor)

  int tempcharfrequency[26] = {0};  // Pinakas int 26 theseon gia to topiko frequency arxikopoieitai me 0
 
  pthread_mutex_lock(&mymutex);  // Kleidonoume ton mutex gia na mhn exoume pollaplh tautoxronh prosvash apo ta alla threads (krishmh perioxh)
  lseek(*argptr, myOffset, SEEK_SET);  // Apo to filedescriptor pou tou perasame san argument phgainei sthn arxh tou arxeiou kai metakinhei ton dhkth 
                                       //kata offset theseis makria
  
  for (size_t i = 0; i < 500; i++) {  // Arxikopoihsh tou pinaka partial me ""
  partial[i] = "";
  }
  
  read(*argptr,partial,strlen(partial));  // Diavazei apo to filedescriptor pou tou perasame san argument ta stoixeia tou arxeiou kai ta antigrafei 
                                          // ston pinaka partial, mexri na diavasei 500 bytes
 
  for (int i = 0; i < 500; i++) {  // Gemizei ton pinaka tempchatfrequency gia i apo 1 mexri 500 me vhma 1 me ton ekshs tropo:
    tempcharfrequency[partial[i] - 'a']++;  // O xarakthras pou vrisketai ston pinaka partial sthn thesi i, afairontas tou to 'a' mporei na parei times 
  }                                         // mono apo to 0 mexri to 26. Sinepos auksanoume thn thesi tou pinaka tempcharfrequency pou antistoixei sto
                                            // partial[i] - 'a' kata ena

//   Xrhsimo gia debugging  
 
  /*
  for (int i = 0; i < 26; i++) {
    printf("%c->%d  ", i+49+'0', tempcharfrequency[i]); 
  }
    printf("\n");
  */
  
  for (int i = 0; i < 26; i++) {  // Apo to 1 mexri to 26 me vhma 1
  charfrequency[i]+=tempcharfrequency[i];  // Prosthetei ta topika apotelesmata tou tempcharfrequency ston kirio pinaka charfrequency 
  }
  myOffset += 500;  // Metavaloume to offset kata 500 theseis
  pthread_mutex_unlock(&mymutex);  // Ksekleidonoume ton mutex afou teliosame thn epeksergasia sthn krishmh perioxh   
  pthread_exit (NULL);  // Telionei h ektelesh tou thread
}


void sig_handler(int signum){  // Leitourgia pou kanei cleanup an dexthei shma sigint h sigterm afou prota rothsei ton xrhsth an ontos thelei na kanei cleanup


  if (pid!=0) { // An to pid einai tou patera (gia na mhn ektelestei 2 fores o signal handler)
    char answer[2];
    answer[1] = '\0';
    
        if (flag == 0) { // An den exei dothei kapoio signal eno vriskomaste hdh ston signal handler
        printf("\nDo you really want to terminate the program? [y/N] -> ");
        do {
        
        scanf(" %1s", answer);
        flag = 1;
        if (answer[0] == 'y' || answer[0] == 'Y') {
            printf("Terminating program..\n");  // Kanei print sto termatiko thn frash Terminating program..
            const char *semName = "myfilelockFork";  // Thetei to onoma tou shmaforou iso me myfilelockFork
            sem_t *sem_filelock;  // Dhlonei thn kleidaria
            sem_close(sem_filelock);  // Kleinei ton shmaforo (thn kleidaria)
            sem_unlink(semName);  // Aposindeei to onoma tou shmaforou apo ton shmaforo
            exit(0);  // Kanei exit me epitixia
        } else if (answer[0] == 'n' || answer[0] == 'N'){
            printf("Continuing...\n");
        } else {
            printf("Wrong Input! Give A Valid Answer This Time...[y/N] -> ");
            fflush(stdin);  // Kanoume clear to input buffer
            scanf("%*[^\n]");  // Agnooume tous upoloipous xarakthres sto input buffer
        }
          
           } while (answer[0] != 'y' && answer[0] != 'Y' && answer[0] != 'n' && answer[0] != 'N');

        }
        
  }
   
}




int main(){  // Leitourgia ths main

    const char *semName = "myfilelockFork";  // thetei to onoma tou shmaforou iso me myfilelockFork
    sem_t *sem_filelock;  // Dhlonei thn kleidaria
    sem_filelock = sem_open(semName, O_CREAT, 0600, 1);  // Anoigoume ton shmaforo me onoma semName (myfilelockFork) kai thn parametro O_CREAT
                                                         // pou ton dhmiourgei MONO an den iparxei , me dikaiomata gia ton xrhsth kai timh 1

    int fd=open("data.txt", O_RDWR | O_CREAT, 00600);  // Anoigoume to arxeio data.txt me parametro readwrite kai thn parametro O_CREAT
                                                       // pou to dhmiourgei MONO an den iparxei kai me dikaiomata gia ton xrhsth

    pid=fork();  // Kanoume fork meta thn fd=open oste o file descriptor ths kirias diergasias kai tou paidiou na einai koinos

        signal(SIGINT,sig_handler);  // Kaloume thn leitourgia sig_handler an dothei shma SIGINT
        signal(SIGTERM,sig_handler);  // Kaloume thn leitourgia sig_handler an dothei shma SIGTERM

    if (pid!=0) {  // An to pid einai tou patera
        


        //printf("hello from parent \n");   Xrhsimo gia debugging

        sem_wait(sem_filelock);  // A. Kanei thn timh tou shmaforou ish me 0 (mpainei gia proth fora edo, prohgoumenos h timh htan dhlomenh me 1)

        srand(getpid());  // Xrhsimo gia thn dhmiourgia tixeon xarakthron me vash to pid
        char buf[RANDOM_STRING_LENGTH + 1];  // Dhmiourgia pinaka char me onoma buf kai me 2001 theseis
        for (size_t i = 0; i < RANDOM_STRING_LENGTH; i++) {  // Gia i apo 0 mexri 2000 me vhma 1
        buf[i] = "abcdefghijklmnopqrstuvwxyz"[rand() %26];  // Pairnei enan tixeo xarakthra apo autous pou periexontai sta eisagogika kai ton topothetei sthn
        }                                                   // i thesh tou pinaka buf

        buf[RANDOM_STRING_LENGTH] = '\0';  // Sto telos tou pinaka prosthetoume to null terminator gia na deiksoume oti telionei to string
        write(fd,buf,strlen(buf));  // Grafoume apo to fd sto arxeio data.txt ta periexomena tou pinaka buf mexri na exoun graftei 2000 bytes
        
        //printf("Writing from process:%d\n", getpid());   Xrhsimo gia debugging

        sleep(3);  // Mikrh kathisterhsh gia na prolavei o xrhsths na pathsei ctrl c
        sem_post(sem_filelock);  // B. Kanei thn timh tou shmaforou ish me 1 (prohgoumenos h timh htan 0) 

        // Prepei na perimenoume to paidi gia na kanei cleanup o goneas (na kleisei to arxeio kai ton shmaforo)
        int status;  // Metavlhth pou dhlonei thn katastash pou vrisketai to paidi
        waitpid(pid,&status,0);  // Perimenei mexri na teleiosei to paidi
        printf("Parent cleaning up..\n");  // Kanei print sto termatiko to parapano string
        close(fd);  // Kleinei ton file descriptor
        sem_close(sem_filelock);  // Kleinei ton shmaforo
        sem_unlink(semName);  // Aposindeei to onoma tou shmaforou apo ton shmaforo
           
    } else {  // An to pid einai tou paidiou

        //printf("hello from child\n");   Xrhsimo gia debugging
        sem_wait(sem_filelock);  // C. Kanei thn timh tou shmaforou ish me 0 (prohgoumenos h timh htan 1) 

        //printf("Reading from process:%d\n", getpid());   Xrhsimo gia debugging


        int filedescriptors[4];  // Pinakas apo filedescriptors
        for (int i = 0; i < 4; i++) {  // Gia i apo 0 mexri 4 me vhma 1
          filedescriptors[i] = dup(fd);  // dhmiourgei tessera nea filedescriptors pou einai antigrafa tou arxikou fd
        }

        pthread_t threads[NTHREADS];  // Dhlosh pinaka apo threads

        for (int i = 0; i < NTHREADS; i++) {  // Gia i apo 0 mexri 4 me vhma 1
        
        pthread_create(&threads[i], NULL, thread_func, &filedescriptors[i]);  // Dhmiourgei neo thread pou tha ektelei thn leitourgia thread_func kai
                                                                              // tou pernaei os parametro thn dieuthinsi tou i-ostou filedescriptor
        }
        
        for (int i = 0; i < NTHREADS; i++){  // Gia i apo 0 mexri 4 me vhma 1
         pthread_join(threads[i],NULL);  // Perimenei mexri na teliosoun ola ta threads kai gnostopoiei to charfrequency
        }
        for (int i = 0; i < 26; i++) {  // Gia i apo 0 mexri 26 me vhma 1
        printf("%c->%d  ", i+49+'0', charfrequency[i]);  // Ektiponei sto termatiko ton xarakthra i(+49+'0' , metatroph int se char me vash ton pinaka ascii) 
        }                                                // kai dipla ektiponei to poses fores emfanistike o xarakthras autos

        printf("\n");  // Prosthetei newline (afhnei mia grammh)

        sleep(3);  // Mikrh kathisterhsh gia na prolavei o xrhsths na pathsei ctrl c
        for (int i = 0; i < 4; i++) {  // Gia i apo 0 mexri 4 me vhma 1
          close (filedescriptors[i]);  // Kleinei tous file descriptors pou einai antigrafa tou arxikou fd
        }
        sem_post(sem_filelock);  // D. Kanei thn timh tou shmaforou ish me 1 (prohgoumenos h timh htan 0)
    }

}