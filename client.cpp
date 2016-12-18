//
// Created by zy-li14 on 16-12-16.
//
#include "network.h"
#include "utility.h"
#include <string>
#include <iostream>
#include <regex>
#include <cstdlib>
#include <stdlib.h>


int command_socket = 0, data_socket = 0;
constexpr int RSPNS_SIZE = 3;
constexpr int MAX_BUFFER_SIZE = 4096 * 4096;
char *buffer = new char[MAX_BUFFER_SIZE];
struct sockaddr_in server_data_addr, server_command_addr;

int retr(const std::string &filename)
//send retr command and wait for response.
;

int pwd();

int stor(const std::string &filename);

int list(const std::string &path);

int cwd(const std::string &path);

void help();

void quit();

int init()
//return error code
;

int pasv()
//send PASV command and wait for response
;

void rn(std::string basic_strin, std::string);

void execCommand(std::string string, std::string content);

void server();

int main(int argc, char **argv) {
    init();
    server_command_addr = construct_sockaddr("", 5021);
    int connect_response = connect(command_socket, (sockaddr *) &server_command_addr, sizeof(server_command_addr));
    if (connect_response != 0) {
        log("ERROR: Can't connect to Server", std::cerr);
        return 1;
    }
    pasv();
    std::string line;
    std::regex commandRegex("([a-zA-Z\\?]+)\\s*(\\S*|\\\".*\\\")");
    std::regex complexCommandRegex("([a-zA-Z\\?]+)\\s*(\\S*|\\\".*\\\")\\s*(\\S*|\\\".*\\\")");
    std::smatch match, cmatch;
    while (std::cout << ">>" && std::getline(std::cin, line)) {
        std::string command;
        if (std::regex_match(line, match, commandRegex)) {
            command = lower(match[1]);
        }
        else if (std::regex_match(line, cmatch, complexCommandRegex)) {
            command = lower(cmatch[1]);
        } else{
            std::cerr << "Invalid command. Type ? to show help." << std::endl;
            continue;
        }
        std::string content = match[2];
        if (command == "get") {
            retr(content);
        } else if (command == "put") {
            stor(content);
        } else if (command == "pwd") {
            pwd();
        } else if (command == "dir") {
            list(content);
        } else if (command == "cd") {
            cwd(content);
        } else if (command == "?") {
            help();
        } else if (command == "quit") {
            quit();
        }
        else if (command == "rename")
        {
            rn(cmatch[2], cmatch[3]);
        }
            else if (command == "rmd")
        {
            execCommand ("RMD", content);
        }
            else if (command == "mkdir")
        {
            execCommand ("MKD", content);
        }
        else if (command == "delete")
        {
            execCommand ("DELE", content);
        }
        else if (command == "server")
        {
            server();
        }
        else {
            std::cerr << "Invalid command. Type ? to show help" << std::endl;
            std::cout << std::endl;
        }
    }
}

int init()
//return error code
{
    return command_socket = socket(AF_INET, SOCK_STREAM, 0);
}

int check_command_socket() {
    if (command_socket < 0) {
        std::cerr << "ERROR: Send PORT command with command socket uninitialized. Command Socket is " +
                     std::to_string(command_socket) << std::endl;
        return -1;
    } else
        return 0;
}

int port(unsigned long addr, unsigned short port)
//send a port command and wait for response
{
    int ret = 0;
    sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(addr);
    my_addr.sin_port = htons(port);
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        std::cerr << "ERROR: Send PORT command with listen socket uninitialized. Data Socket is " +
                     std::to_string(listen_socket) << std::endl;
        return -1;
    }
    bind(data_socket, (struct sockaddr *) &my_addr, sizeof(my_addr));
    sprintf(buffer, "PORT %s\r\n", encode_address(addr, port).c_str());
    if ((ret = check_command_socket()) != 0)
        return ret;
    send(command_socket, buffer, strlen(buffer), 0);
    listen(listen_socket, 20);
    data_socket = accept(listen_socket, (sockaddr *) &server_data_addr, NULL);
    return ret;
}

int pasv()
//send PASV command and wait for response
{
    int ret = 0;
    if ((ret = check_command_socket()) != 0)
        return ret;
    sprintf(buffer, "PASV\r\n");
    send(command_socket, buffer, strlen(buffer), 0);
    while (recv(command_socket, buffer, MAX_BUFFER_SIZE, 0) <= 0);
    unsigned long addr;
    unsigned short port;
    decode_address(buffer, addr, port);
    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        std::cerr << "ERROR: Unable to get a new socket for data socket in pasv()." << std::strerror(errno)
                  << std::endl;
        return -1;
    }
    struct sockaddr_in *sockaddrIn = (&server_data_addr);
    sockaddrIn->sin_family = AF_INET;
    sockaddrIn->sin_addr.s_addr = addr;
    sockaddrIn->sin_port = port;
    log("PASV addr:" + ul2str(ntohl(addr)) + ", port:" + std::to_string(ntohs(port)), std::cout);
    if (connect(data_socket, (struct sockaddr *) (sockaddrIn), sizeof(server_data_addr)) != 0) {
        std::cerr << "ERROR: data socket connection failed." << std::strerror(errno) << std::endl;
        return 1;
    }
    return ret;
}

int sendCommand(const std::string &command, const std::string &content = std::string(""))
{
    int ret = 0;
    if ((ret = check_command_socket()) != 0)
        return ret;
    if (content.empty())
        sprintf(buffer, "%s\r\n", command.c_str());
    else
        sprintf(buffer, "%s %s\r\n", command.c_str(), content.c_str());
    send(command_socket, buffer, strlen(buffer), 0);
    return ret;
}

