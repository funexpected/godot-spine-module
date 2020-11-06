#ifdef MODULE_SPINE_ENABLED

#include "spine_batching_queue.h"

SpineBatchingQueue *SpineBatchingQueue::instance = NULL;

void SpineBatchingQueue::push(Spine *node) {
    mut->lock();
	if (queue.find(node) == NULL ) {
        queue.push_back(node);
        sem->post();
    }
    mut->unlock();
}

Spine* SpineBatchingQueue::pop() {
    sem->wait();
    mut->lock();
	Spine* node = queue.front()->get();
	queue.pop_front();
    mut->unlock();
	return node;
}

void SpineBatchingQueue::thread_func(void *userdata) {
	SpineBatchingQueue *sbq = (SpineBatchingQueue*)userdata;
	
	while (!sbq->exit_thread) {
		Spine *node = sbq->pop();
		if (node == NULL) return;

		node->_animation_build();
	}
}

void SpineBatchingQueue::create_instance() {
	SpineBatchingQueue::instance = memnew(SpineBatchingQueue);
}

SpineBatchingQueue* SpineBatchingQueue::get_instance() {
	return SpineBatchingQueue::instance;
}

void SpineBatchingQueue::finish() {
	if (!thread) return;
	exit_thread = true;
	Thread::wait_to_finish(thread);
	thread = NULL;

}

SpineBatchingQueue::SpineBatchingQueue() {
	exit_thread = false;
    sem = Semaphore::create();
    mut = Mutex::create();
	thread = Thread::create(SpineBatchingQueue::thread_func, this);
}

#endif