#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;


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
    int getContentLenght();
    std::string getStatus();
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

std::string HTTPReq::getStatus() {
    return this->status;
}

int HTTPReq::getContentLenght() {
    return atoi(this->contentLength.c_str());
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
    string lixo, status;
    int content;
    istringstream iss(bytecode);
    iss >> lixo;
    iss >> status;
    iss >> lixo;
    iss >> lixo;
    iss >> content;

    setMethod("res");
    setStatus(status);
    setContentLength(content);
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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "usage: web-server [URL]" << std::endl;
        return 1;
    }

    string url = argv[1];
    string address;
    string port;
    string object;
    bool beginHost = false, endHost = false, beginPort = false, endPort = false, beginObject = false;
    int count = 0;
    object += '/';
    for(int i = 0; i < url.length(); i++){
        if(url[i] == '/')
            count++;
        else if(count == 2 && url[i] == ':'){
            endHost = true;
            beginPort = true;
        } else{
            if(beginHost && !endHost){
                address += url[i];
            }
            else if(beginPort && !endPort) {
                port += url[i];
            }
            else if(beginObject) {
                object += url[i];
            }
        }
        if(count == 2 && !beginHost) {
            beginHost = true;
        }
        if(count == 3 && !beginObject){
            beginObject = true;
            endPort = true;
        }

    }

    stringstream stream(port);

    int numPort = 0;
    stream >> numPort;

    char addr[INET_ADDRSTRLEN];
    char host[INET_ADDRSTRLEN];
    strcpy(host, address.c_str());
    addrDNS(host, addr);

    cout << addr << " " << port << " " << object << endl;
    // cria o socket TCP IP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // atividade de preenchimento da estrutura de endereço IP
    // e porta de conexão, precisa ser zerado o restante da estrutura
    // struct sockaddr_in {
    //  short            sin_family;   // e.g. AF_INET, AF_INET6
    //  unsigned short   sin_port;     // e.g. htons(3490)
    //  struct in_addr   sin_addr;     // see struct in_addr, below
    //  char             sin_zero[8];  // zero this if you want to
    // };
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(numPort);     // short, network byte order
    serverAddr.sin_addr.s_addr = inet_addr(addr);
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

    // conecta com o servidor atraves do socket
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        return 2;
    }

    // a partir do SO, eh possivel obter o endereço IP usando
    // pelo cliente (nos mesmos) usando a funcao getsockname
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
        perror("getsockname");
        return 3;
    }

    // em caso de multiplos endereços, isso permite o melhor controle
    // a rotina abaixo, imprime o valor do endereço IP do cliente
    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    std::cout << "Set up a connection from: " << ipstr << ":" <<
              ntohs(clientAddr.sin_port) << std::endl;

    // trecho de código para leitura e envio de dados nessa conexao
    // buffer eh o buffer de dados a ser recebido no socket com 20 bytes
    // input eh para a leitura do teclado
    // ss eh para receber o valor de volta
    bool isEnd = false;
    char buf[1500] = {0};
    std::string input;
    std::stringstream ss;

    //Criar HTTP request
    HTTPReq request(object, "req");
    string bytecode;
    bytecode = request.encode();

    while (!isEnd) {
        // leitura do teclado
        std::cout << "send(y/n): ";
        std::cin >> input;
        if(input.compare("y") == 0){
            // converte a string lida em vetor de bytes
            // com o tamanho do vetor de caracteres
            if (send(sockfd, bytecode.c_str(), bytecode.size(), 0) == -1) {
                perror("send");
                return 4;
            }
            int cont = 0, clength, bytes_recebidos;
            string msg;
            HTTPReq resp;
            //Receber mensagem que contem HTTP header
            if (recv(sockfd, buf, 1500, 0) == -1) {
                perror("recv");
                return 5;
            }

            msg = buf;
            resp.parse(msg);
            std::cout << "tamanho do arquivo total: " << resp.getContentLenght() << endl;
            std::cout << "tamanho da mensagem com header: " << msg.size() << endl;
            if(resp.getStatus().compare("200") == 0){
                clength = resp.getContentLenght();
                std::string::iterator aux;
                //começar a gravar file
                for (std::string::iterator it=msg.begin(); it!=msg.end(); ++it){
                    if(*it == '\r' && *(it + 1) == '\n' && *(it + 2) == '\r' && *(it + 3) == '\n'){
                        //it eh o iterator ond começa a linha em branco
                        //it + 4 eh ond começa o file
                        aux = (it + 4);
                        break;
                    }
                }
                auto str = std::string(aux, msg.end());
                std::cout << "str: " << str << std::endl;
                cont += str.size();
                string filename = "." + object;
                std::ofstream ofs(filename, std::ofstream::out);
                ofs << str;

                while(cont != clength){
                    // zera o buffer
                    memset(buf, '\0', sizeof(buf));
                    msg = "";

                    // recebe no buffer uma certa quantidade de bytes ate 20
                    if ((bytes_recebidos = recv(sockfd, buf, 1500, 0) == -1)) {
                        perror("recv");
                        return 5;
                    }
                    std::cout << "Bytes recebidos: " << bytes_recebidos << std::endl;
                    msg = buf;
                    ofs << msg;
                    std::cout << "tamanho da mensagem recebida: " << msg.size() << std::endl;
                    cont += msg.size();
                    std::cout << "bytes restantes: " << clength - cont << std::endl;

                }
                std::cout << "saiu do loop" << endl;
                ofs.close();
            } else if(resp.getStatus().compare("404") == 0) {
              std::cout << buf << std::endl;
              std::cout << object.substr(1,object.length()) << " was not found" << std::endl;
            }
            msg = "";
        }

        // se a string tiver o valor close, sair do loop de eco
        if (ss.str() == "close\n")
            break;

        // zera a string ss
        ss.str("");
    }

    // fecha o socket
    close(sockfd);

    return 0;
}
