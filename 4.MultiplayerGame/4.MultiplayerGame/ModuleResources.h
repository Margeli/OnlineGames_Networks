#pragma once

#define USE_TASK_MANAGER

struct Texture;

class ModuleResources : public Module
{
public:

	Texture *background = nullptr;
	Texture *space = nullptr;
	Texture *asteroid = nullptr;
	Texture *spacecraft1 = nullptr;
	Texture *spacecraft2 = nullptr;
	Texture *spacecraft3 = nullptr;
	Texture *spacecraft4 = nullptr;
	Texture *spacecraft5 = nullptr;
	Texture *spacecraft6 = nullptr;
	Texture *laser = nullptr;

	bool finishedLoading = false;
private:

	bool init() override;

#if defined(USE_TASK_MANAGER)
	void onTaskFinished(Task *task) override;

	void loadTextureAsync(const char *filename, Texture **texturePtrAddress);
#endif

	struct LoadTextureResult {
		Texture **texturePtr = nullptr;
		Task *task = nullptr;
	};

	LoadTextureResult taskResults[1024] = {};
	int taskCount = 0;
	int finishedTaskCount = 0;
};

