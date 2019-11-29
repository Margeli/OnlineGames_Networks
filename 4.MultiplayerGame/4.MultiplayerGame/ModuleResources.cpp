#include "Networks.h"


#if defined(USE_TASK_MANAGER)
class TaskLoadTexture : public Task
{
public:

	const char *filename = nullptr;
	Texture *texture = nullptr;

	void execute() override
	{
		texture = App->modTextures->loadTexture(filename);
	}
};
#endif


bool ModuleResources::init()
{
	background = App->modTextures->loadTexture("background.jpg");

#if !defined(USE_TASK_MANAGER)
	space = App->modTextures->loadTexture("space_background.jpg");
	asteroid1 = App->modTextures->loadTexture("asteroid1.png");
	asteroid2 = App->modTextures->loadTexture("asteroid2.png");
	spacecraft1 = App->modTextures->loadTexture("spRed.png");
	spacecraft2 = App->modTextures->loadTexture("spOrange.png");
	spacecraft3 = App->modTextures->loadTexture("spYellow.png");
	spacecraft4 = App->modTextures->loadTexture("spGreen.png");
	spacecraft5 = App->modTextures->loadTexture("spBlue.png");
	spacecraft6 = App->modTextures->loadTexture("spPink.png");
	loadingFinished = true;
	completionRatio = 1.0f;
#else
	loadTextureAsync("space_background.jpg", &space);
	loadTextureAsync("asteroid1.png",        &asteroid1);
	loadTextureAsync("asteroid2.png",        &asteroid2);
	loadTextureAsync("spRed.png",      &spacecraft1);
	loadTextureAsync("spOrange.png",      &spacecraft2);
	loadTextureAsync("spYellow.png",      &spacecraft3);
	loadTextureAsync("spGreen.png",		 &spacecraft4);
	loadTextureAsync("spBlue.png",		 &spacecraft5);
	loadTextureAsync("spPink.png",		 &spacecraft6);
	loadTextureAsync("laser.png",            &laser);
#endif

	return true;
}

#if defined(USE_TASK_MANAGER)

void ModuleResources::loadTextureAsync(const char * filename, Texture **texturePtrAddress)
{
	TaskLoadTexture *task = new TaskLoadTexture;
	task->owner = this;
	task->filename = filename;
	App->modTaskManager->scheduleTask(task, this);

	taskResults[taskCount].texturePtr = texturePtrAddress;
	taskResults[taskCount].task = task;
	taskCount++;
}

void ModuleResources::onTaskFinished(Task * task)
{
	ASSERT(task != nullptr);

	TaskLoadTexture *taskLoadTexture = dynamic_cast<TaskLoadTexture*>(task);

	for (int i = 0; i < taskCount; ++i)
	{
		if (task == taskResults[i].task)
		{
			*(taskResults[i].texturePtr) = taskLoadTexture->texture;
			finishedTaskCount++;
			delete task;
			task = nullptr;
			break;
		}
	}

	ASSERT(task == nullptr);

	if (finishedTaskCount == taskCount)
	{
		finishedLoading = true;
	}
}

#endif
