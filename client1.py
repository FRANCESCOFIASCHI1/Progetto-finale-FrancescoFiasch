#! /usr/bin/env python3
# client1

import argparse, logging, os, os.path, sys, struct, socket, threading
from concurrent.futures import ThreadPoolExecutor

# Configurazione dell'indirizzo IP e del numero di porta del server
HOST = "127.0.0.1"  # Indirizzo IP del server
PORT = 56697  # Numero di porta del server


def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks

# thread uno per ogni linea del file passato
def gestisci_connessione(linea,host=HOST,port=PORT):
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((host, port))
    s.sendall(("Tipo A").encode())
    lun = len(linea)
    assert (lun<2048),"ERRORE Lunghezza"
    # invio stringa linea al server
    if not lun==1:
      s.sendall(struct.pack("!i",lun))        # invio lunghezza stringa
      s.sendall(linea.encode())               # invio stringa

def main(name_file, host=HOST,port=PORT):
  #creiamo il server socket
  with ThreadPoolExecutor(len(name_file)) as exe:
    with open(name_file,"r") as file:
      for linea in file:
        exe.submit(gestisci_connessione,linea)

#gestione degli argomenti da linea di comando
parser = argparse.ArgumentParser()
parser.add_argument('arg1', type=str)    # passo un solo file dalla linea di comando
argv = parser.parse_args()
main(argv.arg1)