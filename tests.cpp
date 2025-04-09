#include <iostream>
//#define _UNIX03_THREADS
#include <pthread.h>



using namespace std;

void timed_in() {

}

int main(int argc, char* argv[])
{

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, timed_in(), (void*)args); //-1 si a acabado
    int pthread_cancel(pthread_t thread);

    return 0;
}