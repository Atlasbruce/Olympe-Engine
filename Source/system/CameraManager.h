#pragma once
// DEPRECATED: Use CameraSystem (ECS) instead
// This file is kept for legacy compatibility during migration phase
// All functionality has been moved to CameraSystem

class CameraManager
{
public:
    static CameraManager& GetInstance() { static CameraManager instance; return instance; }
    static CameraManager& Get() { return GetInstance(); }
    
    // All methods are deprecated and now no-ops
    void Initialize() {} // Use CameraSystem event registration
    void Shutdown() {}
    void Process() {} // Now handled by CameraSystem::Process()
};