/*
#include "const.h"
#include "cmd.h"
*/
#include "../common/connection.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>

#define DEFAULT_SIZE 64

using namespace std;

enum CommandType {
    HELP, LIST, QUIT, RETR, STOR, DELE, BAD_REQ
};

CommandType str2cmd(string str){
    if (str.compare("help") == 0)   { return HELP; }
    if (str.compare("ls") == 0)     { return LIST; }
    if (str.compare("q") == 0)      { return QUIT; }
    if (str.compare("get") == 0)    { return RETR; }
    if (str.compare("put") == 0)    { return STOR; }
    if (str.compare("rm") == 0)     { return DELE; }
    return BAD_REQ;
}

void availableCommands(){
    cout << endl;

    cout << "sftp: secure file transfer for file up to 4 GB" << endl;
    cout << "Usage:" << endl;
    cout << " help           -- show this content" << endl;
    cout << " ls             -- list all files available on the server" << endl;
    cout << " q              -- quit" << endl;
    cout << " get <filename> -- download the specified file" << endl;
    cout << " put <filename> -- upload the specified file" << endl;
    cout << " rm <filename>  -- remove the specified file" << endl;

    cout << endl;
}

vector<string> split(string str, char delimeter){
    stringstream ss(str);
    string s;
    vector<string> str_vector;
    while (getline(ss, s, delimeter)){
        str_vector.push_back(s);
    }
    return str_vector;
}

void parseCommand(string str){
    vector<string> parsed_str = split(str, ' ');
    /*
    for (int i = 0; i < parsed_str.size(); i++){
        cout << parsed_str[i] << endl;
    }
    */
    CommandType cmd = str2cmd(parsed_str[0]);

    switch(cmd){
        case HELP: availableCommands(); break;
        case LIST: cout << "LIST" << endl; break;
        case QUIT: cout << "QUIT" << endl; exit(0);
        case RETR: cout << "RETR" << endl; break;
        case STOR: cout << "STOR" << endl; break;
        case DELE: cout << "DELE" << endl; break;
        default:   cout << "BAD_REQ" << endl;
    }
}

void read(const char *filename){
    streampos size;
    char *memblock = 0;

    ifstream file;
    file.open(filename, ios::in|ios::binary|ios::ate);
    if (file.is_open()){
        size = file.tellg();
        cout << "Copying " << size << " bytes in memory\n";
        memblock = new char [size];
        file.seekg (0, ios::beg);
        file.read (memblock, size);
        file.close();
        cout << memblock;
    } else {
        cout << "Unable to open file" << endl;
    }
}

