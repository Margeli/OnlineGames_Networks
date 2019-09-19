#include "ModuleTaskManager.h"


void ModuleTaskManager::threadMain()
{
	while (!scheduledTasks.empty())
	{
		// TODO 3:
		// - Wait for new tasks to arrive
		// - Retrieve a task from scheduledTasks
		// - Execute it
		// - Insert it into finishedTasks
		std::unique_lock<std::mutex> lock(mtx);
		Task* t =  scheduledTasks.front();
		t->execute();
		finishedTasks.push(t);
		scheduledTasks.pop();
	}
}

bool ModuleTaskManager::init()
{
	// TODO 1: Create threads (they have to execute threadMain())

	for (int i = 0; i < MAX_THREADS; ++i) {
		threads[i] = std::thread(threadMain);
	}


	return true;
}

bool ModuleTaskManager::update()
{
	// TODO 4: Dispatch all finished tasks to their owner module (use Module::onTaskFinished() callback)
	while (!finishedTasks.empty()) {
		Task* t = finishedTasks.front();
		t->owner->onTaskFinished(t);
		finishedTasks.pop();
	}

	return true;
}

bool ModuleTaskManager::cleanUp()
{
	// TODO 5: Notify all threads to finish and join them

	for (int i = MAX_THREADS-1; i >=0; --i) {
		threads[i].join();
	}

	return true;
}

void ModuleTaskManager::scheduleTask(Task *task, Module *owner)
{
	task->owner = owner;
	scheduledTasks.push(task);

	// TODO 2: Insert the task into scheduledTasks so it is executed by some thread
}
