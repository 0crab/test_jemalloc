#include <iostream>
#include <thread>
#include <vector>

#include "timer.h"
//#include "debra/reclaimer_debra.h"
#include "brown_reclaim.h"

using namespace std;

static int THREAD_NUM;
static size_t TEST_NUM;
static int TEST_TIME;
static size_t TEST_BLOCK_SIZE;
static int BATCH_SIZE;

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
//typedef AllocNew<BufferType> Alloc;
//typedef Reclaimer_debra Alloc;

typedef brown_reclaim<BufferType ,allocator_new<BufferType>, pool_perthread_and_shared<>,reclaimer_debra <>> Alloc;

Alloc * m_alloc;
Timer * timer;
ThreadBarrier * threadBarrier;

atomic<int> stopMeasure(0);

thread_local size_t op_index;
thread_local size_t read_cumulate;
size_t g_read_cumulate;

void concurrent_worker(int tid){
    //init
    m_alloc->initThread(tid);

    Block & tblock = blocks[tid];
    for(size_t i = 0; i < TEST_BLOCK_SIZE;i++) {
        tblock.data[i] = reinterpret_cast<BufferType *>(m_alloc->allocate(tid));

        sprintf(tblock.data[i]->buf, "%zu", i);
    }

    for(int i = 0 ; i < BATCH_SIZE; i ++)
        m_alloc->free(reinterpret_cast<uint64_t>(tblock.data[i]));
    op_index = 0;

    //start
    threadBarrier->threadWait();
    timer->startTime(tid);


    while(stopMeasure.load(memory_order_relaxed) == 0){
        //long op_count = 0;

        for(size_t i = 0; i < TEST_NUM / BATCH_SIZE ; i ++){

            for(int j = 0; j < BATCH_SIZE; j++){
                tblock.data[(op_index + j) % TEST_BLOCK_SIZE] = reinterpret_cast<BufferType *>(m_alloc->allocate(tid));
            }

            size_t next_index =( op_index +  BATCH_SIZE ) % TEST_BLOCK_SIZE;

            for(int j = 0; j < BATCH_SIZE; j++){
                read_cumulate += *(size_t *)tblock.data[(next_index + j) % TEST_BLOCK_SIZE];
                m_alloc->free(reinterpret_cast<uint64_t>(tblock.data[(next_index + j) % TEST_BLOCK_SIZE]));
            }

            op_index = next_index;
        }

        timer->add_op_count(tid,TEST_NUM);
        if(timer->fetchTime(tid) / 1000000 >= TEST_TIME){
            stopMeasure.store(1, memory_order_relaxed);
        }
    }

    g_read_cumulate += read_cumulate;
    timer->recordRunTime(tid);

}

int main(int argc, char **argv) {
    if (argc == 6) {
        THREAD_NUM = stol(argv[1]);
        TEST_BLOCK_SIZE = stol(argv[2]);
        TEST_NUM = stol(argv[3]);
        BATCH_SIZE = stol(argv[4]);
        TEST_TIME = stol(argv[5]);
    } else {
        printf("./a.out <thread_num> <block_size> <test_num> <batch_size> <test_time>\n");
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

    cout<<"num prevented optimizations "<<g_read_cumulate<<endl;
    cout<<"throughput "<<timer->getThroughPut()<<endl;

    return 0;
}
