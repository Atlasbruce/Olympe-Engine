/**
 * @file AnimationGraphFrameworkDocument.h
 * @brief Adapter wrapper implementing IGraphDocument for Animation graphs.
 * @author Olympe Engine - Phase 55 Framework Integration
 * @date 2026-03-15
 *
 * @details
 * AnimationGraphFrameworkDocument is an adapter that wraps AnimationGraphRenderer
 * and exposes it through the IGraphDocument interface. This enables uniform
 * handling of all graph types (VisualScript, BehaviorTree, EntityPrefab, AnimationGraph)
 * in the unified framework.
 *
 * Pattern: Adapter (structural design pattern)
 * - Adapter: AnimationGraphFrameworkDocument
 * - Adaptee: AnimationGraphRenderer
 * - Target Interface: IGraphDocument
 */

#pragma once

#include "IGraphDocument.h"
#include <string>
#include <memory>

namespace Olympe
{
	// Forward declarations
	class AnimationGraphRenderer;

	/**
	 * @class AnimationGraphFrameworkDocument
	 * @brief Adapter implementing IGraphDocument for Animation graph renderer.
	 *
	 * This class wraps an AnimationGraphRenderer instance and provides the
	 * IGraphDocument interface for unified graph document handling.
	 */
	class AnimationGraphFrameworkDocument : public IGraphDocument
	{
	public:
		explicit AnimationGraphFrameworkDocument(AnimationGraphRenderer* renderer);
		~AnimationGraphFrameworkDocument() override;

		// IGraphDocument interface
		bool Load(const std::string& filePath) override;
		bool Save(const std::string& filePath) override;
		std::string GetName() const override;
		DocumentType GetType() const override;
		std::string GetFilePath() const override;
		void SetFilePath(const std::string& path) override;
		bool IsDirty() const override;

		// Renderer access for framework
		// Both const and non-const versions required by IGraphDocument
		IGraphRenderer* GetRenderer() override;
		const IGraphRenderer* GetRenderer() const override;

		// Document modification notification
		void OnDocumentModified() override { /* Delegated to renderer */ }

		// Extended accessors for animation-specific operations
		void SetRenderer(AnimationGraphRenderer* renderer) { m_renderer = renderer; }

	private:
		AnimationGraphRenderer* m_renderer;
	};

} // namespace Olympe
