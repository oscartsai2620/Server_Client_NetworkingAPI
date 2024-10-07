// your PA3 BoundedBuffer.cpp code here
#include "BoundedBuffer.h"
#include <assert.h>
#include <cstring>

using namespace std;


BoundedBuffer::BoundedBuffer (int _cap) : cap(_cap) {
    // modify as needed
}

BoundedBuffer::~BoundedBuffer () {
    // modify as needed
}

void BoundedBuffer::push (char* msg, int size) {
    // 1. Convert the incoming byte sequence given by msg and size into a vector<char>
    vector<char> message(msg, msg + size);
    // 2. Wait until there is room in the queue (i.e., queue lengh is less than cap)
    unique_lock<mutex> lck(mtx);
    while((int)q.size() >= cap){
        pop_wait.wait(lck);
    }
    // 3. Then push the vector at the end of the queue
    q.push(message);
    // 4. Wake up threads that were waiting for push
    push_wait.notify_one();
    
}

int BoundedBuffer::pop (char* msg, int size) {
    // 1. Wait until the queue has at least 1 item
    unique_lock<mutex> lck(mtx);
    while(q.empty()){
        push_wait.wait(lck);
    }
    // 2. Pop the front item of the queue. The popped item is a vector<char>
    vector<char> message = q.front();
    q.pop();
    // 3. Convert the popped vector<char> into a char*, copy that into msg; assert that the vector<char>'s length is <= size
    assert((int)message.size() <= size);
    memcpy(msg, message.data(), message.size());
    // 4. Wake up threads that were waiting for pop
    pop_wait.notify_one();
    // 5. Return the vector's length to the caller so that they know how many bytes were popped
    return message.size();
}

size_t BoundedBuffer::size () {
    return q.size();
}
