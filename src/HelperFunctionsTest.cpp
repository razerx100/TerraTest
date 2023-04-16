#include <vector>
#include <VkHelperFunctions.hpp>
#include <gtest/gtest.h>

TEST(HelperFunctionTest, ResolveQueueTest) {
	std::vector<std::uint32_t> expectedResults{ 0u, 1u };
	auto result = ResolveQueueIndices(1u, 0u, 1u);

	EXPECT_EQ(std::size(expectedResults), std::size(result))
		<< "The result and the expected results have different length";

	for (size_t index = 0u; index < std::size(expectedResults); ++index)
		EXPECT_EQ(expectedResults[index], result[index])
		<< "The queue index doesn't match with the expected index";
}