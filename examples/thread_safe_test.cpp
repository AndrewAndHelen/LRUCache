#include "LRUCache.hpp"
#include <iostream>
#include <string>
#include <chrono>

using namespace std;

using LRUCache = LRUCache_<int, string>;
using LRUCachePtr = std::shared_ptr<LRUCache>;

void ThreadTestPut(LRUCachePtr cache, int startIdx, int endIdx)
{
    while (startIdx < endIdx) {
        string data = "This is first thread, Put:" + std::to_string(startIdx);

        if (cache->Put(startIdx, data)) {
            cout << "Success " <<data << endl;
        } else {
            cout << "False  " << data << endl;
        }

        this_thread::sleep_for(chrono::milliseconds(10));

        ++startIdx;
    }
}

void ThreadTestGet(LRUCachePtr cache, int startIdx, int endIdx)
{
    while (startIdx < endIdx) {
        string data = "This is first thread, Get:" + std::to_string(startIdx);

        auto suc = cache->Get(startIdx);

        if (suc.second) {
            cout << "Success " << data << endl;
        }
        else {
            cout << "False  " << data << endl;
        }

        this_thread::sleep_for(chrono::milliseconds(10));

        ++startIdx;
    }
}

int main(int argc, char** argv)
{
    int maxSize = 100;

    auto cache = std::make_shared<LRUCache>(maxSize);

    thread t1(ThreadTestPut, cache, 0, 100);
    thread t2(ThreadTestPut, cache, 100, 200);

    thread t3(ThreadTestGet, cache, 0, 100);
    thread t4(ThreadTestGet, cache, 100, 200);

    t1.join();
    t2.join();

    t3.join();
    t4.join();

    return 0;
}


