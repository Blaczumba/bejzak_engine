#pragma once

#include <array>
#include <vector>

template<typename Type, size_t N>
class SparseSet {
private:
	std::array<Type, N> _sparse;
};
