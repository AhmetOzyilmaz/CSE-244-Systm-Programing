/***********************************************************************************************/
/* AHMET ÖZYILMAZ 111044014 Homework5 */
/***********************************************************************************************/

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <termios.h> 
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>




#define MAXPATHLEN 1024
#define BUFSIZE 1024
#define NTHREADS 100
#define LOGFILE "logFile.txt"


int FindNumberOfFile(char *TargetDirectory, char *KeyWordTwo);
int grepfromFile(char* fileName , char* Key,long int ThreadId,int pipefd);
void *thread_function(void *dummyPtr);
void FindMyKeyWord(char *TargetDirectory, char *KeyWordTwo);


int fifo;
char FIFO_FILE[BUFSIZE]; /*main fonksiyonundaki ana fork */

int threadCount=0;/*i */
static volatile sig_atomic_t doneflag = 0;

static void setdoneflag(int signo)
{
    doneflag = 1;
}

/* prototype for thread routine */


pthread_t thread_id[NTHREADS];/* stor threads id*/

typedef struct 
{
    char filN[MAXPATHLEN];
    char KeyN[100];
    long int ThreadIdN;
    int pipefdN;
} grepFrom ;

/* glo

bal vars */
/* semaphores are declared global so they can be accessed 
   in main() and in thread routine,
   here, the semaphore is used as a mutex */
sem_t mutex;
int counter; /* shared variable */
int main(int argc,char* argv[])
{
     int pid ;/* recursive fonksiyonu çağırıcak olan fork*/
    int fdFifo;
    char memory[BUFSIZE]; /* Okuma islemi icin gerekli string */

    FILE *oup;
    oup = fopen(LOGFILE,"a");

    if(argc != 3){
       fprintf(stderr, "Wrong USAGE : ./11044014_AHMET_OZYILMAZ_HW4 directoryName Keyword\n");
       return -1 ;
    }

    /* Signal icin gerekli kisim. */
    struct sigaction act;
    act.sa_handler = setdoneflag; /* set up signal handler */
    act.sa_flags = 0;
    if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1))
    {
        perror("Failed to set SIGINT handler");
        return 1;
    }

    sprintf(FIFO_FILE,"FIFO_%ld",(long)getpid());/*Ana fifo*/
    sem_init(&mutex, 0, 1);      /* initialize mutex to 1 - binary semaphore */


    /*fd main fifo descripter*/
    if ((fdFifo=mkfifo(FIFO_FILE, 0666)) == -1)
    {
    /* create a named pipe */
        if (errno != EEXIST)
        {
            fprintf(stderr, "[%ld]:failed to create named pipe %s: %s\n",(long)getpid(), FIFO_FILE, strerror(errno));
            return 1;
        }
    }
    
    pid = fork();
    if(pid == -1)
    {
        fprintf(stderr, "ERROR CANNOT FORK");
        exit(1);
    }
    else if(pid == 0 )
    {
        /*fifo oluituruğ child findmy fonksyionunu çağırıcak */
       
       FindMyKeyWord(argv[1], argv[2]);
        exit(0);
    }
    else
    {

         fdFifo = open(FIFO_FILE,O_RDONLY); // read
                    memset(memory,0,BUFSIZE);

               /*child dan gelen verileri log dosyasına basıcak */ 
        //fprintf(stderr, "XXXXXXXXXXXXXXXXXXXXXXXXXX funtion  parent pid %ld  child pid %ld \n",(long)getppid(),(long)getpid());
        while(read(fdFifo,memory,BUFSIZE) > 0)
       {
           fprintf(stderr,"%s",memory);/*fifoya yazılıcak root sa log dosyasına yazıcaz */
           fprintf(oup,"%s\n",memory);/*fifoya yazılıcak root sa log dosyasına yazıcaz */
            memset(memory,0,BUFSIZE);

        }

    }

        
    while(wait(NULL)>0);

    fclose(oup);
    unlink(FIFO_FILE);
    // wait(NULL);
   // fprintf(stderr, "ZZZZZZZZZZZZZZZZZZZZZZZZZZZ main funtion  parent pid %ld  child pid %ld \n",(long)getppid(),(long)getpid());
                  
    /* exit */     
    sem_destroy(&mutex); /* destroy semaphore */

    exit(0);
} /* main() */


