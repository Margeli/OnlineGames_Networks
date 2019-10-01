#include "ModuleTaskManager.h"


void ModuleTaskManager::threadMain()
{
	Task* t = nullptr;
	while (true) {
		{
			// TODO 3:
			// - Wait for new tasks to arrive
			// - Retrieve a task from scheduledTasks
			// - Execute it
			// - Insert it into finishedTasks
			std::unique_lock<std::mutex> lock(mtx);

			while (scheduledTasks.empty() && !exitFlag) {
				event.wait(lock);
			}
			if (exitFlag) {
				break;
			}
			else {
				t = scheduledTasks.front();
				scheduledTasks.pop();
			}
		}
		t->execute();
		{
			std::unique_lock<std::mutex> lock(mtx);
			finishedTasks.push(t);
		}


	}
}

bool ModuleTaskManager::init()
{
	// TODO 1: Create threads (they have to execute threadMain())

	for (auto &thread : threads)
	{
		thread = std::thread(&ModuleTaskManager::threadMain, this);
	}


	return true;
}

bool ModuleTaskManager::update()
{
	// TODO 4: Dispatch all finished tasks to their owner module (use Module::onTaskFinished() callback)
	if (!finishedTasks.empty()) {

		std::unique_lock<std::mutex> lock(mtx);

		while (!finishedTasks.empty()) {
			Task* t = finishedTasks.front();
			t->owner->onTaskFinished(t);
			finishedTasks.pop();
		}
	}
	return true;
}

bool ModuleTaskManager::cleanUp()
{
	// TODO 5: Notify all threads to finish and join them
	{
		std::unique_lock<std::mutex>lock(mtx);
	
	exitFlag = true;
	event.notify_all();
	}
	for (auto &thread : threads)
	{
		thread.join();
	}
	
	return true;
}

void ModuleTaskManager::scheduleTask(Task *task, Module *owner)
{
	std::unique_lock<std::mutex> lock(mtx);
	task->owner = owner;
	scheduledTasks.push(task);
	event.notify_one();

	// TODO 2: Insert the task into scheduledTasks so it is executed by some thread
}
