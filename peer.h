#include<iostream>
#include<sys/socket.h>
#include<thread>
#include<arpa/inet.h>
#include<unistd.h>
#include<string>
#include<map>
#include<vector>
#include<stack>
#include<mutex>

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

#include"core.h"
#include"json.hpp"
using namespace std;
using json = nlohmann::json;


class Peer {
    
    private:
        /* data */
    public:
        int currentConnect =0;
        json onlList = json::array();
        json ListPeer = json::array();
        mutex listMutex;
        int listenfd;
        int port;
        int currentSeverSock;
        string addr;
        //vector<files> filesList; // dungf de luu ten file vaf ma hash di kem
        json fileList = json::array();
        json allPeerFileList= json::array();
        Peer() = default;  // default constructor

        json PeerIn(string ip, int port){
            return {{"ip",ip},{"port",port},{"connect",0},{"sock",-1}};// chua connect voi peer nay, se doi sang so khac sau khi connect
        }
        Peer(string& ip, int p) : addr(ip), port(p) {
            //said hello
            cout << "Peer created with IP " << addr << " and port " << port << endl;
            //start listen to other peers
            listenfd = socket(AF_INET, SOCK_STREAM, 0);
            if(listenfd < 0) { 
                cerr << "Cannot create socket\n"; 
            }
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_port = htons(port);
            address.sin_addr.s_addr = inet_addr(addr.c_str());

            if(bind(listenfd, (sockaddr*)&address, sizeof(address)) < 0) { 
                cerr << "Bind error\n"; 
                close(listenfd);
                return;
            }
            if(listen(listenfd, 10)<0){
                cerr << "Listen error\n"; 
                close(listenfd);
                return;
            }
            cout << "Start listening...\n";
            // hashing file
            fileList=listFile();
            cout<<fileList.dump()<<endl;
        }
        ~Peer() {
            cout << "Peer destroyed" << endl;
        }
        void acceptLoop(int maxConnection){    // chi danh cho viec chap nhan yeu cau tai tu peer khac va chia se file
            cout<<"start listenning for request"<<endl;
            while(true){
                if(currentConnect<maxConnection){
                    sockaddr_in otherPeer{};
                    socklen_t otherPeerSize = sizeof(otherPeer);
                    int otherPeerSock = accept(listenfd,(sockaddr*)&otherPeer, &otherPeerSize);
                    if(otherPeerSock<0){
                        cout << "Accept error\n"; 
                        close(otherPeerSock);
                        continue;
                    }
                    thread(&Peer::handleOtherPeerRequest, this, otherPeerSock, otherPeer).detach();
                }
            }
        }
        void handleOtherPeerRequest(int otherPeerSock,sockaddr_in otherPeer ){
            cout<<"handleing req\n";
            uint32_t lenNet;
            if (recvAll(otherPeerSock, &lenNet, sizeof(lenNet)) <= 0) {
                throw runtime_error("Lost connection while receiving length");
            }
            uint32_t len = ntohl(lenNet);
            vector<char> buffer(len);

            if (recvAll(otherPeerSock, buffer.data(), len) <= 0) {
                throw runtime_error("Lost connection while receiving data");
            }

            string jsonStr(buffer.begin(), buffer.end());
            json peerInfo= json::parse(jsonStr);
            string ip = peerInfo["ip"];
            int port = peerInfo["port"];
            cout<<"connected to "<<ip<<":"<<port<<endl;

            for(auto& p : ListPeer){
                if(p["ip"] == ip && p["port"]== port){
                    p["sock"]=otherPeerSock;
                    p["connect"]=1;
                    break;
                }
            }
            string onllist=ListPeer.dump();
            cout<<onllist<<endl;
            recvOtherPeer(otherPeerSock);
        }

