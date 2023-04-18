#ifndef TERRA_TEST_CHECK_FUNCTIONS_HPP_
#define TERRA_TEST_CHECK_FUNCTIONS_HPP_
#include <vulkan/vulkan.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <string>

template<typename T>
void ObjectInitCheck(const std::string& name, const std::unique_ptr<T>& ptr) noexcept {
	EXPECT_NE(ptr, nullptr) << "Failed to initialise the object " << name << ".";
}

template<typename T>
void VkObjectInitCheck(const std::string& name, T vkObject) noexcept {
	EXPECT_NE(vkObject, VK_NULL_HANDLE) << "Failed to initialise the vkObject " << name << ".";
}

template<typename T>
void ObjectNullCheck(const std::string& name, const std::unique_ptr<T>& ptr) noexcept {
	EXPECT_EQ(ptr, nullptr) << "The object " << name << " isn't null.";
}

template<typename T>
void VkObjectNullCheck(const std::string& name, T vkObject) noexcept {
	EXPECT_EQ(vkObject, VK_NULL_HANDLE) << "The object " << name << " isn't null.";
}
#endif
