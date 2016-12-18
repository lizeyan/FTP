//
// Created by zy-li14 on 16-12-16.
//
#include "network.h"
#include "utility.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <cstdio>
#include <thread>
#include <chrono>
#include <sstream>
#include <cstring>

int command_socket = 0, data_socket = 0;
constexpr int MAX_BUFFER_SIZE = 4096 * 4096;
char *buffer = new char[MAX_BUFFER_SIZE];
struct sockaddr_in my_command_addr, my_data_addr;
std::string localAddress;
void handle_client(int command_client, sockaddr_in client_addr);

int main(int argc, char **argv) {
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (argc == 3) {
        unsigned short listening_port = 5021;
        std::stringstream portStream(argv[2]);
        portStream >> listening_port;
        my_command_addr = construct_sockaddr("", listening_port);
        localAddress = argv[1];
    } else if (argc == 2) {
        my_command_addr = construct_sockaddr("", 5021);
        localAddress = argv[1];
    } else {
        my_command_addr = construct_sockaddr("", 5021);
    }
    if (bind(listen_socket, (sockaddr *) &my_command_addr, sizeof(my_command_addr)) != 0) {
        std::cout << ul2str(ntohl(my_command_addr.sin_addr.s_addr)) << std::endl;
        log("Can't bind listen socket on specified address and port", std::cerr);
    }
    if (listen(listen_socket, 20) != 0) {
        log("Can't listen via listen socket", std::cerr);
        return -1;
    }
    int command_socket = 0;
    sockaddr_in client_addr;
    socklen_t client_addr_len;
    int count = 0;
    while (true) {
        if (count > 20) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        } else if (count < 0)
            break;
        while ((command_socket = accept(listen_socket, (sockaddr *) &client_addr, &client_addr_len)) <= 0);
        std::thread thread([command_socket, client_addr, &count]() {
            log("Client " + ul2str(ntohl(client_addr.sin_addr.s_addr)) + " is connected.", std::cout);
            ++count;
            handle_client(command_socket, client_addr);
            --count;
        });
        thread.detach();
    }

}

