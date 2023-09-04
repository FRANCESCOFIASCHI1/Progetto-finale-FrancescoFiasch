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


ssize_t  /* Read "n" bytes from a descriptor */
readn(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nread;
 
   nleft = n;
   while (nleft > 0) {
     if((nread = read(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount read so far */
     } else if (nread == 0) break; /* EOF */
     nleft -= nread;
     ptr   += nread;
   }
   return(n - nleft); /* return >= 0 */
}

// crea un oggetto di tipo entry
// con chiave s e valore n
ENTRY *crea_entry(char *s, int n) {
  ENTRY *e = malloc(sizeof(ENTRY));
  if (e == NULL)
    termina("errore malloc entry 1");
  e->key = strdup(s); // salva copia di s
  e->data = (int *)malloc(sizeof(int));
  if (e->key == NULL || e->data == NULL)
    termina("errore malloc entry 2");
  *((int *)e->data) = n;
  return e;
}

void distruggi_entry(ENTRY *e) {
  free(e->key);
  free(e->data);
  free(e);
}



int my_byte_to_integer(char* bytes) {
  int value = 0;
  for (int i = 0; i < sizeof(int); i++) {
    value += bytes[i] * (pow(8,i));
  }
  return value;
}