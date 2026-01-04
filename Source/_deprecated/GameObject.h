/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Class representing a game object
- handles visual representation (sprite, animation, fx)
- Physics properties

*/
#pragma once
#include "Object.h"
#include "World.h"
#include "ObjectComponent.h"
#include <vector>
#include <SDL3/SDL_rect.h>
#include "vector.h"


enum class EntityType
{
    None = 0,
    Player,
    NPC,
    Collectible,
    Trigger,
    Projectile,
    Count
};

class GameObject : public Object
{
public:
    static bool FactoryRegistered;
    static Object* Create();
    GameObject()
    {
        name = "unnamed GameObject";
	}
    virtual ~GameObject() override = default;
	virtual ObjectType GetObjectType() const { return ObjectType::Entity; }
	virtual EntityType GetEntityType() const { return EntityType::None; }

//---------------------------------------------------------------------------------------------
// GameObject Properties--------------------------

	inline void SetPosition(Vector& _p) 
    {
        position = _p;
        vRenderPosition = _p + Vector(width / 2.0f, height / 2.0f, 0.0f);
        boundingBox = { _p.x, _p.y, width, height };
    }
	inline Vector GetPosition() const { return position; }
	inline Vector GetRenderPosition() const { return vRenderPosition; }
    inline void SetSize(float w, float h) 
    { 
        width = w; 
        height = h; 
        boundingBox.w = width; 
        boundingBox.h = height; 
	}
	inline void GetSize(float& w, float& h) const { w = width; h = height; }
	inline SDL_FRect GetBoundingBox() const { return boundingBox; }
	inline void SetBoundingbox(const SDL_FRect& box) { boundingBox = box; }
	inline void SetDynamic(bool b) { isDynamic = b; }
	inline bool IsDynamic() const { return isDynamic; }
	//---------------------------------------------------------------------------------------------

    // Forward received messages to properties (default behavior)
    virtual void OnEvent(const Message& msg) override;

	void Render() { SYSTEM_LOG << " WARNING GameObject::Render call: the visual component should handling the GameObject rendering (sprite, animation etc..) invalid call for GameObject: " << name << " at (" << position.x << ", " << position.y << ")\n"; }

    // Serialization to/from JSON (simple implementation)
    std::string Save() const;
    bool Load(const std::string& json);

protected:
    // position
    Vector position, vRenderPosition;
    // Size
    float width = 100.0f;
    float height = 150.0f;
    // Bounding Box
    SDL_FRect boundingBox = { 0.0f, 0.0f, 0.0f, 0.0f };
    // static or dynamic
    bool isDynamic = false;
};

