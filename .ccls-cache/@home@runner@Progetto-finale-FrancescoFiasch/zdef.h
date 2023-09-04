#define _GNU_SOURCE /* See feature_test_macros(7) */
#include "xerrori.h"
#include <assert.h> // permette di usare la funzione assert
#include <errno.h>
#include <search.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h> // gestisce tipo bool (variabili booleane)
#include <stdio.h>   // permette di usare scanf printf etc ...
#include <stdlib.h>  // conversioni stringa/numero exit() etc ...
#include <string.h>  // confronto/copia/etc di stringhe
#include <unistd.h>  // per sleep

// Funzioni manipolazione ENTRY
ENTRY *crea_entry(char *s, int n);
void distruggi_entry(ENTRY *e);
ssize_t readn(int,void *ptr, size_t n);
int my_byte_to_integer(char* bytes);

#define Num_elem 1000000

// definisco costanti
#define PC_buffer_len 10
#define PORT 56697
#define Max_sequence_length 2048
const char *delim = ".,:; \n\r\t";

/*-------CAPOSCRITTORE-------*/
//struttura dati del thread caposcrittore
typedef struct datecaposc {
  pthread_t *scrittori;
  int *caposcindex;  // indice nel buffer
  char **buffer;
  pthread_mutex_t *mutex_caposc_buffer;    //forse non è necessario
  sem_t *sem_caposc_free;
  sem_t *sem_caposc_data;
} datecaposc;

/*-------CAPOLETTORE-------*/
//struttura dati del thread capolettore
typedef struct datecapolet {
  int capolet;    //named pipe
  pthread_t *lettori;
  int *capoletindex;  // indice nel buffer
  char **buffer;
  pthread_mutex_t *mutex_capolet_buffer;    //forse non è necessario
  sem_t *sem_capolet_free;
  sem_t *sem_capolet_data;
} datecapolet;

/*-------Thread SCRITTORI-------*/
//struttura dati dei thread comuni scrittori
typedef struct thread_scritto {
  pthread_cond_t *cv;  // condition variables condiviso con thread lettori
  int *scrittoindex;  // indice nel buffer
  char **buffer;
  pthread_mutex_t *scrittori_mutex;    // mutex condiviso con thread lettori
  sem_t *sem_caposc_free;
  sem_t *sem_caposc_data;
} TS;

/*-------Thread LETTORI-------*/
//struttura dati dei thread comuni lettore
typedef struct thread_letto {
  pthread_cond_t *cv;  // condition variables condiviso con thread lettori
  int *lettoindex;  // indice nel buffer
  char **buffer;
  pthread_mutex_t *lettori_mutex;    // mutex condiviso con thread lettori
  sem_t *sem_capolet_free;
  sem_t *sem_capolet_data;
} TL;