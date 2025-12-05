
#pragma once // tranh redifine

#include<sys/socket.h>
#include<thread>
#include<arpa/inet.h>
#include <unistd.h>
#include<string>
#include <fstream>
#include <vector>
#include <iomanip>
#include "hash_sha256.h"

using namespace std;
using sha256_output_type = std::array<uint8_t, 32>;

struct files{
    string fileName;
    string hashCode;
};
struct location{
    int port;
    string addr;
};

string sha256FileToHex(const string& filename) {
    ifstream file(filename, ios::binary);
    if(!file.is_open()) {
        throw runtime_error("Cannot open file: " + filename);
    }

    hash_sha256 hash;
    hash.sha256_init();

    const size_t bufferSize = 4096; // đọc file theo chunk 4KB
    vector<uint8_t> buffer(bufferSize);

    while(file.read(reinterpret_cast<char*>(buffer.data()), bufferSize) || file.gcount() > 0) {
        hash.sha256_update(buffer.data(), file.gcount());
    }

    sha256_output_type result = hash.sha256_final();

    // chuyển hash sang hex string
    string hexStr;
    for(auto b : result) {
        stringstream ss;
        ss << hex << setw(2) << setfill('0') << (int)b;
        hexStr += ss.str();
    }

    return hexStr;
}

ssize_t recvAll(int sock, void* buffer, size_t length) {
    size_t total = 0;
    char* buf = (char*)buffer;

    while (total < length) {
        ssize_t bytes = recv(sock, buf + total, length - total, 0);
        if (bytes <= 0) return bytes; // lỗi hoặc mất kết nối
        total += bytes;
    }  
    return total;
}
struct FileHeader {
    char filename[256];
    uint32_t total_chunks;
};
void sendMessage(string mess, int& sock){
    uint32_t lenMess = htonl(mess.size());
    send(sock, &lenMess, sizeof(lenMess), 0);     // gửi độ dài trước
    send(sock, mess.c_str(), mess.size(), 0);      // rồi mới gửi nội dung
}
string recvMessage(int& sock){
    uint32_t lenMess;
    if (recvAll(sock, &lenMess, sizeof(lenMess)) <= 0) {
        //cout<<"Lost connection while receiving length of file name"<<endl;
        return "failed to recv message";
    }

    uint32_t len = ntohl(lenMess);
    vector<char> buffer(len);

    if (recvAll(sock, buffer.data(), len) <= 0) {
        //cout<<"Lost connection while receiving file name"<<endl;
        return "failed to recv message";
    }
    string fileName(buffer.begin(), buffer.end());
    return fileName;
}