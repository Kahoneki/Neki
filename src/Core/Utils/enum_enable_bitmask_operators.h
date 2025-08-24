#pragma once

#include <type_traits>

//Generic helper template to enable bitmask operators for any enum
//Default specialisation below holds ::value=false - this is the fallback if enable_bitmask_operators isn't explicitly defined for the enum in question
//To enable bitmask operators for enums, write an explicit specialisation of this struct for said enum and have it inherit from std::true_type{}
template<typename>
struct enable_bitmask_operators : std::false_type{};

//Bitwise OR overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E> operator|(E lhs, E rhs)
{
	return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) | static_cast<std::underlying_type_t<E>>(rhs));
}

//Bitwise AND overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E> operator&(E lhs, E rhs)
{
	return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) & static_cast<std::underlying_type_t<E>>(rhs));
}

//Bitwise XOR overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E> operator^(E lhs, E rhs)
{
	return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) ^ static_cast<std::underlying_type_t<E>>(rhs));
}

//Bitwise NOT overload
template<typename E>
std::enable_if_t<enable_bitmask_operators<E>::value, E> operator~(E e)
{
	return static_cast<E>(~static_cast<std::underlying_type_t<E>>(e));
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