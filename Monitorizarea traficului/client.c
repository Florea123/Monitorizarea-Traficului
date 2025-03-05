#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;
char strazi[10][101];
/* portul de conectare la server*/
int port;
int ok=0;
char name[100];

void *primire_notificare(void *sock_desc) 
{
    int sock = *(int*)sock_desc;
    char notification[100];
    
    while(ok) 
    {
        memset(notification, 0, sizeof(notification));
        read(sock, notification, sizeof(notification)-1);
        notification[strlen(notification)] = '\0';
        printf("%s", notification);
        fflush(stdout);
    }
    return NULL;
}

void *actualizare_viteza_strada(void *sock_desc) {
    int sock = *(int*)sock_desc;
    char message[256];
    srand(time(NULL));
    
    while(ok) {
        // Generate random speed between 0-100 km/h
        int speed = 10+rand() % 91;
        // Select random street
        char strada[100];
        int j;
        j=rand()%5+1;
        strcpy(strada,strazi[j]);
        int aux=speed,nr=1,i=0;
        char sp[100];
        while(aux!=0)
        {
          nr=nr*10;
          aux=aux/10;
        }
        nr=nr/10;
        aux=speed;
        while(nr!=0)
        {
          sp[i]=aux/nr+'0';
          aux=aux%nr;
          nr=nr/10;
          i++;
        }
        strcpy(message,"/update");
        strcat(message," ");
        strcat(message, sp);
        strcat(message," ");
        strcat(message,strada);
        message[strlen(message)]='\0';
        printf("\nTe deplasezi cu viteza de: %d km/h pe strada %s\n", speed, strada);
        message[strlen(message)]='a';
        message[strlen(message)]='\0';
        write(sock,message,strlen(message));
        sleep(60);
        memset(message, 0, sizeof(message));
        memset(sp,0,sizeof(sp));
        memset(strada,0,sizeof(strada));
    }
    return NULL;
}

int main (int argc, char *argv[])
{
  strcpy(strazi[1],"Dacia");
  strcpy(strazi[2],"Zimbru");
  strcpy(strazi[3],"Bicaz");
  strcpy(strazi[4],"Alexandru");
  strcpy(strazi[5],"Tudor");
  int pid;
  int sd;			// descriptorul de socket
  int login=0;
  struct sockaddr_in server;	// structura folosita pentru conectare 
  char msg[100];		// mesajul trimis
  char not[100];
  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[client] Eroare la socket().\n");
      return errno;
    }
  

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }
  printf("\nBine ai venit in aplicatia mea!\n\n");
while(1)
{
  /* citirea mesajului */
  bzero (msg, 100);
  printf ("Introdu una din comenzile: /register <nume> || /login <nume> pentru a te conecta\n");
  fflush (stdout);
  read (0, msg, 100);
  
  /* trimiterea mesajului la server */
  if (write (sd, msg, 100) <= 0)
    {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }
  if(strcmp(msg,"/register")>0||strcmp(msg,"/login")>0)
  {
    char *p=strtok(msg," ");
    p=strtok(NULL," ");
    strcpy(name,p);
  }
  /* citirea raspunsului dat de server 
     (apel blocant pina cind serverul raspunde) */
  if (read (sd, msg, 100) < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
    }
    if(strcmp(msg,"Te-ai conectat cu succes!")==0||strcmp(msg,"Te-ai inregistrat cu succes!")==0)
  {
    ok=1;
    pthread_t thread_id,thread_speed_strada;
    pthread_create(&thread_id, NULL, primire_notificare, (void*)&sd);
    pthread_create(&thread_speed_strada, NULL, actualizare_viteza_strada, (void*)&sd);
    printf("[client]Mesajul primit de la server: %s\n", msg);
    while(1)
    {
       /* citirea mesajului */
  bzero (msg, 100);
  printf("Introdu comanda:\n");
  printf("/accident pentru a declara ca a avut loc un accident pe strada ta\n");
  printf("/subscribe pentru a afla vremea, cele mai noi oferte la peco,evenimente sportive\n");
  printf("/unsubscribe pentru a nu mai afla informatii despre vreme,oferte peco si sport\n");
  printf("/logout pentru a te deconecta\n\n");
  fflush (stdout);
  read (0, msg, 100);
  
  /* trimiterea mesajului la server */
  if (write (sd, msg, 100) <= 0)
    {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }

  /* citirea raspunsului dat de server 
     (apel blocant pina cind serverul raspunde) */
  if (read (sd, msg, 100) < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
    }
  /* afisam mesajul primit */
  if(strcmp(msg,"Viteza si locatia au fost actualizate cu succes")!=0)
    printf ("[client]Mesajul primit de la server: %s\n", msg);
  else
    memset(msg,0,sizeof(msg));
  if(strcmp(msg,"Te-ai deconectat cu succes\n")==0) 
  {
    ok=0;
    sleep(1);
    break;
  }
  memset(msg,0,sizeof(msg));
    }
  }
  else
    printf ("[client]Mesajul primit de la server: %s\n", msg);
  memset(msg,0,sizeof(msg));
}
  /* inchidem conexiunea, am terminat */
  close (sd);
}