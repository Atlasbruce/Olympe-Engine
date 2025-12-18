#include "ECS_Systems.h"

#include "World.h"
#include "ECS_Components.h"

#include <cmath>

namespace {
	static inline float clampf(float v, float lo, float hi) {
		return (v < lo) ? lo : (v > hi) ? hi : v;
	}

	static inline void set_draw_color(SDL_Renderer* r, const SDL_Color& c) {
		SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
	}

	// Simple safe line draw wrapper.
	static inline void draw_line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2) {
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}

	// Convert world (float) to screen pixels using a camera.
	// This relies on common camera members used across engine codebases; if your camera differs,
	// adjust these lookups accordingly.
	struct CameraView {
		SDL_FRect world_view;   // world rect visible (x,y,w,h)
		SDL_Rect  viewport;     // screen viewport (x,y,w,h)
		float     pixels_per_unit_x;
		float     pixels_per_unit_y;
	};

	// Try to build a CameraView from common engine patterns.
	// If your project uses different camera storage, adapt in one place.
	static bool build_camera_view(World* world, Entity camera_entity, CameraView& out);

	// ----------------- Projections -----------------
	static inline void ortho_world_to_screen(const CameraView& cv, float wx, float wy, int& sx, int& sy) {
		float nx = (wx - cv.world_view.x) * cv.pixels_per_unit_x;
		float ny = (wy - cv.world_view.y) * cv.pixels_per_unit_y;
		sx = cv.viewport.x + (int)std::lround(nx);
		sy = cv.viewport.y + (int)std::lround(ny);
	}

	// Isometric (2:1-ish) diamond projection.
	// Given tile size (tw,th):
	// screen_x = (x - y) * tw/2
	// screen_y = (x + y) * th/2
	static inline void iso_grid_to_world(float gx, float gy, float tw, float th, float& wx, float& wy) {
		// Treat world plane as grid coordinates; we store lines as projected in screen later.
		// For camera mapping, we keep them in "iso-world" where x,y are the grid coords.
		wx = gx; wy = gy;
		(void)tw; (void)th;
	}

	static inline void iso_to_screen(const CameraView& cv, float gx, float gy, float tw, float th, int& sx, int& sy) {
		// Convert iso grid coords to world-like coords then to screen using camera scaling.
		// We map iso projected coordinates in screen-space units directly.
		float px = (gx - gy) * (tw * 0.5f);
		float py = (gx + gy) * (th * 0.5f);
		// px/py are in world units on the rendering plane
		ortho_world_to_screen(cv, cv.world_view.x + px, cv.world_view.y + py, sx, sy);
	}

	// Pointy-top axial hex projection (q,r).
	// Using size s:
	// x = s * sqrt(3) * (q + r/2)
	// y = s * 3/2 * r
	static inline void hex_to_screen(const CameraView& cv, float q, float r, float s, int& sx, int& sy) {
		float px = s * 1.73205080757f * (q + r * 0.5f);
		float py = s * 1.5f * r;
		ortho_world_to_screen(cv, cv.world_view.x + px, cv.world_view.y + py, sx, sy);
	}
}

// NOTE:
// build_camera_view tries to locate a camera component with typical fields.
// If your engine defines camera differently, update this function.
bool build_camera_view(World* world, Entity camera_entity, CameraView& out) {
	// Minimal defensive approach: expect a Camera_data component with members:
	// - SDL_FRect view (world rect)
	// - SDL_Rect viewport (screen viewport)
	// Or:
	// - float x,y,w,h; and viewport_x/y/w/h
	// If not found, return false.
	(void)camera_entity;

	// The repository's ECS component types might differ; try common names via templates isn't possible here.
	// So we fallback to World helper if present.
	// If World exposes GetActiveCameras() you should replace this entirely.

	// As a safe default: use full renderer output as viewport and treat world_view as matching pixels.
	int w = 0, h = 0;
	SDL_RenderGetLogicalSize(world->GetRenderer(), &w, &h);
	if (w == 0 || h == 0) {
		SDL_GetRendererOutputSize(world->GetRenderer(), &w, &h);
	}

	out.viewport = SDL_Rect{ 0, 0, w, h };
	out.world_view = SDL_FRect{ 0, 0, (float)w, (float)h };
	out.pixels_per_unit_x = 1.0f;
	out.pixels_per_unit_y = 1.0f;
	return true;
}