int waitForResponseCode()
{
    ssize_t recv_length;
    while ((recv_length = recv(command_socket, buffer, MAX_BUFFER_SIZE, 0)) == 0);
    log("Response:" + std::string(buffer, recv_length), std::cout);
    char rspns[RSPNS_SIZE];
    for (int i = 0; i < RSPNS_SIZE; ++i)
        rspns[i] = buffer[i];
    return std::atoi(rspns);
}
int wairForData()
{
    ssize_t recv_length;
    while ((recv_length = recv(data_socket, buffer, MAX_BUFFER_SIZE, 0)) == 0);
    if (recv_length < MAX_BUFFER_SIZE)
        *(buffer + recv_length) = '\0';
    return recv_length;
}

int retr(const std::string &filename)
//send retr command and wait for response.
{
    static std::regex filenameRegex(".*?([^/]+)");
    static std::smatch match;
    if (!std::regex_match(filename, match, filenameRegex)) {
        std::cerr << "ERROR: filename is not valid when sending RETR command" << std::endl;
        return -1;
    }
    std::string pureFileName = match[1];
    int response = 0;
    sendCommand("RETR", filename);
    response = waitForResponseCode();
    int totalSize = atoi(buffer + RSPNS_SIZE);
    switch (response) {
        case 226: //OK, then save it
        {
            ssize_t recv_length = 0;
            FILE *fout = fopen(pureFileName.c_str(), "w+");
            while (totalSize > 0 && (recv_length = recv(data_socket, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
                fwrite(buffer, sizeof(char), recv_length, fout);
                std::cout << std::string(buffer, recv_length) << std::endl;
                totalSize -= recv_length;
            }
            fclose(fout);
            break;
        }
        case 425: {
            std::cerr << "ERROR: Server reject RETR command because no TCP connection is established" << std::endl;
            break;
        }
        case 426: {
            std::cerr
                    << "ERROR: Server reject RETR command because the TCP connection was established but then broken by the client or by network failure"
                    << std::endl;
            break;
        }
        case 451:
        case 551: {
            std::cerr << "ERROR: Server reject RETR command because the server had trouble reading the file from disk"
                      << std::endl;
            break;
        }
        default:
            std::cerr << "ERROR: Server reject RETR command for unknown reason" << std::endl;
    }
    return 0;
}

int stor(const std::string &filename) {
    int ret = 0;
    if ((ret = check_command_socket()) != 0)
        return ret;
    FILE *fin = fopen(filename.c_str(), "r");
    if (fin == NULL) {
        std::cerr << "ERROR: Can't open file " + filename + " when sending STOR command" << std::endl;
        return -1;
    }
    sprintf(buffer, "STOR %s", filename.c_str());
    send(command_socket, buffer, strlen(buffer), 0);
    size_t fileLength;
    int totalSize = 0;
    while ((fileLength = fread(buffer, sizeof(char), MAX_BUFFER_SIZE, fin)) > 0) {
        totalSize += fileLength;
        send(data_socket, buffer, fileLength, 0);
        std::cout << "fuck" << std::endl;
    }
    sprintf(buffer, "%d\r\n", totalSize);
    log("Total Size:" + std::to_string(totalSize), std::cout);
    send(command_socket, buffer, strlen(buffer), 0);
    log("Sending done", std::cout);
    waitForResponseCode();
    return ret;
}

int sendAndEchoResponse(const std::string &command, const std::string &content = std::string("")) {
    int ret = 0;
    if ((ret = sendCommand(command, content)) != 0)
        return ret;
    waitForResponseCode();
    wairForData();
    std::cout << buffer << std::endl;
    return ret;
}

int pwd() {
    return sendAndEchoResponse("PWD");
}

int list(const std::string &path) {
    return sendAndEchoResponse("LIST", path);
}

int nlst(const std::string &path) {
    return sendAndEchoResponse("NLST", path);
}

int cwd(const std::string &path) {
    int response = 0;
    int ret = 0;
    if ((ret = sendCommand("CWD", path)) != 0)
        return ret;
    response = waitForResponseCode();
    switch (response) {
        case 250: {
            std::cout << "Working directory changed to " + path << std::endl;
            break;
        }
        default:
            std::cerr << "ERROR: Server reject CWD because of unknown reason" << std::endl;
    }
    return 0;
}

void help() {
    std::cout << "Simple FTP Client." << std::endl;
    std::cout << "\tget path-to-file" << std::endl;
    std::cout << "\tput path-to-file" << std::endl;
    std::cout << "\t?" << std::endl;
    std::cout << "\tpwd" << std::endl;
    std::cout << "\tdir [path]" << std::endl;
    std::cout << "\tcd path" << std::endl;
    std::cout << "\tquit" << std::endl;
}

void quit() {
    sprintf(buffer, "QUIT");
    send(command_socket, buffer, strlen(buffer), 0);
    exit(0);
}

void server() {
    sendAndEchoResponse("SYST", "");
    sendAndEchoResponse("HELP", "");
}

void execCommand(std::string string, std::string content) {
    sendCommand(string, content);
    int response = waitForResponseCode();
    switch (response)
    {
        case 250:
            break;
        default:
            std::cerr << "ERROR: unknown error from server" << std::endl;
            std::cout << std::endl;
    }
}

void rn(std::string from, std::string to) {
    sendCommand("RNFR", from);
    int response = waitForResponseCode();
    switch (response)
    {
        case 250:
            break;
        default:
            std::cerr << "ERROR: unknown error from server" << std::endl;
            std::cout << std::endl;
            return;
    }
    sendCommand("RNTO", to);
    response = waitForResponseCode();
    switch (response)
    {
        case 250:
            break;
        default:
            std::cerr << "ERROR: unknown error from server" << std::endl;
            std::cout << std::endl;
            return;
    }
}
