#ifdef MODULE_SPINE_ENABLED

#ifndef SPINE_BATCHING_QUEUE_H
#define SPINE_BATCHING_QUEUE_H

#include "spine.h"
#include "core/os/semaphore.h"
#include "smartptr.h"

class Spine;

class SpineBatchingQueue {

    SmartPtr<Semaphore> sem;
    SmartPtr<Mutex> mut;
	static SpineBatchingQueue *instance;

	List<Spine *> queue;
	bool exit_thread;
	SmartPtr<Thread> thread;

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