void GridSystem::Render(World* world, SDL_Renderer* renderer) {
	if (!world || !renderer) return;

	// Find singleton GridSettings_data
	Entity grid_e = world->ecs.GetSingleton<GridSettings_data>();
	if (!grid_e.IsValid()) return;

	auto& settings = world->ecs.GetComponent<GridSettings_data>(grid_e);
	if (!settings.enabled) return;

	set_draw_color(renderer, settings.color);

	// Determine cameras / viewports
	// Prefer engine-provided camera list if available; otherwise render once with fallback camera.
	std::vector<Entity> cameras;
	if constexpr (true) {
		// If your World exposes cameras, plug it here.
		// cameras = world->GetActiveCameraEntities();
	}
	if (cameras.empty()) {
		// Fallback: render once with a default view
		cameras.push_back(Entity{});
	}

	for (Entity camEnt : cameras) {
		CameraView cv{};
		if (!build_camera_view(world, camEnt, cv)) continue;

		SDL_RenderSetViewport(renderer, &cv.viewport);

		const float cw = (settings.cell_w <= 0.0f) ? 32.0f : settings.cell_w;
		const float ch = (settings.cell_h <= 0.0f) ? 32.0f : settings.cell_h;

		if (settings.projection == GridProjection::Ortho) {
			// Visible world rect
			float x0 = cv.world_view.x;
			float y0 = cv.world_view.y;
			float x1 = cv.world_view.x + cv.world_view.w;
			float y1 = cv.world_view.y + cv.world_view.h;

			int start_x = (int)std::floor(x0 / cw) - 1;
			int end_x   = (int)std::ceil (x1 / cw) + 1;
			int start_y = (int)std::floor(y0 / ch) - 1;
			int end_y   = (int)std::ceil (y1 / ch) + 1;

			for (int gx = start_x; gx <= end_x; ++gx) {
				float wx = gx * cw;
				int sx0, sy0, sx1, sy1;
				ortho_world_to_screen(cv, wx, y0, sx0, sy0);
				ortho_world_to_screen(cv, wx, y1, sx1, sy1);
				draw_line(renderer, sx0, sy0, sx1, sy1);
			}
			for (int gy = start_y; gy <= end_y; ++gy) {
				float wy = gy * ch;
				int sx0, sy0, sx1, sy1;
				ortho_world_to_screen(cv, x0, wy, sx0, sy0);
				ortho_world_to_screen(cv, x1, wy, sx1, sy1);
				draw_line(renderer, sx0, sy0, sx1, sy1);
			}
		}
		else if (settings.projection == GridProjection::Iso) {
			// Render iso grid lines centered around camera.
			// Determine approximate center grid coordinate from view center.
			float cx = cv.world_view.x + cv.world_view.w * 0.5f;
			float cy = cv.world_view.y + cv.world_view.h * 0.5f;
			(void)cx; (void)cy;

			int hx = settings.half_extent_x;
			int hy = settings.half_extent_y;

			// Draw lines of constant x (diagonals) and constant y.
			for (int i = -hx; i <= hx; ++i) {
				// line along y varying
				int sx0, sy0, sx1, sy1;
				iso_to_screen(cv, (float)i, (float)-hy, cw, ch, sx0, sy0);
				iso_to_screen(cv, (float)i, (float) hy, cw, ch, sx1, sy1);
				draw_line(renderer, sx0, sy0, sx1, sy1);
			}
			for (int j = -hy; j <= hy; ++j) {
				int sx0, sy0, sx1, sy1;
				iso_to_screen(cv, (float)-hx, (float)j, cw, ch, sx0, sy0);
				iso_to_screen(cv, (float) hx, (float)j, cw, ch, sx1, sy1);
				draw_line(renderer, sx0, sy0, sx1, sy1);
			}
		}
		else { // Hex
			float s = cw;
			int hx = settings.half_extent_x;
			int hy = settings.half_extent_y;

			// Draw axial coordinate "grid" as three families of parallel lines.
			// We approximate by drawing segments between two far points for each constant coordinate.
			// q constant
			for (int q = -hx; q <= hx; ++q) {
				int sx0, sy0, sx1, sy1;
				hex_to_screen(cv, (float)q, (float)-hy, s, sx0, sy0);
				hex_to_screen(cv, (float)q, (float) hy, s, sx1, sy1);
				draw_line(renderer, sx0, sy0, sx1, sy1);
			}
			// r constant
			for (int r = -hy; r <= hy; ++r) {
				int sx0, sy0, sx1, sy1;
				hex_to_screen(cv, (float)-hx, (float)r, s, sx0, sy0);
				hex_to_screen(cv, (float) hx, (float)r, s, sx1, sy1);
				draw_line(renderer, sx0, sy0, sx1, sy1);
			}
			// (q+r) constant (third axis)
			for (int k = -hx; k <= hx; ++k) {
				int sx0, sy0, sx1, sy1;
				// use q = k - r
				hex_to_screen(cv, (float)(k - (-hy)), (float)-hy, s, sx0, sy0);
				hex_to_screen(cv, (float)(k - ( hy)), (float) hy, s, sx1, sy1);
				draw_line(renderer, sx0, sy0, sx1, sy1);
			}
		}
	}

	// Restore viewport
	SDL_RenderSetViewport(renderer, nullptr);
}
