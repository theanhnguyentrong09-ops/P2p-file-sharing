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


class Peer {
    
    private:
        /* data */
    public:
        json onlList = json::array();
        mutex listMutex;
        int listenfd;
        int port;
        string addr;
        vector<files> filesList; // dungf de luu ten file vaf ma hash di kem
        stack<files>filesToShare;
        Peer() = default;  // default constructor

        Peer(string& ip, int p) : addr(ip), port(p) {
            //said hello
            cout << "Peer created with IP " << addr << " and port " << port << endl;

            //connect to server
            
            

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
            
        }
        ~Peer() {
            cout << "Peer destroyed" << endl;
        }
        void acceptLoop(){    // chi danh cho viec chap nhan yeu cau tai tu peer khac va chia se file
            cout<<"start listenning for request"<<endl;
            while(true){
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
        
        void makeConnect(sockaddr* sockaddress){
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if(connect(sockfd, sockaddress, sizeof(sockaddr_in)) < 0) { 
                cout<< "Connect error\n"; 
                close(sockfd);
            }
        }
        
        
        void recvOnlList(int toServerSock) {
            uint32_t lenNet;
            if (recvAll(toServerSock, &lenNet, sizeof(lenNet)) <= 0) {
                throw runtime_error("Lost connection while receiving length");
            }

            uint32_t len = ntohl(lenNet);
            vector<char> buffer(len);

            if (recvAll(toServerSock, buffer.data(), len) <= 0) {
                throw runtime_error("Lost connection while receiving data");
            }

            string jsonStr(buffer.begin(), buffer.end());
            onlList = json::parse(jsonStr);
            cout<<"recved the onlList"<<endl;
        }
        void handleTracker(int toServerSock,sockaddr_in tracker){
            recvOnlList(toServerSock);
            string onllist=onlList.dump();
            cout<<onllist<<endl;
        }
        void handleOtherPeerRequest(int otherPeerSock,sockaddr_in otherPeer ){
            //sendfile()
        }

        //recvfile() // (trong recv co luon phan rebuild file)
};