        int makeConnect(string ip, int port){
            sockaddr_in otherPeer{};
            otherPeer.sin_family = AF_INET;
            otherPeer.sin_port = htons(port);
            otherPeer.sin_addr.s_addr = inet_addr(ip.c_str());
            int otherPeerSock = socket(AF_INET, SOCK_STREAM, 0);
            if(connect(otherPeerSock ,(sockaddr*) &otherPeer, sizeof(sockaddr_in)) < 0) { 
                cout<< "Connect error\n";              
                close(otherPeerSock );
                return 0;
            }
            cout<<"connected to "<<ip<<":"<<port<<endl;
            json info;
            info["ip"]=this->addr;
            info["port"]=this->port;
            string data = info.dump();

            // Gửi độ dài trước
            uint32_t len = htonl(data.size());
            send(otherPeerSock , &len, sizeof(len),0);
            send(otherPeerSock , data.c_str(), data.size(), 0);
            return otherPeerSock;
        }
        void collectFiles(){
            // tam thoi la se tai file tuan tu theo trong danh sach allpeerfilelist
            lock_guard<mutex> lock(listMutex);
            for(auto it = allPeerFileList.begin();it!=allPeerFileList.end();++it){
                string hash = it.key();          // lấy chuỗi hash
                auto value = it.value();  
                bool exited=0;
                for(auto& file:fileList){
                    if(file["hash"]==hash){
                        exited=1;
                        break;
                    }
                }
                if(exited)continue;
                for(auto uc=value.begin();uc!=value.end();++uc){
                    cout<<"making connect to"<<(*uc)["ip"]<<" "<<(*uc)["port"]<<endl;
                    int otherPeerSock=makeConnect((*uc)["ip"],(*uc)["port"]);
                    if(otherPeerSock==0)continue;
                    cout<<"connected to "<<(*uc)["ip"]<<":"<<(*uc)["port"]<<endl;
                    //thread(&Peer::downloadFileFrom, this,otherPeerSock, (*uc)["filename"],hash).detach();
                    downloadFileFrom(otherPeerSock,(*uc)["filename"],hash);
                }
            }
            cout<<"Finish collecting!\n";
        }
        void recvOtherPeer(int otherPeerSock){
            //while(true){
                // cout<<"listening from socket: "<<otherPeerSock<<endl;
                // uint32_t lenMess;
                // if (recvAll(otherPeerSock, &lenMess, sizeof(lenMess)) <= 0) {
                //     cout<<"Lost connection while receiving length of message"<<endl;
                //     close(otherPeerSock);
                // }

                // uint32_t len = ntohl(lenMess);
                // vector<char> buffer(len);

                // if (recvAll(otherPeerSock, buffer.data(), len) <= 0) {
                //     cout<<"Lost connection while receiving message"<<endl;
                //     close(otherPeerSock);
                // }
                // string mess(buffer.begin(), buffer.end());
                cout<<"start recieved download req\n"<<endl;
                string mess=recvMessage(otherPeerSock);
                cout<<"received: "<<mess<<endl;
                if(mess=="download"){
                    thread(&Peer::sendFileTo, this ,otherPeerSock).detach();
                }
            //}
        }
        void downloadFileFrom(int otherPeerSock,string fileName,string hash){
            cout<<"start download "<<fileName<<" from socket "<<otherPeerSock<<endl;
            // string mess="download";
            // uint32_t lenMess=htonl(mess.size());
            // send(otherPeerSock,mess.c_str(),lenMess,0);
            sendMessage("download",otherPeerSock);
            string approve=recvMessage(otherPeerSock);
            cout<<"recv :"<<approve<<endl;
            

            // uint32_t lenApprove;
            // if (recvAll(otherPeerSock, &lenApprove, sizeof(lenApprove)) <= 0) {
            //     cout<<"Lost connection while receiving length of approvement"<<endl;
            //     close(otherPeerSock);
            // }

            // uint32_t len = ntohl(lenApprove);
            // vector<char> buffer(len);

            // if (recvAll(otherPeerSock, buffer.data(), len) <= 0) {
            //     cout<<"Lost connection while receiving approvement"<<endl;
            //     close(otherPeerSock);
            // }
            // string approve(buffer.begin(), buffer.end());
            
            if(approve=="ok"){
                sendMessage(fileName,otherPeerSock);
                string saveDir = "/home/purpleguy404/c++/P2P1/Files/"; 
                FileHeader header{};
                if(recv(otherPeerSock, &header, sizeof(header), 0) <= 0) { cout << "Header recv error\n"; return; }
                uint32_t total_chunks = ntohl(header.total_chunks);
                cout << "Receiving file: " << header.filename << ", total chunks: " << total_chunks <<"hashcode: "<<hash<< endl;

                // Mở file để ghi trực tiếp
                string fullPath = saveDir + header.filename;
                ofstream outfile(fullPath, ios::binary);
                if(!outfile.is_open()) { cerr << "Cannot open output file\n"; return; }

                for(uint32_t i = 0; i < total_chunks; i++) {
                    uint32_t chunk_size_network{};
                    if(recv(otherPeerSock, &chunk_size_network, sizeof(chunk_size_network), 0) <= 0) { cout << "Chunk size recv error\n"; break; }
                    uint32_t chunk_size = ntohl(chunk_size_network);

                    vector<char> buf(chunk_size);
                    size_t bytes_received = 0;
                    while(bytes_received < chunk_size) {
                        int n = recv(otherPeerSock, buf.data() + bytes_received, chunk_size - bytes_received, 0);
                        if(n <= 0) { cerr << "Connection closed or error\n"; break; }
                        bytes_received += n;
                    }

                    outfile.write(buf.data(), bytes_received);
                    cout << "Received chunk " << i << " (" << bytes_received << " bytes)\n";
                }

                outfile.close();
                if(sha256FileToHex(fullPath)==hash){
                    cout << "File reconstructed successfully: " << fullPath << endl;
                    sendTrackerFileList(currentSeverSock);
                }
                else{
                    cout<<"failed files\n";
                }
            }
            else{
                cout<<"the otherPeer dont want to sendfile"<<endl;
            }
        }
        void sendFileTo(int otherPeerSock){

            sendMessage("ok",otherPeerSock);
            string fileName = recvMessage(otherPeerSock);
            cout << "[SENDER] client wants: " << fileName << endl;
            if(fileName=="failed to recv message"){
                cout<<"peer failed to send file name\n";
            }
            else cout<<"start sending: "<<fileName<<endl;
            

            ifstream infile(fileName, ios::binary);
            if(!infile.is_open()) { cout << "Cannot open file\n";}

            vector<char> filedata((istreambuf_iterator<char>(infile)), istreambuf_iterator<char>());
            infile.close();
            if(filedata.empty()) { cout << "File empty\n"; }
            int chunk_size = 1024*128; // mỗi chunk KB
            uint32_t total_chunks = (filedata.size() + chunk_size - 1) / chunk_size;

            FileHeader header{};
            std::string baseName = fileName.substr(fileName.find_last_of("/\\") + 1);
            strncpy(header.filename, baseName.c_str(), 256);
            header.total_chunks = htonl(total_chunks);
            send(otherPeerSock, &header, sizeof(header), 0);
            for(uint32_t i = 0; i < total_chunks; i++) {
                uint32_t size = (filedata.size() - i*chunk_size < chunk_size) ? (filedata.size() - i*chunk_size) : chunk_size;
                uint32_t size_network = htonl(size);
                send(otherPeerSock, &size_network, sizeof(size_network), 0);
                //sleep(1);
                send(otherPeerSock, filedata.data() + i*chunk_size, size, 0);
                cout << "Sent chunk " << i << " (" << size << " bytes)\n";
            }
            cout << "Finished send file\n";
        }