/*
void inviaPorta(){
  int ret, len;
  uint16_t msglen;

  sprintf(buffer, "%d", my_port);
  len = strlen(buffer) + 1;

  msglen = htons(len);

  // invio lunghezza stringa
  ret = send(tcp_sd, (void *)&msglen, sizeof(uint16_t), 0);
  if (ret < 0) {
    perror("send() ");
    exit(-1);
  }

  // invio porta
  ret = send(tcp_sd, (void *)buffer, len, 0);
  if (ret < 0) {
    perror("send() ");
    exit(-1);
  }

  // aggiorno stato del client
  pthread_mutex_lock(&stato_client);
  s = CLIENT_REGISTERED;
  pthread_mutex_unlock(&stato_client);

}


void digitaMessaggio(){
  char riga[DEFAULT_SIZE];
  // pulizia riga
  memset(riga, 0, DEFAULT_SIZE);

  while(strcmp(riga, ".\n") != 0) {
    fgets(riga, sizeof(riga), stdin);

    if (strcmp(riga, ".\n") != 0)
      strcat(buffer, riga);
  }

}


void riceviMessaggiOffline(){
  int ret, len;
  uint16_t msglen, esito;
  int code = 0;

  while(code != MSG_OK){

    ret = recv(tcp_sd, (void *)&esito, sizeof(uint16_t), 0);

    if (ret < sizeof(uint16_t)){
      perror("recv()");
      exit(-1);
    }

    code = ntohs(esito);

    if (code == MSG_PART){
      //pulizia buffer
      memset(buffer, 0, DEFAULT_SIZE);

      ret = recv(tcp_sd, (void *)&msglen, sizeof(uint16_t), 0);

      if (ret < sizeof(uint16_t)){
        perror("recv()");
        exit(-1);
      }

      len = ntohs(msglen);
      // Attendo il messaggio
      ret = recv(tcp_sd, (void*)buffer, len, 0);

      if (ret < 0) {
        perror("recv()");
        exit(-1);
      }

      printf("%s", buffer);

    }
  }
}


void *riceviMessaggiOnline(){
  int ret, addrlen;
  struct sockaddr_in cl_addr;
  char online_msg[DEFAULT_SIZE];

  while(1){
    //pulizia buffer
    memset(online_msg, 0, DEFAULT_SIZE);

    addrlen = sizeof(cl_addr);
    // Attendo il messaggio online
    ret = recvfrom(udp_sd, (void*)online_msg, DEFAULT_SIZE, 0, (struct sockaddr *)&cl_addr, (socklen_t *)&addrlen);

    if (ret < 0) {
      perror("recvfrom() ");
      exit(-1);
    }

    pthread_mutex_lock(&stato_client);
    if (s == CLIENT_REGISTERED){
      pthread_mutex_lock(&output);
      printf("\n%s", online_msg);
      printf("%s> ", whoami); fflush(stdout);
      pthread_mutex_unlock(&output);
    }

    pthread_mutex_unlock(&stato_client);
  }

}

void abilitaRicezioneMessaggiOnline(){
  int ret;

  ret = pthread_create(&thread, NULL, riceviMessaggiOnline, NULL);

  if (ret){
    perror("pthread_create()");
    exit(-1);
  }
}


void inviaMessaggioOffline(){
  int ret, len;
  uint16_t msglen;

  memset(buffer, 0, DEFAULT_SIZE);

  pthread_mutex_lock(&output);
  digitaMessaggio();

  len = strlen(buffer) + 1;
  msglen = htons(len);

  // invio lunghezza messaggio
  ret = send(tcp_sd, (void *)&msglen, sizeof(uint16_t), 0);
  if (ret < 0) {
    perror("send() ");
    exit(-1);
  }

  // invio messaggio
  ret = send(tcp_sd, (void *)buffer, len, 0);
  if (ret < 0) {
    perror("send() ");
    exit(-1);
  }

  riceviEsito();

  pthread_mutex_unlock(&output);

}


void inviaMessaggioOnline(struct destinatario *d){
  int ret;
  struct sockaddr_in cl_addr;

  // creazione indirizzo client
  memset(&cl_addr, 0, sizeof(cl_addr));
  cl_addr.sin_family = AF_INET;
  cl_addr.sin_port = htons(d->port);
  inet_pton(AF_INET, d->addr, &cl_addr.sin_addr);

  memset(buffer, 0, DEFAULT_SIZE);
  sprintf(buffer, "%s (msg istantaneo)>\n", whoami);

  pthread_mutex_lock(&output);
  digitaMessaggio();

  // invio lunghezza messaggio non è necessaria perchè UDP invia il pacchetto e non si preoccupa dell'ordine
  // invio messaggio
  ret = sendto(udp_sd, (void *)buffer, DEFAULT_SIZE, 0, (struct sockaddr *)&cl_addr, sizeof(cl_addr));
  if (ret < 0) {
    perror("send() ");
    exit(-1);
  }

  printf("Messaggio istantaneo inviato\n");

  pthread_mutex_unlock(&output);
}


struct destinatario riceviInfoDestinatario(){
  int len, ret;
  uint16_t msglen;
  struct destinatario d;

  //pulizia buffer
  memset(buffer, 0, DEFAULT_SIZE);

  ret = recv(tcp_sd, (void *)&msglen, sizeof(uint16_t), 0);

  if (ret < sizeof(uint16_t)){
    perror("recv()");
    exit(-1);
  }

  len = ntohs(msglen);
  // Attendo il messaggio
  ret = recv(tcp_sd, (void*)buffer, len, 0);

  if (ret < 0) {
    perror("recv()");
    exit(-1);
  }

  sscanf(buffer, "%s %d", d.addr, &d.port);

  return d;
}


void stampaLista(){
  int ret, len;
  int code = 0;
  uint16_t msglen, esito;

  printf("Client registrati\n");

  while(code != MSG_OK){

    ret = recv(tcp_sd, (void *)&esito, sizeof(uint16_t), 0);

    if (ret < sizeof(uint16_t)){
      perror("recv()");
      exit(-1);
    }

    code = ntohs(esito);

    if (code == MSG_PART){
      //pulizia buffer
      memset(buffer, 0, DEFAULT_SIZE);

      ret = recv(tcp_sd, (void *)&msglen, sizeof(uint16_t), 0);

      if (ret < sizeof(uint16_t)){
        perror("recv()");
        exit(-1);
      }

      len = ntohs(msglen);
      // Attendo il messaggio
      ret = recv(tcp_sd, (void*)buffer, len, 0);

      if (ret < 0) {
        perror("recv()");
        exit(-1);
      }

      printf("\t%s\n", buffer);

    }
  }
}


void riceviEsito(){
  uint16_t esito;
  int ret, code;
  struct destinatario d;


  // attesa risposta
  ret = recv(tcp_sd, (void *)&esito, sizeof(uint16_t), 0);
  if (ret < sizeof(uint16_t)){
    perror("recv()");
    exit(-1);
  }

  // controllo esito
  code = ntohs(esito);

  switch(code){
    case REG_OK:
      printf("Registrazione avvenuta con successo\n");
      strcpy(whoami, username);
      inviaPorta();
      riceviMessaggiOffline();
      break;

    case REG_ERR: printf("Syntax: !register username\n"); break;

    case SOCK_ERR: printf("Hai già una sessione aperta\n"); break;

    case DEREG_OK:
      printf("Deregistrazione avvenuta con successo\n");
      memset(whoami, 0, strlen(whoami));
      pthread_mutex_lock(&stato_client);
      s = CLIENT_NOT_REGISTERED;
      pthread_mutex_unlock(&stato_client);
      break;

    case DEREG_ERR: printf("Attualmente non sei registrato presso il server\n");  break;

    case CMD_NOT_FOUND: printf("Error: Command not recognized\n"); break;

    case DIS_OK: printf("Client disconnesso\n"); close(tcp_sd); exit(0);

    case LIST_OK: stampaLista(); break;

    case LIST_EMPTY: printf("Nessun client registrato al server\n"); break;

    case SEND_OK: printf("Messaggio offline inviato\n"); break;

    case SEND_WRN: printf("Syntax: !send username\n"); break;

    case SEND_ERR: printf("Impossibile connettersi a %s: utente inesistente\n", username); break;

    case SEND_ONLINE: d = riceviInfoDestinatario(); inviaMessaggioOnline(&d); break;

    case SEND_OFFLINE: inviaMessaggioOffline(); break;

    case BAD_REQ: printf("Error: bad request\n"); break;

    default: printf("Codice errore non presente\n");
  }
}



void inviaComando(){
  uint16_t msglen;
  int len, ret;

  len = strlen(buffer) + 1;
  msglen = htons(len);

  // invio lunghezza
  ret = send(tcp_sd, (void *)&msglen, sizeof(uint16_t), 0);
  if (ret < 0) {
    perror("send() ");
    exit(-1);
  }

  // invio comando
  ret = send(tcp_sd, (void *)buffer, len, 0);
  if (ret < 0) {
    perror("send() ");
    exit(-1);
  }
}
*/


/****************************************/
/*                MAIN                  */
/****************************************/

int main(int argc, char* argv[]) {
    if (argc < 3){
        cout << "./bin/client <ipserver> <serverport>" << endl;
        exit(0);
    }

    string line;
    char buffer[DEFAULT_SIZE];

    char* sv_addr = argv[1];
    int sv_port = atoi(argv[2]);

    Connection c = Connection(sv_addr, sv_port);
    c.recv(buffer, DEFAULT_SIZE);
    cout << buffer;

    while(1){
        // pulizia buffer
        line.clear();
        cout << ">";
        // waiting for command
        cin.clear();
        getline(cin, line);

        // Elimino carattere \n
        //buffer[strlen(buffer)] = '\0';
        //sscanf(buffer, "%4s %1024s", strcmd, filename);

        if (!line.empty()){
            parseCommand(line);
        }
    }
}
