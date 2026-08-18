/**
 * @file AnimationGraphFrameworkDocument.cpp
 * @brief Implementation of Animation Graph document adapter.
 * @author Olympe Engine - Phase 55 Framework Integration
 */

#include "AnimationGraphFrameworkDocument.h"
#include "../AnimationGraphRenderer.h"
#include "../../system/system_utils.h"

namespace Olympe
{
	AnimationGraphFrameworkDocument::AnimationGraphFrameworkDocument(AnimationGraphRenderer* renderer)
		: m_renderer(renderer)
	{
	}

	AnimationGraphFrameworkDocument::~AnimationGraphFrameworkDocument()
	{
	}

	bool AnimationGraphFrameworkDocument::Load(const std::string& filePath)
	{
		if (!m_renderer) return false;
		return m_renderer->Load(filePath);
	}

	bool AnimationGraphFrameworkDocument::Save(const std::string& filePath)
	{
		if (!m_renderer) return false;
		return m_renderer->Save(filePath);
	}

	std::string AnimationGraphFrameworkDocument::GetName() const
	{
		if (!m_renderer) return "";

		std::string path = m_renderer->GetCurrentPath();
		if (path.empty()) return "Untitled";

		// Extract filename from path
		size_t lastSlash = path.find_last_of("/\\");
		if (lastSlash != std::string::npos)
			return path.substr(lastSlash + 1);
		return path;
	}

	DocumentType AnimationGraphFrameworkDocument::GetType() const
	{
		return DocumentType::UNKNOWN;  // Animation Graph not yet in enum
	}

	std::string AnimationGraphFrameworkDocument::GetFilePath() const
	{
		if (!m_renderer) return "";
		return m_renderer->GetCurrentPath();
	}

	void AnimationGraphFrameworkDocument::SetFilePath(const std::string& path)
	{
		// Animation graphs store path internally after Load/Save
		// This is a no-op in the current implementation
	}

	bool AnimationGraphFrameworkDocument::IsDirty() const
	{
		if (!m_renderer) return false;
		return m_renderer->IsDirty();
	}

	// Non-const version delegates to const version
	IGraphRenderer* AnimationGraphFrameworkDocument::GetRenderer()
	{
		return const_cast<IGraphRenderer*>(
			static_cast<const AnimationGraphFrameworkDocument*>(this)->GetRenderer()
		);
	}

	// CRITICAL FIX (Phase 55): Cast implemented in .cpp to avoid circular dependency
	// AnimationGraphRenderer is an IGraphRenderer, so static_cast is valid here
	// (both pointers are compatible - AnimationGraphRenderer implements IGraphRenderer)
	const IGraphRenderer* AnimationGraphFrameworkDocument::GetRenderer() const
	{
		return static_cast<const IGraphRenderer*>(m_renderer);
	}

} // namespace Olympe