int FindNumberOfFile(char *TargetDirectory, char *KeyWordTwo)
{

    int fdPipe[2],fdFork;/*fifo descriptor*/
    struct stat stDirInfo,stFileInfo;
    struct dirent * stFiles;
    DIR * stDirIn;
    char szFullName[MAXPATHLEN],szDirectory[MAXPATHLEN];/*sys/param.h ait  MAXPATHLEN*/
    int NumberOfFile = 0;

    strcpy( szDirectory,TargetDirectory);

    if (stat( szDirectory, &stDirInfo) < 0){/*Sysyem call information about a file based on its filename. */
        fprintf(stderr, "No such file or directory\n");
        exit(0);
    }

    if (!S_ISDIR(stDirInfo.st_mode))
        exit(0);

    if ((stDirIn = opendir( szDirectory)) == NULL){
        fprintf(stderr, "file or directory Cannot open\n");
        exit(0);
    }
    if (pipe(fdPipe) == -1)/*directoryler için fifo folder la için pipe */
    {
        perror("Failed to create the pipe");
        exit(EXIT_FAILURE);
    }

    while (( stFiles = readdir(stDirIn)) != NULL) /* klasorun icerigi okunur */
    {
        sprintf(szFullName, "%s/%s", szDirectory, stFiles -> d_name );

        if (lstat(szFullName, &stFileInfo) < 0)
           perror ( szFullName );

        if (S_ISDIR(stFileInfo.st_mode))
        {

            if ((strcmp(stFiles->d_name , "..")) && (strcmp(stFiles->d_name , ".")))
            {
                fdFork = fork();
                if(fdFork == -1)
                {
                    fprintf(stderr, "ERROR CANNOT FORK");
                    exit(1);
                }
                else if(fdFork == 0 )
                {
                    FindNumberOfFile(szFullName, KeyWordTwo);
                    exit(0);
                }
            }
        }

        else /* dosya ise */
        {
            NumberOfFile ++ ;
        }

    }  

    while ((errno == EINTR) && (closedir(stDirIn) == -1)) ;
   // wait(&status);

    return NumberOfFile;
}

void FindMyKeyWord(char *TargetDirectory, char *KeyWordTwo)
{
    int fdFifo;
    int status,fdPipe[2],fdFork;/*fifo descriptor*/
    struct stat stDirInfo,stFileInfo;
    struct dirent * stFiles;
    DIR * stDirIn;
    char szFullName[MAXPATHLEN],szDirectory[MAXPATHLEN];/*sys/param.h ait  MAXPATHLEN*/
    char memory[BUFSIZE];
    grepFrom* grepfromFileStruct;
    int Value = 0;
    char** szFullNameArray;
    int index = 0;
   // fprintf(stderr, "BORN parent pid %ld  my pid %ld \n",(long)getppid(),(long)getpid());

    
    Value =  FindNumberOfFile(TargetDirectory,KeyWordTwo);
    //fprintf(stderr, "NumberOfFile - --------> %d\n", Value);
    szFullNameArray = (char**)malloc(Value * sizeof(char*));

    for (int i = 0; i < Value; ++i)
    {
        szFullNameArray[i] = malloc(sizeof(char)*MAXPATHLEN);
    }

    grepfromFileStruct = (grepFrom*) malloc(sizeof(grepFrom)*Value);


    strcpy( szDirectory,TargetDirectory);

    if (stat( szDirectory, &stDirInfo) < 0){/*Sysyem call information about a file based on its filename. */
        fprintf(stderr, "No such file or directory\n");
        exit(0);
    }

    if (!S_ISDIR(stDirInfo.st_mode))
        exit(0);

    if ((stDirIn = opendir( szDirectory)) == NULL){
        fprintf(stderr, "file or directory Cannot open\n");
        exit(0);
    }

    if (pipe(fdPipe) == -1)/*directoryler için fifo folder la için pipe */
    {
        perror("Failed to create the pipe");
        exit(EXIT_FAILURE);
    }

    while (( stFiles = readdir(stDirIn)) != NULL) /* klasorun icerigi okunur */
    {
        sprintf(szFullName, "%s/%s", szDirectory, stFiles -> d_name );

        if (lstat(szFullName, &stFileInfo) < 0)
           perror ( szFullName );

        if (S_ISDIR(stFileInfo.st_mode))
        {

            if ((strcmp(stFiles->d_name , "..")) && (strcmp(stFiles->d_name , ".")))
            {

                fdFork = fork();
                if(fdFork == -1)
                {
                    fprintf(stderr, "ERROR CANNOT FORK");
                    exit(1);
                }
                else if(fdFork == 0 )
                {

                    FindMyKeyWord(szFullName, KeyWordTwo);
                    exit(0);
                }
            }
        }

        else /* dosya ise */
        {

             strcpy(szFullNameArray[index],szFullName);

             strcpy(grepfromFileStruct[index].filN,szFullName);
            //fprintf(stderr, "%s\n",grepfromFileStruct[index].filN );

            strcpy(grepfromFileStruct[index].KeyN,KeyWordTwo);
            grepfromFileStruct[index].pipefdN =fdPipe[1]; 
            //fprintf(stderr, "%s\n",grepfromFileStruct.filN  );        
            pthread_create( &thread_id[threadCount], NULL, thread_function,&grepfromFileStruct[index]);
            threadCount++;
           
            index++;
            sleep(1);
        }

    }  

    while ((errno == EINTR) && (closedir(stDirIn) == -1)) ;

   // fprintf(stderr, "1 parent pid %ld  child pid %ld \n",(long)getppid(),(long)getpid());
   // printf("threadCount %d\n",threadCount );

    //fprintf(stderr, "threadCount %d\n",threadCount );

   // fprintf(stderr, " 2 parent pid %ld  child pid %ld \n",(long)getppid(),(long)getpid());
    close(fdPipe[1]);

    fdFifo = open(FIFO_FILE,O_WRONLY); // write 


    while(read(fdPipe[0],memory,BUFSIZE) > 0)
    {
        //fprintf(stderr,"AHMET %s",memory);/*pipe a yazılanlar okunur*/
        write(fdFifo, memory, strlen(memory));
      //  fprintf(stderr,"ReadPipe ->>>>< %s\n",memory);
        memset(memory,0,BUFSIZE);
        sleep(1);
    }


        //fprintf(stderr, "NumberOfFile - --------> %d\n", Value);

    for (int i = 0; i < Value; ++i)
    {
         free( szFullNameArray[i]);
    }
   // free(szFullNameArray);
    free(grepfromFileStruct);

    while ((wait(&status)) > 0);
}



