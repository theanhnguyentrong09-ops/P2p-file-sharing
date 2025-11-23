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
    int listenfd;
    int port;
    string addr;
    //vector<location>listpeers;
    private:
        /* data */
    public:
        json onlList = json::array();
        mutex listMutex;
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
            string ip = inet_ntoa(client.sin_addr);
            int port = ntohs(client.sin_port);
            cout<<"connected to client"<<ip<<"/"<<port<<endl;
            {
                lock_guard<mutex> lock(listMutex);
                json newUser = {{"ip", ip}, {"port", port}, {"sock", clientSock}};
                json toSendNewUser={{"ip", ip}, {"port", port}};
                onlList.push_back(newUser);
                //peerInfos.push_back({clientSock,{port,ip}});
            }
            sendOnlList(clientSock);
            //anounceUpdateOnlList();
            // nhan heartbeat tu client 
            char buffer[1024];
            while (true) {
                int byteRecv = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
                if (byteRecv <= 0) {
                    cout << "Client disconnected: " << ip << ":" << port << endl;
                    lock_guard<mutex> lock(listMutex);
                    for(auto i=onlList.begin();i!=onlList.end();i++){
                        if((*i)["sock"]==clientSock){
                            onlList.erase(i);
                            break;
                        }   
                    }
                    anounceUpdateOnlList();
                    break;
                }
                buffer[byteRecv] = '\0';
                cout << "[" << ip << "] says: " << buffer << endl;
            }
        }
        //sendListOnl()
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

        void sendOnlList(int clientSock){
            lock_guard<mutex> lock(listMutex);
            json clean=makeCleanList();
            string data = clean.dump();

            // Gửi độ dài trước
            uint32_t len = htonl(data.size());
            send(clientSock, &len, sizeof(len), 0);
            send(clientSock, data.c_str(), data.size(), 0);
        }
        //anounce to every peer
        void anounceUpdateOnlList(){
            lock_guard<mutex> lock(listMutex);
            json clean=makeCleanList();
            string data = clean.dump();  // chuyển JSON -> string

            // Gửi độ dài trước
            uint32_t len = htonl(data.size());
            for(auto i=onlList.begin();i!=onlList.end();i++){
                send((*i)["sock"], &len, sizeof(len), 0);
                send((*i)["sock"], data.c_str(), data.size(), 0);
            }
        }
};