#ifndef TEST_JEMALLOC_ALLOC_NEW_H
#define TEST_JEMALLOC_ALLOC_NEW_H

#include "pool_perthread_and_shared.h"



template<typename T>
class AllocNew{
    typedef allocator_new<T>  Allocator;
    typedef  pool_perthread_and_shared<T> Pool;

public:

    AllocNew(){}

    AllocNew(int thread_num){
        cout<<"*pool*"<<endl;
        alloc = new Allocator(thread_num, nullptr);
        pool = new Pool(thread_num, alloc, nullptr);
    }

    void initThread(int tid){
        pool->initThread(tid);
    }

    T * allocate(int tid){
        return pool->get(tid);
    }

    void deallocate(int tid,T * p){
        pool->add(tid,p);
    }

private:
    allocator_new<T> * alloc;
    pool_perthread_and_shared<T> *pool;
};
#endif //TEST_JEMALLOC_ALLOC_NEW_H
