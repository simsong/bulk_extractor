#include <pthread.h>

int main(int argc,char **argv)
{
    pthread_mutex_t M;
    pthread_mutex_init(&M,0);
    exit(0);
}
