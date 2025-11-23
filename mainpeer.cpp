#include<iostream>
#include<sys/socket.h>
#include<thread>
#include<arpa/inet.h>
#include<unistd.h>
#include<string>
#include<map>
#include<vector>
#include<stack>

#include "core.h"
#include "peer.h"
#include "tracker.h"

using namespace std;

int main(){
    string ip="127.0.0.1";
    Peer mypeer(ip,54001);

    int toServerSock=socket(AF_INET, SOCK_STREAM, 0);
    // sockaddr_in myaddress{};
    // myaddress.sin_family = AF_INET;
    // myaddress.sin_port = htons(54011);
    // myaddress.sin_addr.s_addr = inet_addr(mypeer.addr.c_str());
    // if(bind(toServerSock, (sockaddr*)&myaddress, sizeof(myaddress)) < 0) { 
    //     cerr << "Bind error\n"; 
    //     close(toServerSock);
    //     return 0;
    // }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(54000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if(connect(toServerSock, (sockaddr*)&server, sizeof(server)) < 0) { 
        cerr << "Connect to server failed\n";
        return 0; 
    }
    thread(&Peer::handleTracker, &mypeer, toServerSock, server).detach();
    thread(&Peer::acceptLoop,&mypeer);
    cout << "Press ENTER to exit...\n";
    while(true){
        if(cin.get() == '\n'){
            cout << "Exiting program.\n";
            break;
        }
    }
    close(toServerSock);
}