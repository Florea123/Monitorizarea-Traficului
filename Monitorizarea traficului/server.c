#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "database.h"
#include <pthread.h>

/* portul folosit */

#define PORT 2715

extern int errno;		/* eroarea returnata de unele apeluri */
char strazi[10][101];
int accident[101];
int ok=1;
char accident_notification[100];
pthread_t thread_accident;

static void *notificare_abonat(void *arg)
{
    while(1) 
    {
        sqlite3_stmt* stmt;
        char* sql = "SELECT client_id FROM users WHERE subscribed = 1 AND is_logged_in = 1;";
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) 
        {
            while(sqlite3_step(stmt) == SQLITE_ROW) 
            {
                int client_id = sqlite3_column_int(stmt, 0);
                char notification[] = "\nVremea e de 20 de grade C\nPretul la benzina este de 7.20/L si la motorina 7.45/L\nCampionatul de fotball va avea loc duminica asta la ora 21:00\n";
                int len = strlen(notification);
                write(client_id, notification, len);
            }
            sqlite3_finalize(stmt);
        }
        sleep(60); // Reduced sleep time for more responsive notifications
    }
    return NULL;
}

static void *notificare_limita_viteza(void *arg)
{
    while(1) 
    {
        sqlite3_stmt* stmt;
        char* sql = "SELECT client_id, street FROM users WHERE is_logged_in = 1;";
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) 
        {
            while(sqlite3_step(stmt) == SQLITE_ROW) 
            {
              char message[101];
                int client_id = sqlite3_column_int(stmt, 0);
                char* strada = (char*)sqlite3_column_text(stmt, 1);
                int j = 0;
                for(int i = 1; i <= 5; i++) {
                    if(strcmp(strazi[i], strada) == 0) 
                    {
                        j = i;
                        break;
                    }
                }
                if(accident[j] == 1)
                  strcpy(message,"\nEste un accident pe strada ta! Viteza recomandata este de 10 km/h\n");
                else
                  strcpy(message,"\nViteza maxima este de 50 km/h\n");
                write(client_id, message, strlen(message));

            }
            sqlite3_finalize(stmt);
        }
        sleep(60); 
    }
    return NULL;
}
static void *resetare_accidente(void *arg) 
{
    while(1) {
        for(int i = 1; i <= 5; i++) 
            if(accident[i] == 1)
                accident[i] = 0;
        sleep(120);
    }
    return NULL;
}
void *notificare_accident(void *arg) {

    sqlite3_stmt* stmt;
    char* sql = "SELECT client_id FROM users WHERE is_logged_in = 1;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) 
    {
        while(sqlite3_step(stmt) == SQLITE_ROW) 
        {
            int client_id = sqlite3_column_int(stmt, 0);
            char notification[256];
            strcpy(notification, "Este un accident pe strada ");
            strcat(notification, accident_notification);
            strcat(notification,"!");
            notification[strlen(notification)]='\0';
            write(client_id, notification, strlen(notification));
        }
        sqlite3_finalize(stmt);
    }
    return NULL;
}
/* functie de convertire a adresei IP a clientului in sir de caractere */
char * conv_addr (struct sockaddr_in address)
{
  static char str[25];
  char port[7];

  /* adresa IP a clientului */
  strcpy (str, inet_ntoa (address.sin_addr));	
  /* portul utilizat de client */
  bzero (port, 7);
  sprintf (port, ":%d", ntohs (address.sin_port));	
  strcat (str, port);
  return (str);
}
int ver[101];
int login[101];
FILE *file; 
/* programul */
int main ()
{
  strcpy(strazi[1],"Dacia");
  strcpy(strazi[2],"Zimbru");
  strcpy(strazi[3],"Bicaz");
  strcpy(strazi[4],"Alexandru");
  strcpy(strazi[5],"Tudor");
  accident[1]=0;
  accident[2]=0;
  accident[3]=0;
  accident[4]=0;
  accident[5]=0;
  struct sockaddr_in server;	/* structurile pentru server si clienti */
  struct sockaddr_in from;
  fd_set readfds;		/* multimea descriptorilor de citire */
  fd_set actfds;		/* multimea descriptorilor activi */
  struct timeval tv;		/* structura de timp pentru select() */
  int sd, client;		/* descriptori de socket */
  int optval=1; 			/* optiune folosita pentru setsockopt()*/ 
  int fd;			/* descriptor folosit pentru 
				   parcurgerea listelor de descriptori */
  int len;			/* lungimea structurii sockaddr_in */
  int nfds;
  /* creare socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server] Eroare la socket().\n");
      return errno;
    }
  /*setam pentru socket optiunea SO_REUSEADDR */ 
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(optval));

  /* pregatim structurile de date */
  bzero (&server, sizeof (server));

  /* umplem structura folosita de server */
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (PORT);

  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server] Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 5) == -1)
    {
      perror ("[server] Eroare la listen().\n");
      return errno;
    }
  
  /* completam multimea de descriptori de citire */
  FD_ZERO (&actfds);		/* initial, multimea este vida */
  FD_SET (sd, &actfds);		/* includem in multime socketul creat */

  tv.tv_sec = 1;		/* se va astepta un timp de 1 sec. */
  tv.tv_usec = 0;
  
  /* valoarea maxima a descriptorilor folositi */
  nfds = sd;
  init_database();
  printf ("[server] Asteptam la portul %d...\n", PORT);
  fflush (stdout);  
  /* servim in mod concurent (!?) clientii... */
  pthread_t thread_abonat,thread_limitare_viteza,thread_resetare_accidente;
  pthread_create(&thread_abonat, NULL, &notificare_abonat, NULL);
  pthread_create(&thread_limitare_viteza, NULL, &notificare_limita_viteza, NULL);
  pthread_create(&thread_resetare_accidente, NULL, &resetare_accidente, NULL);
  while (1)
    {
      /* ajustam multimea descriptorilor activi (efectiv utilizati) */
      bcopy ((char *) &actfds, (char *) &readfds, sizeof (readfds));

      /* apelul select() */
      if (select (nfds+1, &readfds, NULL, NULL, &tv) < 0)
	{
	  perror ("[server] Eroare la select().\n");
	  return errno;
	}
      /* vedem daca e pregatit socketul pentru a-i accepta pe clienti */
      if (FD_ISSET (sd, &readfds))
	{
	  /* pregatirea structurii client */
	  len = sizeof (from);
	  bzero (&from, sizeof (from));

	  /* a venit un client, acceptam conexiunea */
	  client = accept (sd, (struct sockaddr *) &from, &len);

	  /* eroare la acceptarea conexiunii de la un client */
	  if (client < 0)
	    {
	      perror ("[server] Eroare la accept().\n");
	      continue;
	    }

          if (nfds < client) /* ajusteaza valoarea maximului */
            nfds = client;
            
	  /* includem in lista de descriptori activi si acest socket */
	  FD_SET (client, &actfds);

	  printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n",client, conv_addr (from));
	  fflush (stdout);
	}
      /* vedem daca e pregatit vreun socket client pentru a trimite raspunsul */
      for (fd = 0; fd <= nfds; fd++)	/* parcurgem multimea de descriptori */
	{
	  /* este un socket de citire pregatit? */
	  if (fd != sd && FD_ISSET (fd, &readfds))
	    {	
	      if (sayHello(fd)==0)
		{
		  printf ("[server] S-a deconectat clientul cu descriptorul %d.\n",fd);
      if(login[fd]==1)
      {
		  login[fd]=0;
      char* name=get_username_by_client_id(fd);
      logout_user(name);
      set_client_id(name, -1);
      }
      fflush (stdout);
		  close (fd);		/* inchidem conexiunea cu clientul */
		  FD_CLR (fd, &actfds);/* scoatem si din multime */
		}
	    }
	}			/* for */
    }				/* while */
