#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include<pthread.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;
#define BUFFER_SIZE 10000

struct threadArg {
    string dirpath;
    int sock;
};

class HTTPReq {
private:
    string status;
    string method;
    string url;
    string contentLength;
    void setURL(string url);
    void setMethod(string method);
public:
    void setStatus(string status);
    void setContentLength (int lenght);
    string encode();
    void parse(std::string bytecode);
    string getURL();
    HTTPReq();
    HTTPReq(string url, string method);
    HTTPReq(string status, string method, int length);
};

HTTPReq::HTTPReq(){}

HTTPReq::HTTPReq(string url, string method) {
    setURL(url);
    setMethod(method);
}

HTTPReq::HTTPReq(string status, string method, int length) {
    setStatus(status);
    setMethod(method);
    setContentLength(length);
}

void HTTPReq::setURL(string url) {
    this->url = url;
}

void HTTPReq::setMethod(string method) {
    this->method = method;
}

void HTTPReq::setStatus(string status) {
    this->status = status;
}

void HTTPReq::setContentLength (int lenght) {
    this->contentLength = to_string(lenght);
}

std::string HTTPReq::getURL() {
    return this->url;
}

string HTTPReq::encode() {
    string buffer = "";

    if (this->method.compare("req") == 0){
        buffer += "GET " + this->url + " HTTP/1.0";
        buffer += "\r\n";
        buffer += "\r\n";
    }else if (this->method.compare("res") == 0){
        buffer += "HTTP/1.1 " + this->status + " ";
        if(this->status.compare("200") == 0){
            buffer += "OK";
        }else if(this->status.compare("404") == 0){
            buffer += "Not Found";
        }else if(this->status.compare("400") == 0){
            buffer += "Bad Request";
        }
        buffer += "\r\n";
        buffer += "Content-Length: ";
        buffer += this->contentLength;
        buffer += "\r\n";
        buffer += "\r\n";
    }
    return buffer;
}

void HTTPReq::parse(string bytecode) {
    string lixo, url;
    istringstream iss(bytecode);
    iss >> lixo;
    iss >> url;
    //Verificar se file existe, se nao existir retornar 404, se existir encontrar tamanho e retorna 200
    //Teste de exemplo
    setURL(url);
    setMethod("res");
    setStatus("200");
    setContentLength(-1);
}

