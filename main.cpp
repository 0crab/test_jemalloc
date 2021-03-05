#include <iostream>
#include <thread>
#include <vector>

#include "timer.h"
#include "new/alloc_new.h"

using namespace std;


static int THREAD_NUM;
static size_t TEST_NUM;
static int TEST_TIME;
static size_t TEST_BLOCK_SIZE;

#define CAT2(x, y) x##y
#define CAT(x, y) CAT2(x, y)

#define PAD64 volatile char CAT(___padding, __COUNTER__)[64]
#define PAD volatile char CAT(___padding, __COUNTER__)[128]



#define BUFFER_TYPE_LEN 50
struct BufferType{
    char buf[BUFFER_TYPE_LEN];
};

struct Block{

    Block() {}

    void init(size_t block_size){
        data = reinterpret_cast<BufferType **>(new BufferType[block_size]);
    }

    BufferType ** data;
    PAD;
};

Block * blocks;
typedef AllocNew<BufferType> Alloc;
Alloc * m_alloc;
Timer * timer;
ThreadBarrier * threadBarrier;

atomic<int> stopMeasure(0);

thread_local size_t op_index;

void concurrent_worker(int tid){
    //init
    m_alloc->initThread(tid);

    Block & tblock = blocks[tid];
    for(size_t i = 0; i < TEST_BLOCK_SIZE;i++) {
        tblock.data[i] = m_alloc->allocate(tid);
        sprintf(tblock.data[i]->buf, "%lld", i);
    }

    m_alloc->deallocate(tid,tblock.data[0]);
    op_index = 0;

    //start
    threadBarrier->threadWait();
    timer->startTime(tid);


    while(stopMeasure.load(memory_order_relaxed) == 0){
        //long op_count = 0;

        for(size_t i = 0; i < TEST_NUM; i ++){
            size_t next_index =( op_index + 1 ) % TEST_BLOCK_SIZE;
            tblock.data[op_index] = m_alloc->allocate(tid);
            m_alloc->deallocate(tid,tblock.data[next_index]);
            op_index = next_index;
        }

        timer->add_op_count(tid,TEST_NUM);
        if(timer->fetchTime(tid) / 1000000 >= TEST_TIME){
            stopMeasure.store(1, memory_order_relaxed);
        }
    }

    timer->recordRunTime(tid);

}

int main(int argc, char **argv) {
    if (argc == 5) {
        THREAD_NUM = stol(argv[1]);
        TEST_BLOCK_SIZE = stol(argv[2]);
        TEST_NUM = stol(argv[3]);
        TEST_TIME = stol(argv[4]);
    } else {
        printf("./a.out <thread_num> <block_size> <test_num> <test_time>\n");
        return 0;
    }

    blocks = new Block[THREAD_NUM];
    for(int i = 0; i < THREAD_NUM; i++){
        blocks[i].init(TEST_BLOCK_SIZE);
    }

    m_alloc = new Alloc(THREAD_NUM);
    timer = new Timer(THREAD_NUM);
    threadBarrier = new ThreadBarrier(THREAD_NUM);


    vector<thread> threads;
    for(int i = 0; i < THREAD_NUM; i++) threads.push_back(thread(concurrent_worker,i));
    for(int i = 0; i < THREAD_NUM; i++) threads[i].join();

    cout<<"throughput "<<timer->getThroughPut()<<endl;

    return 0;
}
