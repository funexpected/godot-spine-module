#ifdef MODULE_SPINE_ENABLED

#ifndef SPINE_BATCHING_QUEUE_H
#define SPINE_BATCHING_QUEUE_H

#include "spine.h"
#include "core/os/semaphore.h"

class Spine;

class SpineBatchingQueue {

    Semaphore *sem;
    Mutex *mut;
	static SpineBatchingQueue *instance;

	List<Spine *> queue;
	bool exit_thread;
	Thread *thread;

public:
	static void thread_func(void *userdata);

	void push(Spine* p_node);
	Spine* pop();
	void finish();
	static void create_instance();
	static SpineBatchingQueue* get_instance();
	SpineBatchingQueue();
	~SpineBatchingQueue();
};

#endif
#endif