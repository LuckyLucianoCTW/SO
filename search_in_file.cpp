#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>


pthread_mutex_t lock; 

enum e_state
{
 pending,
 processing,
 done,
 suspended
};


struct fileStruct
{
  char* pathFile;
  char** path;
  char** files;
  int*   fileSize;
  int sizeofPath;
  int sizeofFiles;
  int totalSize; 
  enum e_state my_state;
  int priority;
  int id;
  int allocated;
  int currentReadFilePoint;
  int currentReadPathPoint;
};



 
typedef struct fileStruct fileStruct;

void Alloc(fileStruct* st);
void swapFunc(fileStruct* y, fileStruct* x)
{
   free(y->fileSize);
  strcpy(y->pathFile,x->pathFile);
  for(int i = 0 ; i < x->sizeofPath; i++)
     strcpy(y->path[i],x->path[i]);
  y->fileSize = (int*)malloc(x->sizeofFiles * sizeof(int));
  for(int i = 0 ; i < x->sizeofFiles; i++){
     strcpy(y->files[i],x->files[i]);
    y->fileSize[i] = x->fileSize[i];
  }
  y->sizeofPath = x->sizeofPath;
  y->sizeofFiles = x->sizeofFiles;
  y->totalSize = x->totalSize;
  y->my_state = x->my_state;
  y->priority = x->priority;
  y->id = x->id;
  y->allocated = x->allocated;
  y->currentReadFilePoint = x->currentReadFilePoint;
  y->currentReadPathPoint = x->currentReadPathPoint;
}


int readFileSize(const char* filename)
{
 struct stat st;
 st.st_size = 0;
 stat(filename,&st);
 return st.st_size;
}
int isFile(const char* path)
{
 struct stat path_f;
 stat(path,&path_f);
 return S_ISREG(path_f.st_mode);
}
 

void readPath(char* path, fileStruct* st)
{
  
 struct dirent *pDirent;
 DIR *pDirectory = opendir(path);
 if(pDirectory == NULL)
   return;
 int stage = 0;
 while( (pDirent = readdir(pDirectory)) != NULL)
 {
 if(!strcmp(pDirent->d_name,"."))
 continue;
 if(!strcmp(pDirent->d_name,".."))
 continue;

 char* test = malloc(256 * sizeof(char));
 strcpy(test,path);
 strcpy(test + strlen(test), "/");
 strcat(test,pDirent->d_name);
 if(isFile(test) == 0)
 {
 strcpy(st->path[st->sizeofPath],test);
 st->sizeofPath++;
 }
  else
 {
 strcpy(st->files[st->sizeofFiles],test);
 st->sizeofFiles++;
  
 
 }
 free(test);
 }
 
 closedir(pDirectory);
 
}


void ReadFileSize(fileStruct* st)
{
  st->totalSize = 0;
  st->fileSize = malloc(st->sizeofFiles * sizeof(int));
  for(st->currentReadFilePoint ; st->currentReadFilePoint < st->sizeofFiles; st->currentReadFilePoint++)
  {
  
  if(st->my_state == 3)
   break;
  st->fileSize[st->currentReadFilePoint] = readFileSize(st->files[st->currentReadFilePoint]);
  st->totalSize += st->fileSize[st->currentReadFilePoint];
   
  } 
}

void ReadPaths(fileStruct* st)
{
   if(st->sizeofPath == 0)
   readPath(st->pathFile, st);
   while(st->currentReadPathPoint < st->sizeofPath)
   {
   if(st->my_state == 3)
   break;
   readPath(st->path[st->currentReadPathPoint],st);
   st->currentReadPathPoint++; 
   }
   ReadFileSize(st);
}

 

void PrintDetails(fileStruct* st, int type)
{
  char stars[4];
  char starx[] = "*";
   for(int i = 0 ; i < st->priority;i++)
      strcat(stars,starx);
      float percent = st->currentReadFilePoint / (float)st->sizeofFiles;
     percent *= 100;
 printf("ID : %d  | Priority : %s |Total Files in %s | Directory : %d | Size : %0.1f [mb] | dirs : %d | ReadPercent= %0.1f%c \n",st->id,stars,st->pathFile,st->sizeofFiles,st->totalSize/1000000.0f,st->sizeofPath, percent,'%');
  if(type == 0)
    return;
  for(int i = 0 ; i < st->sizeofFiles; i++)
  {
    float f = 0.0f;
    if(st->fileSize[i] > 0)
     f = (float)st->fileSize[i]/st->totalSize;
    f*= 100;
    printf("File : %s %f %c",st->files[i],f,'%');
    printf("\n");
   }
}