        // tracker
        void recvOnlList(int toServerSock) {
            uint32_t lenOnlList;
            if (recvAll(toServerSock, &lenOnlList, sizeof(lenOnlList)) <= 0) {
                throw runtime_error("Lost connection while receiving length");
            }

            uint32_t len = ntohl(lenOnlList);
            vector<char> buffer(len);

            if (recvAll(toServerSock, buffer.data(), len) <= 0) {
                throw runtime_error("Lost connection while receiving onl list");
            }

            string jsonStr(buffer.begin(), buffer.end());
            onlList = json::parse(jsonStr);
            cout<<"recved the onlList"<<endl;
        }

        void recvFileList(int toServerSock){
            uint32_t lenFileList;
            if (recvAll(toServerSock, &lenFileList, sizeof(lenFileList)) <= 0) {
                throw runtime_error("Lost connection while receiving file list length");
            }

            uint32_t len = ntohl(lenFileList);
            vector<char> buffer(len);

            if (recvAll(toServerSock, buffer.data(), len) <= 0) {
                throw runtime_error("Lost connection while receiving file list");
            }

            string jsonStr(buffer.begin(), buffer.end());
            allPeerFileList = json::parse(jsonStr);
            cout<<"recved the fileList"<<endl;
        }


        void recvTracker(int toServerSock){
            while(true){
                //recv message from tracker
                uint32_t lenMess;
                if (recvAll(toServerSock, &lenMess, sizeof(lenMess)) <= 0) {
                    throw runtime_error("Lost connection while receiving length");
                }

                uint32_t len = ntohl(lenMess);
                vector<char> buffer(len);

                if (recvAll(toServerSock, buffer.data(), len) <= 0) {
                    throw runtime_error("Lost connection while receiving message");
                }

                string mess(buffer.begin(), buffer.end());
                //cout<<"recved the message: "<<mess<<endl;
                //if tracker send onllist
                if(mess=="filelist"){
                    recvFileList(toServerSock);
                    cout<<allPeerFileList.dump()<<endl;
                }
                else if(mess=="peerlist"){
                    recvOnlList(toServerSock);
                    for(auto& j : onlList){
                        string ip = j["ip"];
                        int port = j["port"];
                        if(ip==this->addr && port==this->port) continue;
                        // kiểm tra xem đã tồn tại chưa
                        bool exists = false;
                        for(auto& p : ListPeer){
                            if(p["ip"] == ip && p["port"]== port){
                                exists = true;
                                break;
                            }
                        }

                        if(!exists){
                            ListPeer.push_back(PeerIn(ip, port));
                        }
                    }
                    // 2. Xóa peer đã mất
                    for(auto it = ListPeer.begin(); it != ListPeer.end(); ){
                        string ip = (*it)["ip"];
                        int port = (*it)["port"];

                        bool stillExists = false;
                        for(auto& j : onlList){
                            if(j["ip"] == ip && j["port"] == port){
                                stillExists = true;
                                break;
                            }
                        }

                        if(!stillExists){
                            it = ListPeer.erase(it); // xóa và trả về iterator mới
                        } else {
                            ++it;
                        }
                    }
                    string onllist=ListPeer.dump();
                    cout<<onllist<<endl;
                }
            }
        }
        void sendTrackerFileList(int toServerSock){
            //lock_guard<mutex> lock(listMutex);
            sendMessage("filelist",toServerSock);
            string data = fileList.dump();
            // Gửi độ dài trước
            uint32_t len = htonl(data.size());
            send(toServerSock, &len, sizeof(len), 0);
            send(toServerSock, data.c_str(), data.size(), 0);
        }
        void handleTracker(int toServerSock){//,sockaddr_in tracker){
            currentSeverSock=toServerSock;
            cout<<"server socket: "<<currentSeverSock<<endl;
            json info;
            info["ip"]=this->addr;
            info["port"]=this->port;
            string data = info.dump();

            // Gửi độ dài trước
            uint32_t len = htonl(data.size());
            send(toServerSock, &len, sizeof(len),0);
            send(toServerSock, data.c_str(), data.size(), 0);
            thread(&Peer::recvTracker,this, toServerSock).detach();
            thread(&Peer::sendTrackerFileList,this, toServerSock).detach();
        }
        json listFile(){
            lock_guard<mutex> lock(listMutex);
            json res= json::array();
            fs::path exePath = fs::current_path();        // thư mục đang chạy app
            fs::path folderPath = exePath / "Files";  

            for (const auto & entry : fs::directory_iterator(folderPath)) {
                if (entry.is_regular_file()) {
                    string filename=entry.path().string();
                    string hashcode=sha256FileToHex(filename);
                    res.push_back({{"filename",filename},{"hash",hashcode}});
                }
            }
            return res;
        }
        //recvfile() // (trong recv co luon phan rebuild file)
};


