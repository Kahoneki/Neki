#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/atomic.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/bitset.hpp>
#include <cereal/types/chrono.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/deque.hpp>
#include <cereal/types/forward_list.hpp>
#include <cereal/types/functional.hpp>
#include <cereal/types/list.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/queue.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/stack.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/unordered_set.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/valarray.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>


#define SERIALISE(TYPE, ...) \
	template<class Archive> \
	inline void serialize(Archive& a, TYPE& v) \
	{ \
		a(__VA_ARGS__); \
	}


#define SERIALISE_MEMBER_FUNC(...) \
	template<class Archive> \
	inline void serialize(Archive& a) \
	{ \
		a(__VA_ARGS__); \
	}


namespace glm
{

	SERIALISE(vec2, v.x, v.y)
	SERIALISE(vec3, v.x, v.y, v.z)
	SERIALISE(vec4, v.x, v.y, v.z, v.w)
	SERIALISE(ivec2, v.x, v.y)
	SERIALISE(ivec3, v.x, v.y, v.z)
	SERIALISE(ivec4, v.x, v.y, v.z, v.w)
	SERIALISE(mat2, v[0][0], v[0][1], v[1][0], v[1][1])
	SERIALISE(mat3, v[0][0], v[0][1], v[0][2], v[1][0], v[1][1], v[1][2], v[2][0], v[2][1], v[2][2])
	SERIALISE(mat4, v[0][0], v[0][1], v[0][2], v[0][3], v[1][0], v[1][1], v[1][2], v[1][3], v[2][0], v[2][1], v[2][2], v[2][3], v[3][0], v[3][1], v[3][2], v[3][3])
	SERIALISE(quat, v.x, v.y, v.z, v.w)
}