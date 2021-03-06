#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <stdio.h>
#include <sys/wait.h>
#include <map>

using namespace std;

#define BACKLOG 16
#define BUFLEN 4096
#define DEFAULT_PORT 5000

std::map<int, int> ARR{};

char request[BUFLEN];

class Server {
private:
    int Server_fd;
public:
    static int port;
    Server(int portnum);
    int new_connection(int, int);
    void req_handle(int);
    ~Server() { close(Server_fd); }
    int get_sock() { return Server_fd; }
//    int get_port() const { return port; }
};

int Server::port = DEFAULT_PORT;

Server::Server(int portnum) {
/*
to create server we need:
    1. open socket - take FD, fd = socket(family, type, protocol)
    2. fill structure: sockaddr_in, which used by bind
    3. bind socket - i.e. give the server an adress, bind(socket FD, struct sockaddr_in, sizeof struct)
    4. put into listening mode
*/
    port = portnum;
    if ((Server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Cant create socket" << endl;
        exit(1);
    }
    struct sockaddr_in ServAddr;
    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_port = htons(port);
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(Server_fd, (struct sockaddr*)&ServAddr, sizeof(ServAddr)) < 0) {
        cerr << "Cant bind socket" << endl;
        close(Server_fd);
        exit(1);
    }
    if (listen(Server_fd, BACKLOG) < 0) {
        cerr << "Cant listen" << endl;
        exit(1);
    }
}

enum type { TXT, JPG, PNG, HTML};

char* getfilename(char* path) {
    int i = 8;
    while(path[i] && (path[i] != '?')) i++;
    int len = i-8;
    char* filename = new char[len];
    copy(&path[8], &path[i], &filename[0]);
    filename[i-8] = 0;
    cout << filename << endl;
    return filename;
}

int get_type(const char*req) {
    int i = 0;
    while (req[i] != '.' && req[i] != 0)
        i++;
    char type[5];
    int start = ++i;
    while (req[i] != 0 && (i-start != 4)) {
        type[i-start] = req[i];
        i++;
    }
    type[i-start] = 0;
    if(!strcmp(type,"jpg"))
        return JPG;
    if(!strcmp(type,"png"))
        return PNG;
    if(!strcmp(type,"html"))
        return HTML;
    return TXT;
}

void Send(const char* file, string header, int sock_fd) {
    int fd = open(file, O_RDONLY);
    string str = header;
    str += "\nAllow: GET, HEAD";
    str += "\nServer: MyServer/1.1";
//CONNECTION
    str += "\nConnection: keep-alive";
//CONTENT LENGTH
    str += "\nContent-length: ";
    char c;
    int len = 0;
    while(read(fd, &c, 1)) len++;
    lseek(fd, 0, 0);
    str += to_string(len);
//CONTENT TYPE
    str += "\nContent-type: ";
    int type = get_type(file);
    switch(type) {
        case 1: str += "image/jpeg";
            break;
        case 2: str += "image/apng";
            break;
        case 3: str += "text/html";
            break;
        default:
            if (fd < 0) str += "text/html";
            else str += "text/plain";
            break;
    }
//DATE
    str += "\nDate: ";
    time_t now = time(0);
    str += ctime(&now);
//Last-modified
    str += "Last-modified :";
    struct stat buff;
    fstat(fd, &buff);
    str += ctime(&buff.st_mtime);
//HOST
    str += "Host: 127.0.0.1:";
    str += to_string(Server::port);
    str += "\n\n";
    cout << str << endl;

    int n = str.length();
    char* buf = (char*)malloc(sizeof(char) * (n+1));
    strcpy(buf, str.c_str());
    len = strlen(buf);
    send(sock_fd, buf, len, 0);
    free(buf);

    if(!strncmp(request, "GET", 3)) {
        char buf2[BUFLEN];
        while((len = read(fd, buf2, BUFLEN)) > 0)
            send(sock_fd, buf2, len, 0);
    }
    close(fd);
}

int Server::new_connection(int Client_fd, int i) {
    int len = recv(Client_fd, request, BUFLEN, 0);
    if (len < 0) {
        cerr << "Server error" << endl;
        shutdown(Client_fd, SHUT_RDWR);
        close(Client_fd);
        close(Server_fd);
        exit(1);
    }
    cout << request << endl;
    return len;
}