void handle_client(int command_socket, sockaddr_in client_addr) {
    std::string workingDirectory = "/";
    std::string client_ip_address = ul2str(ntohl(client_addr.sin_addr.s_addr));
    int data_socket = 0;
    int command_length;
    sockaddr_in remote_addr;
    std::string specifiedFile;
    while (true) {
        log("working directory:" + workingDirectory, std::cout);
        command_length = recv(command_socket, buffer, MAX_BUFFER_SIZE, 0);
        if (command_length == 0) {
            close(data_socket);
            close(command_socket);
            return;
        }
        std::string command, content;
        std::stringstream commandStream(std::string(buffer, command_length));
        commandStream >> command >> content;
        log("From " + client_ip_address + " command:" + command + ", content:" + content, std::cout);
        command = lower(command);
        if (command == "quit") {
            close(data_socket);
            close(command_socket);
            break;
        } else if (command == "port") {
            unsigned long addr;
            unsigned short port;
            decode_address(content, addr, port);
            remote_addr = construct_sockaddr(addr, port);
            data_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (connect(data_socket, (sockaddr *) &remote_addr, sizeof(remote_addr)) == 0) {
                sprintf(buffer, "250\r\n");
            } else {
                sprintf(buffer, "500\r\n");
            }
            send(command_socket, buffer, strlen(buffer), 0);
        } else if (command == "pasv") {
            int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in listen_addr = construct_sockaddr(str2ul(localAddress, '.'), 0);
            if (bind(listen_socket, (sockaddr *) &listen_addr, sizeof(struct sockaddr)) != 0) {
                std::cerr << std::strerror(errno) << std::endl;
            }
            socklen_t my_data_addr_len = sizeof(my_data_addr);
            if (getsockname(listen_socket, (sockaddr *) &my_data_addr, &my_data_addr_len) != 0)
            {
                std::cerr << std::strerror(errno) << std::endl;
            }
            if (listen(listen_socket, 20) != 0) {
                std::cerr << std::strerror(errno) << std::endl;
            }
            sprintf(buffer, "%s\r\n", encode_address(my_data_addr.sin_addr.s_addr, my_data_addr.sin_port).c_str());
            send(command_socket, buffer, strlen(buffer), 0);
            log("To " + client_ip_address + ", " + std::string(buffer), std::cout);
            socklen_t data_addr_len;
            data_socket = accept(listen_socket, (sockaddr *) &remote_addr, &data_addr_len);
            if (data_socket <= 0) {
                std::cerr << std::strerror(errno) << std::endl;
            }
        } else if (command == "retr") {
            std::string filename = exec("cd " + workingDirectory + " \n realpath " + content);
            log("filename:" + filename, std::cout);
            FILE *file = fopen(filename.c_str(), "r");
            if (file != NULL) {
                fseek(file, 0, SEEK_END); // seek to end of file
                int totalSize = ftell(file); // get current file pointer
                fseek(file, 0, SEEK_SET);
                sprintf(buffer, "226%d\r\n", totalSize);
                send(command_socket, buffer, strlen(buffer), 0);
                log("Total Size:" + std::to_string(totalSize), std::cout);
                int read_size = 0;
                while ((read_size = fread(buffer, sizeof(char), MAX_BUFFER_SIZE, file)) != 0) {
                    send(data_socket, buffer, read_size, 0);
                    log("To " + client_ip_address + ", " + std::string(buffer, read_size), std::cout);
                }
                fclose(file);
            } else {
                sprintf(buffer, "551\r\n");
                send(command_socket, buffer, strlen(buffer), 0);
            }
        } else if (command == "stor" || command == "appe") {
            int totalSize = 0;
            commandStream >> totalSize;
            if (totalSize <= 0)
            {
                recv(command_socket, buffer, MAX_BUFFER_SIZE, 0);
                totalSize = atoi(buffer);
            }
            log("Total Size:" + std::to_string(totalSize), std::cout);
            std::string filename = exec("cd " + workingDirectory + " \n realpath " + content);
            FILE *file = NULL;
            if (command == "stor")
                file = fopen(filename.c_str(), "w+");
            else if (command == "appe")
                file = fopen(filename.c_str(), "a+");
            if (file != NULL) {
                int recv_length = 0;
                while (totalSize > 0 && (recv_length = recv(data_socket, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
                    fwrite(buffer, sizeof(char), recv_length, file);
                    totalSize -= recv_length;
                }
                fclose(file);
                sprintf(buffer, "250\r\n");
            } else
                sprintf(buffer, "553\r\n");
            send(command_socket, buffer, strlen(buffer), 0);
        } else if (command == "pwd") {
            sprintf(buffer, "%s\r\n", workingDirectory.c_str());
            send(data_socket, buffer, strlen(buffer), 0);
            sprintf(buffer, "250\r\n");
            send(command_socket, buffer, strlen(buffer), 0);
            log("To " + client_ip_address + ", " + std::string(buffer), std::cout);
        } else if (command == "cwd") {
            if (content.empty())
                sprintf(buffer, "501\r\n");
            else {
                int code = system(("cd " + workingDirectory + " && cd " + content + " && pwd").c_str());
                if (code == 0) {
                    sprintf(buffer, "250\r\n");
                    workingDirectory = exec("cd " + workingDirectory + " \n cd " + content + " \n pwd");
                } else {
                    sprintf(buffer, "550\r\n");
                }
            }
            send(command_socket, buffer, strlen(buffer), 0);
            log("To " + client_ip_address + ", " + std::string(buffer), std::cout);
        } else if (command == "list") {
            log("$ cd" + workingDirectory + " \n ls -alrt " + content, std::cout);
            sprintf(buffer, "%s\r\n", exec("cd " + workingDirectory + " \n ls -alrt " + content).c_str());
            send(data_socket, buffer, strlen(buffer), 0);
            sprintf(buffer, "250\r\n");
            send(command_socket, buffer, strlen(buffer), 0);
            log("To " + client_ip_address + ", " + std::string(buffer), std::cout);
        } else if (command == "nlst") {
            sprintf(buffer, "%s\r\n", exec("cd " + workingDirectory + " \n ls -a" + content).c_str());
            send(data_socket, buffer, strlen(buffer), 0);
            sprintf(buffer, "250\r\n");
            send(command_socket, buffer, strlen(buffer), 0);
            log("To " + client_ip_address + ", " + std::string(buffer), std::cout);
        } else if (command == "rnfr") {
            specifiedFile = content;
            sprintf(buffer, "250\r\n");
            send(command_socket, buffer, strlen(buffer), 0);
        } else if (command == "rnto") {
            if (content.empty())
                sprintf(buffer, "501\r\n");
            else {
                int status = system(("cd " + workingDirectory + " \n mv " + specifiedFile + " " + content).c_str());
                if (status == 0)
                    sprintf(buffer, "250\r\n");
                else
                    sprintf(buffer, "553\r\n");
            }
            send(command_socket, buffer, strlen(buffer), 0);
        } else if (command == "dele") {
            if (content.empty())
                sprintf(buffer, "501\r\n");
            else {
                int status = system(("cd " + workingDirectory + " \n rm " + content).c_str());
                if (status == 0)
                    sprintf(buffer, "250\r\n");
                else
                    sprintf(buffer, "553\r\n");
            }
            send(command_socket, buffer, strlen(buffer), 0);
        } else if (command == "rmd") {
            if (content.empty())
                sprintf(buffer, "501\r\n");
            else {
                int status = system(("cd " + workingDirectory + " \n rm -rf " + content).c_str());
                if (status == 0)
                    sprintf(buffer, "250\r\n");
                else
                    sprintf(buffer, "553\r\n");
            }
            send(command_socket, buffer, strlen(buffer), 0);
        } else if (command == "mkd") {
            if (content.empty())
                sprintf(buffer, "501\r\n");
            else {
                int status = system(("cd " + workingDirectory + " \n mkdir " + content).c_str());
                if (status == 0)
                    sprintf(buffer, "250\r\n");
                else
                    sprintf(buffer, "553\r\n");
            }
            send(command_socket, buffer, strlen(buffer), 0);
        } else if (command == "syst") {
            sprintf(buffer, "%s\r\n", exec("uname -a && lscpu && lsblk && lsusb && lspci").c_str());
            send(data_socket, buffer, strlen(buffer), 0);
            sprintf(buffer, "250\r\n");
            send(command_socket, buffer, strlen(buffer), 0);
            log("To " + client_ip_address + ", " + std::string(buffer), std::cout);
        } else if (command == "help") {
            sprintf(buffer,
                    "Implemented Commands on this server:\n\tQUIT\n\tRETR\n\tSTOR\n\tPWD\n\tCWD\n\tLIST\n\tNLST\n\tRNFR\n\tRNTO\n\tDELE\n\tRMD\n\tMKD\n\tSYST\n\t");
            send(data_socket, buffer, strlen(buffer), 0);
            sprintf(buffer, "250\r\n");
            send(command_socket, buffer, strlen(buffer), 0);
            log("To " + client_ip_address + ", " + std::string(buffer), std::cout);
        } else {
            //Syntax Error
            sprintf(buffer, "500\r\n");
            send(command_socket, buffer, strlen(buffer), 0);
        }
    }

}
