#include <algorithm>

namespace lib {

template <typename Container, typename UnaryPredicate>
bool cont_all_of(const Container& container, UnaryPredicate pred) {
    return std::all_of(container.begin(), container.end(), pred);
}

}