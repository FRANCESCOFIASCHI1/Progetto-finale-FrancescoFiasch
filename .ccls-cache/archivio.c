#include "zdef.h"      //file con definizioni utili al programma

int n_lettori = 0;
int numero_stringhe = 0;
pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER; // inizializzazione della condition variables
int conta(char *s);

void aggiungi(char *s) {
  ENTRY *e = crea_entry(s, 1);
  xpthread_mutex_lock(&heap_mutex,__LINE__,__FILE__);
  while(n_lettori>0)
    pthread_cond_wait(&c,&heap_mutex);
  ENTRY *r = hsearch(*e, FIND);
  if (r == NULL) { // la entry è nuova
    //printf("-----FIND NULL-------------\n");
    r = hsearch(*e, ENTER);
    numero_stringhe += 1;    //variabile globale
    if (r == NULL)
      termina("errore o tabella piena");
  } else {
    //printf("-----FIND R->KEY-------------\n");
    // la stringa è gia' presente incremento il valore
    assert(strcmp(e->key, r->key) == 0);
    int *d = (int *)r->data;
    *d += 1;
    distruggi_entry(e); // questa non la devo memorizzare
  }
  xpthread_mutex_unlock(&heap_mutex,__LINE__,__FILE__);
}

int conta(char *s) {
  int ris = 0;
  ENTRY *e = crea_entry(s, 1);
  n_lettori += 1;
  xpthread_mutex_lock(&heap_mutex,__LINE__,__FILE__);
  ENTRY *r = hsearch(*e, FIND);
  pthread_cond_broadcast(&c);
  if (r != NULL) { // la entry esiste di già
    // la stringa è gia' presente ritorno il valore associato ad s
    int *d = (int *)r->data;
    ris = *d;
    //printf("Stringhe Presenti: %d\n",ris);
    distruggi_entry(e); // questa non la devo memorizzare
  }
  n_lettori -= 1;
  xpthread_mutex_unlock(&heap_mutex,__LINE__,__FILE__);
  return ris;
}

/*-------BODY CAPO SCRITTORE-------*/
void *caposc_body(void *arg) {
  datecaposc *a = (datecaposc *)arg;
  int caposc;
  caposc = open("caposc", O_RDONLY);    //leggo 4 byte che mi danno la lunghezza della stringa
  if(caposc == -1)
    printf("ERRORE OPEN");
  char *byte;
  byte = malloc(Max_sequence_length*sizeof(char));
  char *string;
  while (true){
    //printf("--------------------------------------\n");
    ssize_t bytes_read = readn(caposc, byte, 4);
    //printf("Bytes_read: %lo\n",bytes_read);
    if(bytes_read == 0)    //controllo se la pipe è chiusa
      break;
    if(bytes_read == -1)
      printf("ERRORE READ");
    int lun = my_byte_to_integer(byte);
    //printf("Lun = %d\n",lun);
    if(lun == 0)
      break;
    
    bytes_read = readn(caposc, byte, lun);
    byte[bytes_read] = '\0';
    //printf("Stringa_Letta_SCR: %s\n", byte);
    char *saveptr;
    string = strtok_r(byte,delim,&saveptr);
    
    xsem_wait(a->sem_caposc_free,__LINE__,__FILE__);
    a->buffer[*(a->caposcindex) % PC_buffer_len] = strdup(string);
    //-------------------
    //printf("CapoSCR2: %s\n",a->buffer[*(a->caposcindex) % PC_buffer_len]);    //da levare
    //-------------------
    *(a->caposcindex) += 1;
    xsem_post(a->sem_caposc_data,__LINE__,__FILE__);
        
    
    while(true) {
      string = strtok_r(NULL,delim,&saveptr);
      if(string == NULL)
        break;
      
      xsem_wait(a->sem_caposc_free,__LINE__,__FILE__);
      a->buffer[*(a->caposcindex) % PC_buffer_len] = strdup(string);
      //-------------------
      //printf("CapoSCR2: %s\n",a->buffer[*(a->caposcindex) % PC_buffer_len]);    //da levare
      //-------------------
      *(a->caposcindex) += 1;
      xsem_post(a->sem_caposc_data,__LINE__,__FILE__);
    }
  }
  close(caposc);
  free(byte);
  free(string);
  pthread_exit(NULL);
}

