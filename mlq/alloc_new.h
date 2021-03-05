#ifndef TEST_JEMALLOC_ALLOC_NEW_H
#define TEST_JEMALLOC_ALLOC_NEW_H

#include "multi_level_queue.h"

template<typename T>
class AllocNew{
typedef MultiLevelQueue<T> Mlq;
public:

    AllocNew(){}

    AllocNew(int thread_num){
        mlqs = new Mlq[thread_num];
    }

    void initThread(int tid){}

    T * allocate(int tid ){
        mlqs[tid].allocate(sizeof (T));
    }

    void deallocate(int tid, T * p){
        mlqs[tid].deallocate(p);
    }

private:
    Mlq *mlqs;

};

#endif //TEST_JEMALLOC_ALLOC_NEW_H
