// #include<iostream>
// #include "hash_sha256.h"

// #include<iomanip>
// using namespace std;
// using sha256_output_type = std::array<uint8_t, 32>;

// int main(){
//     // Create an object of hash_sha256
//     hash_sha256 hash;

//     // Original message
//     const std::array<std::uint8_t, 3U> msg = {'a', 'b', 'c' };
//     const std::array<uint8_t,3U> mgs={'a','b','c'};
//     // Initialize hash
//     hash.sha256_init();

//     // Update the hash with given data
//     hash.sha256_update(msg.data(), msg.size());
//     cout<<msg.data()<<endl;
//     // Get hash result
//     sha256_output_type hash_result = hash.sha256_final();
//     hash.sha256_init();
//     hash.sha256_update(mgs.data(), mgs.size());
//     sha256_output_type hash_result2 = hash.sha256_final();
//     string i1={},i2={};
//     for(auto i:hash_result){
//         // cout<<hex<<setw(2)<<setfill('0')<<(int)i<<"/";
//         i1+=to_string((int(i)));
//     }
//     cout<<endl;
//     for(auto i:hash_result2){
//         // cout<<hex<<setw(2)<<setfill('0')<<(int)i<<"/";
//         i2+=to_string((int(i)));
//     }
//     cout<<i1<<endl<<i2<<endl<<(i1==i2)<<endl;
// }
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "hash_sha256.h"

using namespace std;
using sha256_output_type = std::array<uint8_t, 32>;

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

int main() {
    string filename = "/home/purpleguy404/c++/P2P/Legends Never Die (ft. Against The Current) ｜ Worlds 2017 - League of Legends.mp3"; // thay bằng file MP3 của bạn
    try {
        string hashHex = sha256FileToHex(filename);
        cout << "SHA-256 hash of file: " << hashHex << endl;
    } catch(const exception& e) {
        cerr << e.what() << endl;
    }
    string filename2 = "/home/purpleguy404/c++/P2P/Legends Never Die (ft. Against The Current) ｜ Worlds 2017 - League of Legends copy.mp3"; // thay bằng file MP3 của bạn
    try {
        string hashHex2 = sha256FileToHex(filename2);
        cout << "SHA-256 hash of file: " << hashHex2 << endl;
    } catch(const exception& e) {
        cerr << e.what() << endl;
    }
    return 0;
}
