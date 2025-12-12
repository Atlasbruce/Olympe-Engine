#include "Sprite.h"
#include "ObjectFactory.h"
#include "GameEngine.h"
#include "GameObject.h"
#include <SDL3/SDL_render.h>
#include "DataManager.h"

bool _Sprite::FactoryRegistered = ObjectFactory::Get().Register("_Sprite", _Sprite::Create);
ObjectComponent* _Sprite::Create()
{
	return new _Sprite();
}
//-----------------------------------------------------
void _Sprite::Initialize()
{

}

void _Sprite::Uninitialize()
{
}

void _Sprite::RenderDebug()
{
	/*
	SDL_SetRenderDrawColor(GameEngine::renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderRect(GameEngine::renderer, &gao->boundingBox);
	/**/
}

void _Sprite::Render()
{
	Vector vRenderPos = gao->GetPosition() - CameraManager::Get().GetCameraPositionForActivePlayer();
	float _w, _h;
	gao->GetSize(_w, _h);
	gao->SetBoundingbox( {vRenderPos.x, vRenderPos.y, _w, _h} );

	if (m_SpriteTexture)
	{
		SDL_FRect box = gao->GetBoundingBox();

		SDL_RenderTexture(GameEngine::renderer, m_SpriteTexture, nullptr, &box);
	}
}

void _Sprite::SetSprite(SDL_Texture* texture)
{
	m_SpriteTexture = texture;
	gao->SetSize((float)m_SpriteTexture->w, (float)m_SpriteTexture->h);
	//gao->width = gao->boundingBox.h = (float)m_SpriteTexture->h;
	//gao->height = gao->boundingBox.w = (float)m_SpriteTexture->w;
}

void _Sprite::SetSprite(const std::string& resourceName,const std::string& filePath)
{
	SetSprite((SDL_Texture*)DataManager::Get().GetSprite(resourceName, filePath));
}

bool _Sprite::Preload(const std::string& resourceName, const std::string& filePath)
{
	return DataManager::Get().PreloadSprite(resourceName, filePath, ResourceCategory::GameObject);
}