void Alloc(fileStruct* st)
{
  st->path = (char**)malloc(1000 *sizeof(char*));
  for(int i = 0 ; i < 1000; i++)
  st->path[i] = (char*)malloc(512 * sizeof(char));
  st->files = (char**)malloc(1000 *sizeof(char*));
  for(int i = 0 ; i < 1000; i++)
  st->files[i] = (char*)malloc(512 * sizeof(char));
  st->pathFile = (char*)malloc(512* sizeof(char));
  st->sizeofFiles = 0;
  st->sizeofPath = 0;
  st->currentReadFilePoint = 0;
  st->currentReadPathPoint = 0;
 
}
int sizeofStruct = 0;
fileStruct* myStruct;
pthread_t* threads;
void* pThread(void* args)
{
   
  fileStruct* myS = args;
  pthread_mutex_lock(&lock);
  int id;
  for(id = 0 ; id < sizeofStruct; id++)
     if(myS[id].my_state == 0)
       break;  
  myS[id].my_state = 1;
  ReadPaths(&myS[id]);
  myS[id].my_state = 2;
  if(myS[id].my_state == 3)
  {
   pthread_mutex_unlock(&lock);
  }
  else
 {
  myS[id].my_state = 2;
  pthread_mutex_unlock(&lock);
  }
}
void DeAlloc(fileStruct* st)
{
  for(int i = 0; i < 1000; i++)
    free(st->path[i]);
  free(st->path);
  for(int i = 0 ; i < 1000; i++)
     free(st->files[i]);
  free(st->files);
  free(st->pathFile);
 
  st->sizeofFiles = 0;
  st->sizeofPath = 0;
  st->currentReadFilePoint = 0;
  st->currentReadPathPoint = 0;
 
}
void MenuThread(char* line)
{
    char* p = strtok(line," ");
    if(p != NULL && strcmp(p,"da"))
      printf("INVALID COMMAND LINE\n");
    else
    {
      
     p = strtok(NULL, " ");
     if(p != NULL)
     {
     if(!strcmp(p,"-a"))
     {
     
     p = strtok(NULL, " ");
     if(p == NULL)
     {
     printf("INVALID SENTENCE\n");
     return;
     }
     for(int i = 0 ; i < sizeofStruct; i++)
        if(strstr(p,myStruct[i].pathFile) != NULL)
         {
          printf("%s is already included in %s\n",p,myStruct[i].pathFile);
          return;
         }
     strcpy(myStruct[sizeofStruct].pathFile,p);
     p = strtok(NULL, " ");
     if(p == NULL)
     {
     printf("INVALID SENTENCE\n");
     return;
     }
     
     p = strtok(NULL, " ");
     if(p == NULL)
     {
     printf("INVALID SENTENCE\n");
     return;
     }
     int priority = atoi(p);
     myStruct[sizeofStruct].priority = priority;
     myStruct[sizeofStruct].id = sizeofStruct + 1;
     int i = 0;
     for(i ; i < sizeofStruct; i++)
        if(myStruct[i].priority < myStruct[sizeofStruct].priority)
          break;
     if(myStruct[i].priority < myStruct[sizeofStruct].priority)
     {
        /*
	da -a Desktop/s0b/src -p 2
	da -a Desktop/s0b/_build -p 2
	da -a Desktop/s0b/bin -p 3
	*/ 
         swapFunc(&myStruct[sizeofStruct + 1],&myStruct[sizeofStruct]);
         for(int j = sizeofStruct; j > i; j--)
           swapFunc(&myStruct[j],&myStruct[j-1]);
         swapFunc(&myStruct[i],&myStruct[sizeofStruct + 1]);
     }
     pthread_create(&threads[sizeofStruct],NULL,pThread,myStruct);
     sizeofStruct++;
  
     }
     else if(!strcmp(p,"-S"))
     {
      p = strtok(NULL," ");
      if(p == NULL)
      {
      printf("Invalid Sentence \n");
      return;
      }
      int i = 0;
      int id = atoi(p);
      for(i = 0; i < sizeofStruct; i++)
         if(myStruct[i].id == id)
           break;
     if(myStruct[i].id != id)
     {
     printf("Invalid ID\n");
     return;
     }
     myStruct[i].my_state = 3;
     printf("Reading the path with id : %d has been suspended\n",id);
     }
     else if(!strcmp(p,"-R"))
     {
      p = strtok(NULL, " ");
      if(p == NULL)
      {
      printf("INVALID ID");
      return;
      }
      int i = 0;
      int id = atoi(p);
      if(id < 1)
        return;
      for(i = 0; i < sizeofStruct; i++)
         if(myStruct[i].id == id)
           break;
     if(myStruct[i].id != id)
     {
     printf("Invalid ID\n");
     return;
     }
     if(myStruct[i].my_state != 3)
     {
     printf("ID : %d is not suspended\n",id);
     return;
     }
     myStruct[i].my_state = 0;
     float percent = (float)myStruct[i].currentReadFilePoint / (float)myStruct[i].sizeofFiles;
     percent *= 100;
     printf("Id : %d left at %0.1f\n has been Resumed\n",id,percent);
     pthread_create(&threads[i],NULL,pThread,myStruct);
     pthread_join(threads[i],NULL);
     
     }
     else if(!strcmp(p,"-r"))
     {
      p = strtok(NULL," ");
      if(p == NULL)
      {
      printf("Invalid Sentence \n");
      return;
      }
      int i = 0;
      int id = atoi(p);
      for(i; i < sizeofStruct; i++)
        if(myStruct[i].id == id)
           break;
     if(myStruct[i].id != id)
     {
      printf("Invalid ID \n");
      return;
     }
     for(i; i < sizeofStruct - 1;i++)
        swapFunc(&myStruct[i],&myStruct[i+1]);
     sizeofStruct--;
     printf("Removed path with id : %d\n",id);
     }
      else if(!strcmp(p,"-i"))
     {
   
      p = strtok(NULL, " ");
      if(p != NULL)
      {
      int id = atoi(p);
      int valid = -1;
      for(int i = 0 ; i < sizeofStruct; i++)
         if(myStruct[i].id == id)
          {
           valid = i;
           break;
          }
      if(valid == -1)
      {
       printf("INVALID ID\n");
       return;
      }
      printf("ID %d STATUS : %d in PATH : %s\n",myStruct[valid].id, myStruct[valid].my_state,myStruct[valid].pathFile);
      }
     }
      else if(!strcmp(p,"-l"))
     {
      for(int i = 0 ; i < sizeofStruct;i++)
       PrintDetails(&myStruct[i],0);
     }
     else if(!strcmp(p,"-p"))
     {
     p = strtok(NULL, " ");
     if(p == NULL)
     {
       printf("INVALID SENTENCE \n");
       return;
     }
     int id = atoi(p);
     int valid = -1;
        for(int i = 0 ; i < sizeofStruct; i++)
         if(myStruct[i].id == id)
          {
           valid = i;
           break;
          }
     if(valid == -1)
      {
      printf("INVALID ID \n");
      return;
      }
      PrintDetails(&myStruct[valid],1);
     }
      else
    {
    printf("INVALID COMMAND LINE \n");
    }
     }
     else
    {
    printf("INVALID COMMAND LINE \n");
    }
    } 
}

int xa = 0;
char line[1024];
void  execute(char *argv)
{
     pid_t  pid;
     int    status;

     if ((pid = fork()) < 0) 
          exit(1);
     else if (pid == 0) {     
             MenuThread(line);   
     }
     else 
     {
        while(wait(&status) != pid);
     }
} 
 
int main(int c, char* v[])
{
    if(pthread_mutex_init(&lock,NULL) != 0)
    {
      printf("FAILED TO INIT");
      return 0;
    }
    myStruct = (fileStruct*)malloc(100 * sizeof(fileStruct));
    threads = (pthread_t*)malloc(100 * sizeof(pthread_t));
    
for(int i = 0 ; i < 100; i++)
    Alloc(&myStruct[i]);
  
    while(1)
    {
    printf("$> : ");
    gets(line);
    printf("\n");
    if(!strcmp(line,"da -exit"))
    {
     printf("Leaving...\n");
     break;
    }
    else
    execute(line);

    }
    for(int i = 0 ; i < 100; i++)
     DeAlloc(&myStruct[i]);
    free(myStruct);
    free(threads);
    pthread_mutex_destroy(&lock);
 return 0;
}