void Server::req_handle(int Client_fd) {
    if(strncmp(request, "GET", 3) && strncmp(request, "HEAD", 4)) {
        Send("src/501.html", "HTTP/1.1 501 NotImplemented", Client_fd);
        cerr << "Error: BadRequest" << endl;
    } else {                               //GET
        int i = 5;
        char c = request[i];
        while(c != ' ') c = request[++i];  //skip to spaces
        char path[i-5];                    //possible filename
        if (i == 5) {                      //URI doesnt contain filename
            path[0] = '/';
            path[1] = '\0';
        } else {                           //copy filename to path
            copy(&request[5], &request[i], &path[0]);
            path[i-5] = 0;
        }
cout << "Length: " << strlen(path) << " Filename: " << path << endl;

        if(!strncmp(path, "cgi-bin", 7)) {
cout << "CGI" << endl;

            int pid;
            if((pid = fork()) < 0) {
                cerr << "Can't make process" << endl;
                exit(1);
            } else if (pid > 0) {
                ARR.insert({pid, Client_fd});
            } else if (pid == 0) {
                chdir("./cgi-bin");
                //UNIQ LOG FILE
                string logfile = to_string(getpid()) + ".txt";
                int fd = open(logfile.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
                //get exec filename
                char* exec_filename = getfilename(path);
                cout << "FILEMAME: " << exec_filename << endl;
                //CREATE ENVIROMENT
                char* argv[] = {exec_filename, NULL};

                char params[strlen(path)-12];
                copy(&path[12], &path[strlen(path)], &params[0]);
                params[strlen(path)-12] = 0;

                char**env = new char*[7];
                env[0] = new char[(int)strlen("SERVER_ADDR=127.0.0.1") + 1];
                strcpy(env[0], "SERVER_ADDR=127.0.0.1");
                env[1] = new char[(int)strlen("SERVER_PORT=XXXX") + 1];
                strcpy(env[1], "SERVER_PORT=");
                strcat(env[1], to_string(port).c_str());
                env[2] = new char[(int)strlen("SERVER_PROTOCOL=HTTP/1.1") + 1];
                strcpy(env[2], "SERVER_PROTOCOL=HTTP/1.1");
                env[3] = new char[(int)strlen("CONTENT_TYPE=text/plain") + 1];
                strcpy(env[3], "CONTENT_TYPE=text/plain");
                env[4] = new char[(int)strlen("SCRIPT_NAME=cgi-bin/") + (int)strlen(exec_filename) + 1];
                strcpy(env[4], "SCRIPT_NAME=cgi-bin/");
                strcat(env[4], exec_filename);
                env[5] = new char[(int)strlen("QUERY_STRING=") + (int)strlen(params) + 1];
                strcpy(env[5], "QUERY_STRING=");
                strcat(env[5], params);
                env[6] = NULL;

                dup2(fd, 1);
                close(fd);
                execve(exec_filename, argv, env);
                delete[]env;
                exit(1);
            }
        } else {
            int Filefd = open(path, O_RDONLY);
            struct stat buff;
            fstat(Filefd, &buff);
            int tmp = Filefd;
            close(Filefd);
            bool IS_FILE = buff.st_mode & S_IFREG;
            if ((i != 5) && (!IS_FILE || (tmp < 0))) {          //cant open file
                Send("src/404.html", "HTTP/1.1 404 NotFound", Client_fd);
                cerr << "Error 404" << endl;
            } else {                                                        //if open or if homepage
                if (i == 5) {
                    Send("src/index.html", "HTTP/1.1 200 MyServer", Client_fd);
                } else {
                    Send(path, "HTTP/1.1 200 MyServer", Client_fd);
                }
            }
        }
    }
    //shutdown(Client_fd, SHUT_RDWR);
    //close(Client_fd);
}

void MY_SIGPIPE (int s) {
//    signal(SIGPIPE, MY_SIGPIPE);
    cout << "SIGPIPE RECIEVED\n";
    exit(1);
}

void MY_SIGCHLD (int s) {
    signal(SIGCHLD, MY_SIGCHLD);
    int status=0, pid;
    if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        //HANDLE CGI ENDING

        auto search = ARR.find(pid);
        int sockfd = search->second;

        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == 0) {
                string logfile = "cgi-bin/" + to_string(pid) + ".txt";
                Send(logfile.c_str(), "HTTP/1.1 200 MyServer", sockfd);
            } else {
                cerr << "CGI has finihed with status " << WEXITSTATUS(status) << endl;
                Send("src/cgi.html", "HTTP/1.1 500 MyServer", sockfd);
            }
        } else if (WIFSIGNALED(status)) {
            cerr << "CGI has finished with signal " << WIFSIGNALED(status) << endl;
            Send("src/cgi.html", "HTTP/1.1 500 MyServer", sockfd);
        }

        ARR.extract(pid);
    }
}

bool is_waiting(int sockfd) {
    for(auto it = ARR.begin(); it != ARR.end(); ++it)
        if (it->second == sockfd)
            return true;
    return false;
}

int main(int argc, char**argv) {
    int portnum = DEFAULT_PORT;
    if (argc == 2)
        portnum = atoi(argv[1]);
    Server server(portnum);

    signal(SIGCHLD, MY_SIGCHLD);
    signal(SIGPIPE, MY_SIGPIPE);

    fd_set readset, allset;
    timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    int nready, maxfd, maxi, sockfd, clients[FD_SETSIZE];
    maxfd = server.get_sock();
    maxi =   -1;
    for(int i = 0; i < FD_SETSIZE; i++)
        clients[i] = -1;

    FD_ZERO(&allset);
    FD_SET(server.get_sock(), &allset);
    for(;;) {
        readset = allset;
        fcntl(server.get_sock(), F_SETFL, O_NONBLOCK);
        nready = select(maxfd+1, &readset, NULL, NULL, NULL);
        if (FD_ISSET(server.get_sock(), &readset)) {
            // connect with new client
            struct sockaddr_in ClientAddr;
            size_t ClAddrLen = sizeof(ClientAddr);
            int Client_fd = accept(server.get_sock(), (struct sockaddr*)&ClientAddr, (socklen_t*)&ClAddrLen);
            if (Client_fd < 0) {
                cerr << "Client error" << endl;
//                exit(1);
            }
            for(int i = 0; i < FD_SETSIZE; i++) {
                if (clients[i] < 0) {
                    clients[i] = Client_fd;
                    if(i > maxi)
                        maxi = i;
                    break;
                }
            }
            FD_SET(Client_fd, &allset);
            if(Client_fd > maxfd)
                maxfd = Client_fd;
            if(--nready <= 0)
                continue;   //no more ready descriptors
        }
        for(int i = 0; i <= maxi; i++) { //check all clients for data
            if((sockfd = clients[i]) < 0)
                continue;
            if(FD_ISSET(sockfd, &readset)) {
                if (is_waiting(sockfd)) {}    //if sockfd is waiting for cgi
                else if (server.new_connection(sockfd, i) == 0) {
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    clients[i] = -1;
                } else
                    server.req_handle(sockfd);

                if(--nready <= 0)
                    break;
            }
        }
    }
    return 0;
}