/*-------BODY Thread SCRITTORI-------*/
// codice dei thread scrittori
void *writer_body(void *arg) {
  TS *a = (TS *)arg;
  char *string;
  while(true) {
    xsem_wait(a->sem_caposc_data,__LINE__,__FILE__);
		xpthread_mutex_lock(a->scrittori_mutex,__LINE__,__FILE__);
    string = strdup(a->buffer[*(a->scrittoindex) % PC_buffer_len]);
    *(a->scrittoindex) +=1;
		xpthread_mutex_unlock(a->scrittori_mutex,__LINE__,__FILE__);
    xsem_post(a->sem_caposc_free,__LINE__,__FILE__);
    
    if(strcmp(string,"\0")==0)
      break;
        
    //-------------------
    //printf("Writer: %s\n",string);
    //-------------------
    aggiungi(string);
  }
  free(string);
  pthread_exit(NULL);
}


/*-------BODY CAPO LETTORE-------*/
void *capolet_body(void *arg) {
  datecapolet *a = (datecapolet *)arg;
  int capolet;
  capolet = open("capolet", O_RDONLY);    //leggo 4 byte che mi danno la lunghezza della stringa
  if(capolet == -1)
    printf("ERRORE OPEN");
  char *byte;
  byte = malloc(Max_sequence_length*sizeof(char));
  char *string;
  while (true){
    //printf("--------------------------------------\n");
    ssize_t bytes_read = readn(capolet, byte, 4);
    //printf("Bytes_read: %lo\n",bytes_read);
    if(bytes_read == 0)    //controllo se la pipe è chiusa
      break;
    if(bytes_read == -1)
      printf("ERRORE READ");
    int lun = my_byte_to_integer(byte);
    //printf("Lun = %d\n",lun);
    if(lun == 0)
      break;
    bytes_read = readn(capolet, byte, lun);
    byte[bytes_read] = '\0';
    //printf("Stringa_Letta_LET: %s\n", byte);
    char *saveptr;
    string = strtok_r(byte,delim,&saveptr);
    
    xsem_wait(a->sem_capolet_free,__LINE__,__FILE__);
    a->buffer[*(a->capoletindex) % PC_buffer_len] = strdup(string);
    //-------------------
    //printf("CapoLET2: %s\n",a->buffer[*(a->capoletindex) % PC_buffer_len]);    //da levare
    //-------------------
    *(a->capoletindex) += 1;
    xsem_post(a->sem_capolet_data,__LINE__,__FILE__);

  
    while(true) {
      string = strtok_r(NULL,delim,&saveptr);
      if(string == NULL)
        break;
      xsem_wait(a->sem_capolet_free,__LINE__,__FILE__);
      a->buffer[*(a->capoletindex) % PC_buffer_len] = strdup(string);
      //-------------------
      //printf("CapoLET2: %s\n",a->buffer[*(a->capoletindex) % PC_buffer_len]);    //da levare
      //-------------------
      *(a->capoletindex) += 1;
      xsem_post(a->sem_capolet_data,__LINE__,__FILE__);
    }
  }
  free(byte);
  close(capolet);
  pthread_exit(NULL);
}

/*-------BODY Thread LETTORI-------*/
void *reader_body(void *arg) {
  TL *a = (TL *)arg;
  char *string;
  while(true) {
    xsem_wait(a->sem_capolet_data,__LINE__,__FILE__);
		xpthread_mutex_lock(a->lettori_mutex,__LINE__,__FILE__);
    string = strdup(a->buffer[*(a->lettoindex) % PC_buffer_len]);
    *(a->lettoindex) += 1;
		xpthread_mutex_unlock(a->lettori_mutex,__LINE__,__FILE__);
    xsem_post(a->sem_capolet_free,__LINE__,__FILE__);
    
    if(strcmp(string,"\0")==0)
      break;
        
    //-------------------
    //printf("Reader: %s\n",string);
    //-------------------
    //printf("CONTA -> %s: %d\n",string,conta(string));
    conta(string);
    //parte di lettori.log
    free(string);
  }
  printf("CONTA -> abaco: %d\n",conta("abaco"));
  pthread_exit(NULL);
}

/*
//----------------INIZIO MAIN------------------\\
*/

