#include <VkHelperFunctions.hpp>
#include <iostream>

int main() {
	auto result = ResolveQueueIndices(0u, 1u);

	for (auto& var : result)
		std::cout << var << " ";

	return 0;
}