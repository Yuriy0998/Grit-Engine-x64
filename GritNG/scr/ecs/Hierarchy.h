#pragma once
#include <entt/entt.hpp>
#include <vector>

namespace grit::ecs {
	struct Parent { entt::entity entity = entt::null; };
	struct Children { std::vector<entt::entity> list; };
}