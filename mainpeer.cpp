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
    int maxConnection=10;
    string ip="127.0.0.1";
    Peer mypeer(ip,54001);

    int toServerSock=socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(54000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if(connect(toServerSock, (sockaddr*)&server, sizeof(server)) < 0) { 
        cerr << "Connect to server failed\n";
        return 0; 
    }
    thread(&Peer::handleTracker, &mypeer, toServerSock).detach();
    thread(&Peer::acceptLoop,&mypeer,maxConnection).detach();
    cout << "Press 's' to collect files, ENTER to skip\n";
    char c = cin.get();
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // loại bỏ phần còn sót trong buffer

    if(c == 's'){
        cout << "Starting collect files...\n";
        mypeer.collectFiles();
    } else {
        cout << "Did not collect files\n";
    }

    cout << "Press ENTER to exit...\n";
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // đợi ENTER
    cout << "Exiting program.\n";
    close(toServerSock);
}