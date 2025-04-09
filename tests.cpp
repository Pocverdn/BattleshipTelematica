#include <iostream>
//#define _UNIX03_THREADS
#include <pthread.h>
#include <time.h>


using namespace std;

void* timed_in(void* att) {
    unsigned char* mgs = (unsigned char*)att;
    scanf("%d%d", mgs[0], mgs[1]);
    return NULL;
}

int main(int argc, char* argv[])
{
    static unsigned char att[2];
    void* arg;
    //unsigned char* att;
    att[0] = 0;
    att[1] = 0;
    arg = att;

    pthread_t thread_id;
    //pthread_create(&thread_id, NULL, timed_in(), (att); //-1 si a acabado 
    pthread_create(&thread_id, NULL, timed_in, arg); //-1 si a acabado

    time_t tiempo;
    time_t current;
    time(&tiempo);
    tiempo = tiempo + 5;
    do {
        time(&current);
    } while ((current < tiempo) & (!att[1]));
    pthread_cancel(thread_id);
    cout << att;

    return 0;
}