int main(int argc, char *argv[]) {
  
  // crea tabella hash
  int ht = hcreate(Num_elem);
  if (ht == 0)
    termina("Errore creazione HT");

  int w = atoi(argv[1]); // numero di thread scrittori
  int r = atoi(argv[2]); // numero di thread lettori

  //creazione buffer
  char* buff_caposcrittori[PC_buffer_len];
  char *buff_capolettori[PC_buffer_len];
  

  

  // inizializzazione dati caposcrittore
  datecaposc datecaposc;
  int caposcindex=0;    //indice del buffer caposc
  sem_t sem_caposc_free, sem_caposc_data;
  xsem_init(&sem_caposc_free,0,PC_buffer_len,__LINE__,__FILE__);
  xsem_init(&sem_caposc_data,0,0,__LINE__,__FILE__);
  pthread_t caposcrittore;

  //caposcrittore
  datecaposc.caposcindex = &caposcindex;
  datecaposc.buffer = buff_caposcrittori;
  datecaposc.sem_caposc_data = &sem_caposc_data;
  datecaposc.sem_caposc_free = &sem_caposc_free;
  pthread_create(&caposcrittore,NULL,caposc_body,&datecaposc);
  
  // inizializzazione argomenti thread scrittori
  TS ts[w]; // dati thread scrittori
  int scrittoindex = 0;
  pthread_mutex_t scrittori_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_t scrittori[w]; // Array dei thread srittori
  
  /*-----------Creazione Scrittori-----------*/
  for(int i=0;i<w;i++) {
    ts[i].cv = &c;      // Assegnazione CV
    ts[i].scrittori_mutex = &scrittori_mutex;      // Mutex Condiviso
    ts[i].sem_caposc_data = &sem_caposc_data;
    ts[i].sem_caposc_free = &sem_caposc_free;
    ts[i].buffer = buff_caposcrittori;
    ts[i].scrittoindex = &scrittoindex;
   //inizializzazione della stringa da passare ai thread
    pthread_create(&scrittori[i],NULL,writer_body,ts+i);
  }

  //---------CAPOLETTORE--------------------------------
  // inizializzazione dati capolettore
  datecapolet datecapolet;
  int capoletindex=0;
  sem_t sem_capolet_free, sem_capolet_data;
  xsem_init(&sem_capolet_free,0,PC_buffer_len,__LINE__,__FILE__);
  xsem_init(&sem_capolet_data,0,0,__LINE__,__FILE__);  
  pthread_t capolettore;

  //capolettore
  datecapolet.capoletindex = &capoletindex;
  datecapolet.buffer = buff_capolettori;
  datecapolet.sem_capolet_data = &sem_capolet_data;
  datecapolet.sem_capolet_free = &sem_capolet_free;
  pthread_create(&capolettore,NULL,capolet_body,&datecapolet);

    // inizializzazione argomenti thread lettori
  TL tl[r]; // dati thread scrittori
  int lettoindex = 0;
  pthread_mutex_t lettori_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_t lettori[r];   // Array dei thread lettori

   /*-----------Creazione Lettori-----------*/
  for(int i=0;i<r;i++) {
    tl[i].cv = &c;      // Assegnazione CV
    tl[i].lettori_mutex = &lettori_mutex;      // Mutex Condiviso
    tl[i].sem_capolet_data = &sem_capolet_data;
    tl[i].sem_capolet_free = &sem_capolet_free;
    tl[i].buffer = buff_capolettori;
    tl[i].lettoindex = &lettoindex;
    //inizializzazione della stringa da passare ai thread
    pthread_create(&lettori[i],NULL,reader_body,tl+i);
  }
  
  /*----------TERMINAZIONE CAPOSCRITTORE---------------*/
  // aspetto termini il caposcrittore
  xpthread_join(caposcrittore,NULL,__LINE__,__FILE__);
  fprintf(stderr,"----------   Caposcrittore Terminato ----------\n");
  // invio valore di terminazione per i lettori
  for(int i=0;i<w;i++) {
    xsem_wait(&sem_caposc_free,__LINE__,__FILE__);
    buff_caposcrittori[caposcindex++ % PC_buffer_len] = "\0";
    xsem_post(&sem_caposc_data,__LINE__,__FILE__);
  }
  // join dei thread scrittori
  for(int i=0;i<w;i++)
    xpthread_join(scrittori[i],NULL,__LINE__,__FILE__);
  fprintf(stderr,"----------   Scrittori Terminati     ----------\n");

  /*----------TERMINAZIONE CAPOLETTORE---------------*/
  xpthread_join(capolettore,NULL,__LINE__,__FILE__);
  fprintf(stderr,"----------   Capolettore Terminato   ----------\n");
  // invio valore di terminazione per i lettori

  for(int i=0;i<r;i++) {
    xsem_wait(&sem_capolet_free,__LINE__,__FILE__);
    buff_capolettori[capoletindex++ % PC_buffer_len] = "\0";
    xsem_post(&sem_capolet_data,__LINE__,__FILE__);
  }
  // join dei thread lettori
  for(int i=0;i<r;i++)
    xpthread_join(lettori[i],NULL,__LINE__,__FILE__);
  fprintf(stderr,"----------   Lettori Terminati       ----------\n");

  printf("Numero di stringhe nella tabella hash: %d\n", numero_stringhe);

  /*---------LIBERO MEMORIA---------*/
  pthread_cond_destroy(&c);    //deallocazione condition variables
  xpthread_mutex_destroy(&scrittori_mutex,__LINE__,__FILE__);
  xpthread_mutex_destroy(&lettori_mutex,__LINE__,__FILE__);
  hdestroy();
  return 0;
}