void *thread_function(void *dummyPtr)
{
    grepFrom grepfromFileStruct;
    grepfromFileStruct = *((grepFrom*)dummyPtr);

    sem_wait(&mutex);       /* down semaphore */
    /* START CRITICAL REGION */
   // fprintf(stderr, "Can Readable File -> %s\n",grepfromFileStruct.filN );
    grepfromFile(grepfromFileStruct.filN, grepfromFileStruct.KeyN,grepfromFileStruct.ThreadIdN,grepfromFileStruct.pipefdN);

    sem_post(&mutex);       /* up semaphore */
    
    //fprintf(stderr,"%d\n", threadCount);
    pthread_exit(0);
}





int grepfromFile(char* fileName , char* Key,long int ThreadId,int pipefd){

    int RowCoun = 0,ColumnCoun = 0,check = 2 ,temp = 0,index= 0, i =0 ,j = 0,k=0,result = 0,t = 0;
    int keyLenght = 0,currentRow  = 0,lineLenght = 0;
    char buf[BUFSIZE];/* Yazma islemi icin gerekli string */

    char red=' ',*line;

    FILE * fp1;

    fp1= fopen(fileName, "r");  

    while(Key[keyLenght] != '\0'){
        keyLenght++;
    }

        while(check!=0)
        {
            check = fscanf(fp1,"%c",&red);
            
            if(red== '\n')
            {
                RowCoun ++;
                if( ColumnCoun> temp)
                    temp = ColumnCoun;
                ColumnCoun=0;
            }
            else
            {
                ColumnCoun++;
            }
            if(check == EOF){
                RowCoun++;
                
                if( ColumnCoun > temp)
                    temp = ColumnCoun;
                else
                    ColumnCoun = temp;
                
                break;
            }
        }
        fclose(fp1);
        fp1= fopen(fileName, "r");
           
        line = (char *) malloc(ColumnCoun+2);
        /*fprintf(stderr,"%d\n",RowCoun);*/
        
        for(t= 0; t<RowCoun ;++t)
        { 

            while(1)/* Line take*/
            {
               check =fscanf(fp1,"%c",&line[index]);
               /*fprintf(stderr,"%c",line[index]);*/
               if(line[index] == '\n' || check == EOF){/*tek bir satırın alındığı kısım*/
                   currentRow++;
                   break;
               }
                index++;
            }
            lineLenght = index;
            
           for(i = 0 ; i < lineLenght  ; i++ ){ 
               for(j = 0 ; j < keyLenght ; ++j  )
               {
                   if(line[j+i] == Key[j] )
                   {
                       result++;
                   }
               }
               if( result ==  keyLenght){/* Anahtar ile kilit cümle uyum kontrolu yapılıypr*/
                   j+=i-keyLenght;
                
               // close(pipefd[0]);
                memset(buf,0,BUFSIZE);

                sprintf(buf," File Name Key %s Column = %d CURRRENT ROW =  %d\n",fileName,j+1,  t+1);
               // fprintf(stderr," --------------------------------_> File Name Key %s Column = %d CURRRENT ROW =  %d\n",fileName,j+1,  t+1);

                if( write( pipefd, buf, BUFSIZE ) < 0 )
                {
                    perror( "child write to FIFO" );
                }
               }

               result= 0;
               
            }
            index =0 ;
            for(k = 0 ; k< ColumnCoun+2; ++k ){/*dinamik arrayin temizlendiği kısım */
                line[k]=' ';
            }
        }
    //close(pipefd);

    fclose(fp1);
    free(line);
    return 1;
}
