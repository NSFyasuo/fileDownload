#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int num = 0;

pthread_mutex_t mutex;


void* func(void* arg){

    for(int i = 0; i < 50; i++){
        pthread_mutex_lock(&mutex);

        num++;
        printf("线程: %lu, num: %d\n", pthread_self(), num);

        pthread_mutex_unlock(&mutex);
    }

    return 0;
}

int main(){
    
    pthread_t tid1, tid2;

    //创建线程
    pthread_create(&tid1, NULL, func, NULL);
    pthread_create(&tid2, NULL, func, NULL);

    //等待线程结束
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    

    pthread_mutex_destroy(&mutex);

    printf("最终 num = %d\n", num);
    
    return 0;
}