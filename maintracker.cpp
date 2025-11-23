#include<iostream>
#include<sys/socket.h>
#include<thread>
#include<arpa/inet.h>
#include<unistd.h>
#include<string>
#include<map>
#include<vector>
#include<stack>

#include"core.h"
#include"peer.h"
#include"tracker.h"

using namespace std;

int main(){
    string ip="127.0.0.1";
    Tracker mytracker(ip,54000);
    thread(&Tracker::acceptLoop, &mytracker).detach();
    cout << "Press ENTER to exit...\n";
    while(true){
        if(cin.get() == '\n'){
            cout << "Exiting program.\n";
            break;
        }
    }
}