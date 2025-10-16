#pragma once

#include <type_traits>
#include <utility>


//Generic helper template to enable bitmask operators for any enum
//Default specialisation below holds ::value=false - this is the fallback if enable_bitmask_operators isn't explicitly defined for the enum in question
//To enable bitmask operators for enums, write an explicit specialisation of this struct for said enum and have it inherit from std::true_type{}
template<typename>
struct enable_bitmask_operators : std::false_type{};

//Enable bitmask operators for the NK::NAME enum type - closes NK namespace scope, creates the truthy enable_bitmask_operators type, and reopens the NK namespace scope
#define ENABLE_BITMASK_OPERATORS(NAME) \
}; \
template<> \
struct enable_bitmask_operators<NK::NAME> : std::true_type {}; \
namespace NK {

//Bitwise OR overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E> operator|(E lhs, E rhs)
{
	return static_cast<E>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

//Bitwise AND overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E> operator&(E lhs, E rhs)
{
	return static_cast<E>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

//Bitwise XOR overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E> operator^(E lhs, E rhs)
{
	return static_cast<E>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

//Bitwise NOT overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E> operator~(E e)
{
	return static_cast<E>(~std::to_underlying(e));
}

//Bitwise OR assignment overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E&> operator|=(E& lhs, E rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

//Bitwise AND assignment overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E&> operator&=(E& lhs, E rhs)
{
	lhs = lhs & rhs;
	return lhs;
}