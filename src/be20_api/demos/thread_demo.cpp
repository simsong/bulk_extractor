/*
 * Example of C++11 threads and atomic variables
 */

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>

std::atomic<int> v(0);
int x(0);

void adder()
{
    for(int i=0;i<1000000;i++){
        v += 1;
        x += 1;
    }
}

int main(int argc, char **argv)
{
    std::thread *t[10];
    for(int i=0;i<10;i++){
        std::cout << "i=" << i << std::endl;
        t[i] = new std::thread(adder);
    }
    for(int i=0;i<10;i++){
        t[i]->join();
    }
    std::cout << "v=" << v << std::endl;
    std::cout << "x=" << x << std::endl;
    return(0);
}