void addrDNS(char *host, char *outStr){
    struct addrinfo hints{};
    struct addrinfo* res;

    // hints - modo de configurar o socket para o tipo  de transporte
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    // funcao de obtencao do endereco via DNS - getaddrinfo
    // funcao preenche o buffer "res" e obtem o codigo de resposta "status"
    int status;
    if ((status = getaddrinfo(host, "80", &hints, &res)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "IP addresses for " << host << ": " << std::endl;

    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
        // a estrutura de dados eh generica e portanto precisa de type cast
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;

        // e depois eh preciso realizar a conversao do endereco IP para string
        inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    }
    strncpy(outStr, ipstr, INET_ADDRSTRLEN);
    freeaddrinfo(res); // libera a memoria alocada dinamicamente para "res"
}

void *connection_handler(void *arg) {
  // faz leitura e escrita dos dados da conexao
  // utiliza um buffer de 20 bytes (char)

  threadArg *argument = (threadArg *)arg;
  // std::cout << "Thread argument is here: " << argument->dirpath << std::endl;
  // std::cout << "Ok, that ended" << std::endl;
  int sock = argument->sock;
  string dirpath = argument->dirpath;
  bool isEnd = false;
  char buf[100] = {0};
  std::stringstream ss;
  // std::cout << "Initialized variables" << std::endl;

  while (!isEnd) {
      // zera a memoria do buffer
      memset(buf, '\0', sizeof(buf));

      // recebe ate 100 bytes do cliente remoto
      if (recv(sock, buf, 100, 0) == -1) {
        perror("recv");
        int ret_value = 5;
        return (void*) ret_value;
      }

      // Imprime o valor recebido no servidor antes de reenviar
      // para o cliente de volta
      ss << buf;
      std::cout << buf;

      //construção do objeto HTTP
      string message(buf);
      string bytecode;
      HTTPReq response;
      response.parse(message);
      const std::string filename = response.getURL();
      const std::string filepath = dirpath + filename;
      struct stat statbuffer;
      int file_size;

      //Cheque para filename "/" para retorna index.html(to do)

      //Cheque de existencia do arquivo pedido e set status
      if(stat (filepath.c_str(), &statbuffer) == 0){
          off_t size = statbuffer.st_size;
          file_size = int(size);
          response.setContentLength(int(size));
          response.setStatus("200");

          bytecode = response.encode();

          std::ifstream is (filepath, std::ifstream::binary);
          //abrir file e definir cont
          /*FILE *file;
          file = fopen(filepath.c_str(), "r");
          if (file == NULL) {//response 404

              continue;
          }*/
          int cont = 0;
          char msg[BUFFER_SIZE];
          memset(msg, '\0', BUFFER_SIZE);
          std::cout << bytecode;
          std::cout << "bytes in header: " << bytecode.size() << std::endl;
          //loop para envio do file
          while(cont < file_size){
              int readNum = BUFFER_SIZE - bytecode.size();
              //fread(msg, sizeof(char), readNum, file);
              is.read (msg,readNum);
              std::cout << "bytes lidos: " << readNum << std::endl;
              string data = msg;
              bytecode += data;
              std::cout << "bytes in bytecode: " << bytecode.size() << std::endl;
              int byte_size;


              if ((byte_size = send(sock, bytecode.c_str(), BUFFER_SIZE, 0)) == -1) {
                  perror("send");
                  int ret_value = 6;
                  return (void*) ret_value;
              }
              std::cout << "bytes enviados: " << byte_size << std::endl;
              bytecode = "\0";
              memset(msg, '\0',BUFFER_SIZE);

              cont += byte_size;
              std::cout << "cont: " << cont << std::endl;
          }

          // envia de volta o buffer recebido como um echo


          // o conteudo do buffer convertido para string pode
          // ser comparado com palavras-chave
          if (ss.str() == "close\n")
            break;

          // zera a string para receber a proxima
          ss.str("");
      } else {
          response.setStatus("404");
          //do something
          int byte_size;
          bytecode = response.encode();
          if ((byte_size = send(sock, bytecode.c_str(), 100, 0)) == -1) {
              perror("send");
              int ret_value = 6;
              return (void*) ret_value;
          }

          continue;
      }
  }

  // fecha o socket
  close(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "usage: web-server [HOST] [PORT] [TMP]" << std::endl;
        return 1;
    }
    char address[INET_ADDRSTRLEN];
    int port = atoi(argv[2]);
    string dir = argv[3];
    string dirpath = "." + dir;

    addrDNS(argv[1], address);
    cout << address << " " << argv[2] << " " << argv[3] << endl;

    // cria um socket para IPv4 e usando protocolo de transporte TCP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Opções de configuração do SOCKETs
    // No sistema Unix um socket local TCP fica preso e indisponível
    // por algum tempo após close, a não ser que configurado SO_REUSEADDR
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
    }

    // struct sockaddr_in {
    //  short            sin_family;   // e.g. AF_INET, AF_INET6
    //  unsigned short   sin_port;     // e.g. htons(3490)
    //  struct in_addr   sin_addr;     // see struct in_addr, below
    //  char             sin_zero[8];  // zero this if you want to
    // };

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);     // porta tem 16 bits, logo short, network byte order
    addr.sin_addr.s_addr = inet_addr(address);
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    // realizar o bind (registrar a porta para uso com o SO) para o socket
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
    }

    // colocar o socket em modo de escuta, ouvindo a porta
    if (listen(sockfd, 1) == -1) {
    perror("listen");
    return 3;
    }

    // aceitar a conexao TCP
    // verificar que sockfd e clientSockfd sao sockets diferentes
    // sockfd eh a "socket de boas vindas"
    // clientSockfd eh a "socket diretamente com o cliente"
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);

    pthread_t thread_id;



    while(int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize)) {
      // std::cout << "Connection accepted" << std::endl;
      threadArg *arg = new threadArg();
      arg->dirpath = dirpath;
      arg->sock = clientSockfd;
      // std::cout << "Thread argument is here: " << arg->dirpath << std::endl;
      // std::cout << "Reaching here" << std::endl;
      if(pthread_create(&thread_id, NULL, connection_handler, (void*) arg) < 0) {
        perror("could not create thread");
        return 1;
      }

      puts("Handler assigned");
    }

    // if (clientSockfd == -1) {
    //   perror("accept");
    //   return 4;
    // }
    //
    // // usa um vetor de caracteres para preencher o endereço IP do cliente
    // char ipstr[INET_ADDRSTRLEN] = {'\0'};
    // inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    // std::cout << "Accept a connection from: " << ipstr << ":" <<
    // ntohs(clientAddr.sin_port) << std::endl;
    //
    // // faz leitura e escrita dos dados da conexao
    // // utiliza um buffer de 20 bytes (char)
    // bool isEnd = false;
    // char buf[100] = {0};
    // std::stringstream ss;
    //
    // while (!isEnd) {
    //     // zera a memoria do buffer
    //     memset(buf, '\0', sizeof(buf));
    //
    //     // recebe ate 100 bytes do cliente remoto
    //     if (recv(clientSockfd, buf, 100, 0) == -1) {
    //       perror("recv");
    //       return 5;
    //     }
    //
    //     // Imprime o valor recebido no servidor antes de reenviar
    //     // para o cliente de volta
    //     ss << buf;
    //     std::cout << buf;
    //
    //     //construção do objeto HTTP
    //     string message(buf);
    //     string bytecode;
    //     HTTPReq response;
    //     response.parse(message);
    //     const std::string filename = response.getURL();
    //     const std::string filepath = dirpath + filename;
    //     struct stat statbuffer;
    //     int file_size;
    //
    //     //Cheque para filename "/" para retorna index.html(to do)
    //
    //     //Cheque de existencia do arquivo pedido e set status
    //     if(stat (filepath.c_str(), &statbuffer) == 0){
    //         off_t size = statbuffer.st_size;
    //         file_size = int(size);
    //         response.setContentLength(int(size));
    //         response.setStatus("200");
    //
    //         bytecode = response.encode();
    //
    //         std::ifstream is (filepath, std::ifstream::binary);
    //         //abrir file e definir cont
    //         /*FILE *file;
    //         file = fopen(filepath.c_str(), "r");
    //         if (file == NULL) {//response 404
    //
    //             continue;
    //         }*/
    //         int cont = 0;
    //         char msg[BUFFER_SIZE];
    //         memset(msg, '\0', BUFFER_SIZE);
    //         std::cout << bytecode;
    //         std::cout << "bytes in header: " << bytecode.size() << std::endl;
    //         //loop para envio do file
    //         while(cont < file_size){
    //             int readNum = BUFFER_SIZE - bytecode.size();
    //             //fread(msg, sizeof(char), readNum, file);
    //             is.read (msg,readNum);
    //             std::cout << "bytes lidos: " << readNum << std::endl;
    //             string data = msg;
    //             bytecode += data;
    //             std::cout << "bytes in bytecode: " << bytecode.size() << std::endl;
    //             int byte_size;
    //
    //
    //             if ((byte_size = send(clientSockfd, bytecode.c_str(), BUFFER_SIZE, 0)) == -1) {
    //                 perror("send");
    //                 return 6;
    //             }
    //             std::cout << "bytes enviados: " << byte_size << std::endl;
    //             bytecode = "\0";
    //             memset(msg, '\0',BUFFER_SIZE);
    //
    //             cont += byte_size;
    //             std::cout << "cont: " << cont << std::endl;
    //         }
    //
    //         // envia de volta o buffer recebido como um echo
    //
    //
    //         // o conteudo do buffer convertido para string pode
    //         // ser comparado com palavras-chave
    //         if (ss.str() == "close\n")
    //           break;
    //
    //         // zera a string para receber a proxima
    //         ss.str("");
    //     } else {
    //         response.setStatus("404");
    //         //do something
    //         int byte_size;
    //         bytecode = response.encode();
    //         if ((byte_size = send(clientSockfd, bytecode.c_str(), 100, 0)) == -1) {
    //             perror("send");
    //             return 6;
    //         }
    //
    //         continue;
    //     }
    // }
    //
    // // fecha o socket
    // close(clientSockfd);

    return 0;
}
