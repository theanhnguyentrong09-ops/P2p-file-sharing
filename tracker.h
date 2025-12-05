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

#include"core.h"
#include"json.hpp"
using namespace std;
using json = nlohmann::json;



class Tracker {
    
    //vector<location>listpeers;
    private:
        /* data */
    public:
        int listenfd;
        int port;
        string addr;
        json onlList = json::array();
        json allPeerFileList= json::array();
        mutex listMutex;
        unordered_map<string, vector<json>> fileMap;
        //vector<pair<int,location>> peerInfos;
        Tracker() = default;  // default constructor

        Tracker(string& ip, int p) : addr(ip), port(p) {
            //said hello
            cout << "Tracker created with IP " << addr << " and port " << port << endl;

            //start listen to other Trackers
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
            
        }
        ~Tracker() {
            cout << "Tracker destroyed" << endl;
        }
        void acceptLoop(){
            while(true){
                sockaddr_in client{};
                socklen_t clientSize = sizeof(client);
                int clientSock = accept(listenfd,(sockaddr*)&client, &clientSize);
                if(clientSock<0){
                    cout << "Accept error\n"; 
                    close(clientSock);
                    continue;
                }
                thread(&Tracker::handleClient, this, clientSock, client).detach();
            }
        }

        void handleClient(int clientSock ,sockaddr_in client){
            uint32_t lenNet;
            if (recvAll(clientSock, &lenNet, sizeof(lenNet)) <= 0) {
                throw runtime_error("Lost connection while receiving length");
            }

            uint32_t len = ntohl(lenNet);
            vector<char> buffer(len);

            if (recvAll(clientSock, buffer.data(), len) <= 0) {
                throw runtime_error("Lost connection while receiving data");
            }

            string jsonStr(buffer.begin(), buffer.end());
            json peerInfo= json::parse(jsonStr);
            cout<<"recved the peer info"<<endl;

            string ip = peerInfo["ip"];
            int port = peerInfo["port"];
            json newPeer = {{"ip", ip}, {"port", port}, {"sock", clientSock}};
            {
                lock_guard<mutex> lock(listMutex);
                //json toSendNewUser={{"ip", ip}, {"port", port}};
                onlList.push_back(newPeer);
                //peerInfos.push_back({clientSock,{port,ip}});
            }
            thread(&Tracker::sendClient, this ,clientSock ,newPeer).detach();
            thread(&Tracker::recvClient, this ,clientSock ,newPeer).detach();
        }
        void sendClient(int clientSock, json peer ){

            // string ip = inet_ntoa(client.sin_addr);
            // int port = ntohs(client.sin_port);
            //sendOnlList(clientSock);
            anounceUpdateOnlList();
            // nhan heartbeat tu client 
            
            
        }
        void recvClient(int clientSock,json peer){
            string mess=recvMessage(clientSock);
            if(mess=="failed to recv message"){
                cout<<"failed to recv the file list\n";
                return;
            }
            else if(mess=="filelist"){
                uint32_t lenNet;
                if (recvAll(clientSock, &lenNet, sizeof(lenNet)) <= 0) {
                    throw runtime_error("Lost connection while receiving peer list file length");
                }

                uint32_t len = ntohl(lenNet);
                vector<char> buffer0(len);

                if (recvAll(clientSock, buffer0.data(), len) <= 0) {
                    throw runtime_error("Lost connection while receiving peer list file");
                }

                string jsonStr(buffer0.begin(), buffer0.end());
                json peerFileList = json::parse(jsonStr);
                {
                    lock_guard<mutex> lock(listMutex);
                    allPeerFileList.push_back({peerFileList,{"ip",peer["ip"]},{"port",peer["port"]}});
                    for (auto& file : peerFileList) {
                        string hash = file["hash"];
                        //cout<<hash<<endl;
                        // tạo object lưu thông tin
                        json entry = {
                            {"filename", file["filename"]},
                            {"ip", peer["ip"]},
                            {"port", peer["port"]}
                        };

                        // chỉ thêm tên file vào nếu chưa tồn tại trong map[hash]
                        bool exists = false;
                        for (auto& f : fileMap[hash]) {
                            if (f["filename"] == file["filename"] && f["ip"] == peer["ip"] && f["port"] == peer["port"]) {
                                exists = true;
                                break;
                            }
                        }

                        if (!exists) {
                            fileMap[hash].push_back(entry);
                        }
                    }
                }
                cout<<"recved the fileList"<<endl;
                // cout<<allPeerFileList.dump()<<endl;
                cout<<makeFileList().dump(3)<<endl;
                anounceUpdateFileList();
            }
            //char buffer[1024];
            while (true) {
                //int byteRecv = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
                string mess2=recvMessage(clientSock);
                cout<<"received :"<<mess2<<endl;
                if (mess2 == "failed to recv message") {
                    //co the viet ham rieng handle offline
                    {
                        lock_guard<mutex> lock(listMutex);
                        for (auto i = onlList.begin(); i != onlList.end(); i++) {
                            if ((*i)["sock"] == clientSock) {
                                cout << "Client disconnected: " << (*i)["ip"] << ":" << (*i)["port"] << endl;
                                string deadIP   = (*i)["ip"];
                                int deadPort    = (*i)["port"];
                                onlList.erase(i);

                                // 2) Xóa toàn bộ quyền sở hữu file của peer đó
                                for (auto it = fileMap.begin(); it != fileMap.end(); ) {
                                    auto& vec = it->second;

                                    vec.erase(
                                        remove_if(vec.begin(), vec.end(),
                                            [&](const json& entry){
                                                return entry["ip"] == deadIP && entry["port"] == deadPort;
                                            }),
                                        vec.end()
                                    );

                                    // nếu hash không còn ai sở hữu thì drop khỏi map luôn
                                    if (vec.empty()) {
                                        it = fileMap.erase(it);
                                    } else {
                                        ++it;
                                    }
                                }
                                // Xóa trong allPeerFileList
                                for (auto it = allPeerFileList.begin(); it != allPeerFileList.end(); ) {
                                    json peerFileData = (*it)[0]; // danh sách file
                                    string ip   = (*it)[1][1];    // hơi dị nhưng đúng cấu trúc ông đang đúc
                                    int port    = (*it)[2][1];

                                    if (ip == deadIP && port == deadPort) {
                                        it = allPeerFileList.erase(it);
                                    } else {
                                        ++it;
                                    }
                                }
                                cout << "Updated fileMap:\n" << json(fileMap).dump(4) << endl;
                                break;
                            }
                        }
                        cout << onlList.dump() << endl;
                        for (auto it = allPeerFileList.begin(); it != allPeerFileList.end(); ) {
                            json peerFileData = (*it)[0]; // danh sách file
                            string ip   = (*it)[1][1];    // hơi dị nhưng đúng cấu trúc ông đang đúc
                            int port    = (*it)[2][1];

                            if (ip == peer["ip"] && port == peer["port"]) {
                                it = allPeerFileList.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                    close(clientSock);
                    anounceUpdateOnlList();
                    anounceUpdateFileList();
                    break;
                }
                else if(mess2=="filelist"){
                    uint32_t lenNet;
                    if (recvAll(clientSock, &lenNet, sizeof(lenNet)) <= 0) {
                        throw runtime_error("Lost connection while receiving length");
                    }

                    uint32_t len = ntohl(lenNet);
                    vector<char> buffer0(len);

                    if (recvAll(clientSock, buffer0.data(), len) <= 0) {
                        throw runtime_error("Lost connection while receiving data");
                    }

                    string jsonStr(buffer0.begin(), buffer0.end());
                    json peerFileList = json::parse(jsonStr);
                    {
                        lock_guard<mutex> lock(listMutex);
                        allPeerFileList.push_back({peerFileList,{"ip",peer["ip"]},{"port",peer["port"]}});
                        for (auto& file : peerFileList) {
                            string hash = file["hash"];
                            //cout<<hash<<endl;
                            // tạo object lưu thông tin
                            json entry = {
                                {"filename", file["filename"]},
                                {"ip", peer["ip"]},
                                {"port", peer["port"]}
                            };

                            // chỉ thêm tên file vào nếu chưa tồn tại trong map[hash]
                            bool exists = false;
                            for (auto& f : fileMap[hash]) {
                                if (f["filename"] == file["filename"] && f["ip"] == peer["ip"] && f["port"] == peer["port"]) {
                                    exists = true;
                                    break;
                                }
                            }

                            if (!exists) {
                                fileMap[hash].push_back(entry);
                            }
                        }
                    }
                    cout<<"recved the update fileList"<<endl;
                    cout<<makeFileList().dump(3)<<endl;
                    anounceUpdateFileList();
                }
            }
        }
        
        //sendListOnl()
        

        // void sendOnlList(int clientSock){
        //     lock_guard<mutex> lock(listMutex);
        //     json clean=makeCleanList();
        //     string data = clean.dump();

        //     // Gửi độ dài trước
        //     uint32_t len = htonl(data.size());
        //     send(clientSock, &len, sizeof(len), 0);
        //     send(clientSock, data.c_str(), data.size(), 0);
        // }
        //anounce to every peer
        json makeCleanList() {
            json arr = json::array();
            for (auto &u : onlList) {
                arr.push_back({
                    {"ip", u["ip"]},
                    {"port", u["port"]}
                });
            }
            return arr;
        }
        void anounceUpdateOnlList(){
            lock_guard<mutex> lock(listMutex);
            json clean=makeCleanList();
            string data = clean.dump();  // chuyển JSON -> string

            // Gửi độ dài trước
            string mess="peerlist";
            uint32_t lenMess=htonl(mess.size());
            uint32_t len = htonl(data.size());
            for(auto i=onlList.begin();i!=onlList.end();i++){
                send((*i)["sock"], &lenMess, sizeof(lenMess), 0);
                send((*i)["sock"], mess.c_str(), mess.size(), 0);
                send((*i)["sock"], &len, sizeof(len), 0);
                send((*i)["sock"], data.c_str(), data.size(), 0);
            }
        }
        //send file list
        json makeFileList(){
            json out;

            for (auto& [hash, fileList] : fileMap) {
                json listJson = json::array();

                for (auto& f : fileList) {
                    listJson.push_back({
                        {"filename", f["filename"]},
                        {"ip",       f["ip"]},
                        {"port",     f["port"]}
                    });
                }

                out[hash] = listJson;
            }
            return out;
        }
        void anounceUpdateFileList(){
            lock_guard<mutex> lock(listMutex);
            json clean=makeFileList();
            string data = clean.dump();  // chuyển JSON -> string

            // Gửi độ dài trước
            string mess="filelist";
            uint32_t lenMess=htonl(mess.size());
            uint32_t len = htonl(data.size());
            for(auto i=onlList.begin();i!=onlList.end();i++){
                send((*i)["sock"], &lenMess, sizeof(lenMess), 0);
                send((*i)["sock"], mess.c_str(), mess.size(), 0);
                send((*i)["sock"], &len, sizeof(len), 0);
                send((*i)["sock"], data.c_str(), data.size(), 0);
            }
        }
};