#include <iostream>
//#define _UNIX03_THREADS
#include <pthread.h>
#include <time.h>


using namespace std;

void* timed_in(void* att) {

    printf("Thread");
    unsigned short* mgs = (unsigned short*)att;

    printf("%d", mgs[0]);
    cin >> mgs[0] >> mgs[1];
    printf("owo");
    printf("%x", mgs[0]);
    return NULL;
}

int main(int argc, char* argv[])
{
    unsigned short* att = new unsigned short[2];
    void* arg;
    //unsigned char* att;
    att[0] = 3;
    att[1] = 0;
    arg = att;

    pthread_t thread_id;
    //pthread_create(&thread_id, NULL, timed_in(), (att); //-1 si a acabado 
    pthread_create(&thread_id, NULL, timed_in, arg); //-1 si a acabado

    time_t tiempo;
    time_t current;
    time(&tiempo);
    tiempo = tiempo + 10;
    do {

        time(&current);
    } while ((current < tiempo) & (!att[1]));
    printf("%d", att[0]);
    printf("\nball sack\n");
    printf("%d", pthread_cancel(thread_id));
    printf("%d ", att[0]);
    delete att;

    return 0;
}