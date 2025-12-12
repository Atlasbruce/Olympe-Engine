#pragma once
#include "ObjectComponent.h"
#include <SDL3/SDL.h>

class _Sprite : public VisualComponent
{
	public:
		explicit _Sprite() { Initialize(); }
		virtual ~_Sprite() override { Uninitialize(); }

	static bool FactoryRegistered;
	static ObjectComponent* Create();

	void Initialize();
	void Uninitialize();

	virtual void RenderDebug() override;
	virtual void Render() override;

	void SetSprite(SDL_Texture* texture);
	void SetSprite(const std::string& resourceName, const std::string& filePath);

	bool Preload(const std::string& resourceName, const std::string& filePath);

protected:
	SDL_Texture* m_SpriteTexture = nullptr;
};

