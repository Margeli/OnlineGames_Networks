#pragma once

struct Behaviour
{
	GameObject *gameObject = nullptr;

	bool isServer = true;

	virtual void start() { }

	virtual void update() { }

	virtual void onInput(const InputController &input) { }

	virtual void onCollisionTriggered(Collider &c1, Collider &c2) { }
};

struct Spaceship : public Behaviour
{
	float horizontalLimit = 40.0f; 
	float verticalLimit = 500.0f;

	void start() override
	{
		gameObject->tag = (uint32)(Random.next() * UINT_MAX);
		gameObject->angle = 90.f;
		gameObject->position.x = 0;
	}

	void onInput(const InputController &input) override
	{
		if (input.actionDown == ButtonState::Pressed)
		{
			if (gameObject->position.y <verticalLimit) {
				const float advanceSpeed = 200.0f;
				gameObject->position.y += advanceSpeed * Time.deltaTime;
				if (isServer)
					NetworkUpdate(gameObject);
			}
		}
		if (input.actionUp == ButtonState::Pressed)
		{
			if (gameObject->position.y > -verticalLimit) {
				const float advanceSpeed = 200.0f;
				gameObject->position.y -= advanceSpeed * Time.deltaTime;
				if (isServer)
					NetworkUpdate(gameObject);
			}
		}
		if (input.actionLeft == ButtonState::Pressed) {
			
			if (gameObject->position.x > -horizontalLimit) {
				const float advanceSpeed = 100.0f;
				gameObject->position.x -= advanceSpeed * Time.deltaTime;
			}			
			if (isServer)
				NetworkUpdate(gameObject);
		}
		if (input.actionRight == ButtonState::Pressed) {
			if (gameObject->position.x < horizontalLimit) {
				const float advanceSpeed = 100.0f;
				gameObject->position.x += advanceSpeed * Time.deltaTime;
			}

			if (isServer)
				NetworkUpdate(gameObject);
		}
		
		/*if (input.actionLeft == ButtonState::Press)
		{
			GameObject * laser = App->modNetServer->spawnBullet(gameObject);
			laser->tag = gameObject->tag;
		}*/
	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override
	{
		if (c2.type == ColliderType::Laser && c2.gameObject->tag != gameObject->tag)
		{
			NetworkDestroy(c2.gameObject); // Destroy the laser

			// NOTE(jesus): spaceship was collided by a laser
			// Be careful, if you do NetworkDestroy(gameObject) directly,
			// the client proxy will poing to an invalid gameObject...
			// instead, make the gameObject invisible or disconnect the client.
		}
	}

};

struct Laser : public Behaviour
{
	float secondsSinceCreation = 0.0f;

	void update() override
	{
		const float pixelsPerSecond = 1000.0f;
		gameObject->position += vec2FromDegrees(gameObject->angle) * pixelsPerSecond * Time.deltaTime;

		secondsSinceCreation += Time.deltaTime;

		if (isServer)
			NetworkUpdate(gameObject);

		const float lifetimeSeconds = 2.0f;
		if (secondsSinceCreation > lifetimeSeconds) NetworkDestroy(gameObject);
	}
};

struct Asteroid : public Behaviour
{
	float secondsSinceCreation = 0.0f;


	float speed = 100.0f;
	float rotSpeed = Random.randomFloat(-100.0f, 100.0f);

	void update() override 
	{
		secondsSinceCreation *= Time.deltaTime;

		if (isServer)
			NetworkUpdate(gameObject);

		gameObject->position.x -= speed * Time.deltaTime;

		//rotation ?
		gameObject->angle += rotSpeed * Time.deltaTime;


		const float lifetimeSeconds = 5.0f;
		if (secondsSinceCreation > lifetimeSeconds) NetworkDestroy(gameObject);
	}
};
	