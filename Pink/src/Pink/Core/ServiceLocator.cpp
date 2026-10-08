#include "Pink/Core/ServiceLocator.h"

namespace Pink {

std::unordered_map<std::type_index, Ref<void>> ServiceLocator::s_Services;

}
