#include "Pool.h"
#include <iostream>


int main(int argc,char **argv)
{
    Thread::Pool p(10);

    std::cout << "thread count: " << p.Thread_Count();

    p.Add_Task([]() {
                  std::cout << "in a task!\n";
              });
    exit(0);
}

