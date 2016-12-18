//
// Created by zy-li14 on 16-12-17.
//

#ifndef PROJECT_UTILITY_H
#define PROJECT_UTILITY_H

#include "network.h"
#include <chrono>
#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <memory>
std::string lower(const std::string &string) {
    std::string ret;
    ret.reserve(string.size());
    for (const auto &c: string) {
        if (c >= 'A' && c <= 'Z')
            ret.push_back(char(c + 'a' - 'A'));
        else
            ret.push_back(c);
    }
    return ret;
}

inline void log(const std::string &string, std::ostream &os) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    os << "[" << std::put_time(std::localtime(&now_c), "%F %T") << "] ";
    os << string;
    os << std::endl;
}

std::string ul2str(unsigned long ip) {
    std::string ret("");
    for (int i = 24; i > 0; i -= 8) {
        ret.append(std::to_string((ip >> i) & 0xFF) + ".");
    }
    ret.append(std::to_string(ip & 0xFF));
    return ret;
}

unsigned long str2ul(const std::string &str) {
    unsigned long h1 = 0, h2 = 0, h3 = 0, h4 = 0;
    sscanf(str.c_str(), "%lu,%lu,%lu,%lu", &h4, &h3, &h2, &h1);
    return ((h4 & 0xFF) << 24) | ((h3 & 0xFF) << 16) | ((h2 & 0xFF) << 8) | (h1 & 0xFF);
}

struct sockaddr_in construct_sockaddr(unsigned long ip, unsigned short port) {
    sockaddr_in ret;
    ret.sin_family = AF_INET;
    ret.sin_addr.s_addr = htonl(ip);
    ret.sin_port = htons(port);
    return ret;
}

struct sockaddr_in construct_sockaddr(const std::string &ip, unsigned short port) {
    if (ip.empty())
        return construct_sockaddr(INADDR_ANY, port);
    else
        return construct_sockaddr(str2ul(ip), port);
}

std::string encode_address(unsigned long addr, unsigned short port)
{
    char ret[128];
    sprintf(ret, "%lu,%lu,%lu,%lu,%hu,%hu",
            (addr >> 24) & 0xFF,
            (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF,
            addr & 0xFF,
            (port >> 8) & 0xFF,
            port & 0xFF);
    return ret;
}
void decode_address(const std::string& string, unsigned long& ip, unsigned short& port)
{
    unsigned long h1 = 0, h2 = 0, h3 = 0, h4 = 0;
    unsigned short p1 = 0, p2 = 0;
    sscanf(string.c_str(), "%lu,%lu,%lu,%lu,%hu,%hu", &h4, &h3, &h2, &h1, &p2, &p1);
    ip = ((h4 & 0xFF) << 24) | ((h3 & 0xFF) << 16) | ((h2 & 0xFF) << 8) | (h1 & 0xFF);
    port = ((p2 & 0xFF) << 8) | (p1 & 0xFF);
}

std::string strip(const std::string& str)
// remove ending spaces
{
    char last;
    std::string ret = str;
    while (true)
    {
        last = ret.back();
        if (last == '\n' || last == '\r' || last == ' ' || last == '\t')
            ret.pop_back();
        else
            break;
    }
    return ret;
}

std::string exec(const std::string& command)
{
    char buffer[128];
    std::string result = "";
    std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return strip(result);
}

#endif //PROJECT_UTILITY_H