close_database();
}				/* main */

/* realizeaza primirea si retrimiterea unui mesaj unui client */
int sayHello(int fd)
{
  char buffer[100];		/* mesajul */
  int bytes;			/* numarul de octeti cititi/scrisi */
  char msg[100];		//mesajul primit de la client 
  char msgrasp[100]=" ";        //mesaj de raspuns pentru client
  if(login[fd]==1)
  {
  bytes = read (fd, msg, sizeof (buffer));
  if (bytes == 0)
    {
      return 0;
    }
    if (bytes < 0)
  {
    perror("[server] Eroare la read() de la client.\n");
    return 0;  // Dacă citirea a eșuat
  }
  char s[100];
  strcpy(s,msg);
  s[7]='\0';
  if(strcmp(s,"/update")!=0)
  printf ("[server]Mesajul a fost trimis de catre clientul:%d cu mesajul %s\n", fd,msg);
      
  /*pregatim mesajul de raspuns */
  bzero(msgrasp,100);
  msg[strlen(msg)-1]='\0';
  if(strcmp(msg,"/subscribe")==0)
  {
    if(is_user_subscribed(get_username_by_client_id(fd))==1)
    {
      strcpy(msgrasp,"Esti deja abonat!");
    }
    else
    {
      subscribe_user(get_username_by_client_id(fd));
      strcpy(msgrasp,"Te-ai abonat cu succes!");
    }
  }
  else
  if(strcmp(msg,"/unsubscribe")==0)
  {
    if(is_user_subscribed(get_username_by_client_id(fd))==0)
    {
      strcpy(msgrasp,"\nNu esti abonat!\n");
    }
    else
    {
      unsubscribe_user(get_username_by_client_id(fd));
      strcpy(msgrasp,"Te-ai dezabonat cu succes!\n");
    }
  }
  else
    if(strcmp(msg,"/logout")==0)
  {
    char* name=get_username_by_client_id(fd);
    strcpy(msgrasp,"Te-ai deconectat cu succes\n");
    login[fd]=0;
    logout_user(name);
    set_client_id(name, -1);
  }
  else
    if(strcmp(msg,"/accident")==0)
{
    strcpy(accident_notification,get_user_street(get_username_by_client_id(fd)));
    for(int i=1;i<=5;i++)
      if(strcmp(accident_notification,strazi[i])==0)
        {
          accident[i]=1;
          break;
        }
    pthread_create(&thread_accident, NULL, notificare_accident, NULL);
    strcpy(msgrasp,"Accidentul a fost raportat cu succes!");
}
  else
    if(strcmp(msg,"/update")>0)
    {
      char* p=strtok(msg," ");
      p=strtok(NULL," ");
      char speed[100];
      int sp=0;
      strcpy(speed,p);
      speed[strlen(speed)]='\0';
      for(int i=0;i<strlen(speed);i++)
          sp=sp*10+(speed[i]-'0');
      update_user_speed(get_username_by_client_id(fd), sp);
      p=strtok(NULL," ");
      update_user_street(get_username_by_client_id(fd),p);
      printf("Utilizatorul %d se deplaseaza cu viteza de %d km/h si se afla pe strada %s\n",fd,sp,p);
      strcpy(msgrasp,"");
    }
  else
    strcpy(msgrasp,"Comanda introdusa nu exista");
  if(strcmp(s,"/update")!=0)
    printf("[server]Trimitem mesajul inapoi catre clientul %d cu mesajul: %s\n",fd,msgrasp);
      
  if (bytes && write (fd, msgrasp, bytes) < 0)
    {
      perror ("[server] Eroare la write() catre client.\n");
      return 0;
    }
  }
  else
  {
    bytes = read (fd, msg, sizeof (buffer));
  if (bytes == 0)
    {
      return 0;
    }
    if (bytes < 0)
  {
    perror("[server] Eroare la read() de la client.\n");
    return 0;  // Dacă citirea a eșuat
  }
  printf ("[server]Mesajul a fost trimis de catre clientul:%d cu mesajul %s\n", fd,msg);
      
  /*pregatim mesajul de raspuns */
    bzero(msgrasp,100);
    char *p;
    p=strtok(msg," ");
    if(strcmp(p,"/register")==0)
{
    p=strtok(NULL," ");
    p[strlen(p)-1]='\0';
    if (check_user_exists(p)==1) {
        strcpy(msgrasp,"Username-ul pe care l-ai introdus deja exista");
    }
    else {
        if (add_user(p)) {
            printf("%s\n",p);
            strcpy(msgrasp,"Te-ai inregistrat cu succes!");
            set_user_logged_in(p, 1);
            set_client_id(p, fd);
            login[fd]=1;
        }
    }
}
    else
      if(strcmp(p,"/login")==0)
{
    p=strtok(NULL," ");
     p[strlen(p)-1]='\0';
    if (check_user_exists(p)==0)
    {
        strcpy(msgrasp,"Username-ul pe care l-ai introdus nu exista");
    }
    else 
    {
        if (check_user_logged_in(p)) 
        {
            strcpy(msgrasp,"Deja este cineva conectat pe acest cont");
        }
        else 
        {
            if (set_user_logged_in(p, 1)) 
            {
                strcpy(msgrasp,"Te-ai conectat cu succes!");
                login[fd]=1;
                set_client_id(p, fd);
            } 
            else 
            {
                strcpy(msgrasp,"Eroare la conectare");
            }
        }
    }
}
      else
       strcpy(msgrasp,"Mai intai trebuie sa te conectezi pentru a folosi aplicatia");
  printf("[server]Trimitem mesajul inapoi catre clientul %d cu mesajul %s\n",fd,msgrasp);
      
  if (bytes && write (fd, msgrasp, bytes) < 0)
    {
      perror ("[server] Eroare la write() catre client.\n");
      return 0;
    }
  }
  memset(msg,0,sizeof(msg));
  memset(msgrasp,0,sizeof(msgrasp));
  return 1;
}