#ifndef TEST_JEMALLOC_ALLOC_NEW_H
#define TEST_JEMALLOC_ALLOC_NEW_H

template<typename T>
class AllocNew{

public:

    AllocNew(){}

    AllocNew(int thread_num){
        cout<<"*malloc*"<<endl;
    }

    void initThread(int tid){}

    T * allocate(int tid ){
        return new T;
    }

    void deallocate(int tid, T * p){
        if(p != nullptr)
            free(p);
    }

};

#endif //TEST_JEMALLOC_ALLOC_NEW_H
