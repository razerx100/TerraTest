#include <gtest/gtest.h>
#include <ObjectManager.hpp>
#include <Terra.hpp>

class RendererVKTest : public ::testing::Test {
protected:
	static ObjectManager m_objectManager;
};

ObjectManager RendererVKTest::m_objectManager;

TEST_F(RendererVKTest, DisplayInitTest) {
	Terra::InitDisplay(m_objectManager);

	EXPECT_NE(Terra::display, nullptr) << "Failed to initialise display.";
}

TEST_F(RendererVKTest, VkInstanceInitTest) {
	m_objectManager.CreateObject(Terra::vkInstance, { "Terra"}, 5u);
	EXPECT_NE(Terra::vkInstance, nullptr) << "Failed to initialise vkInstanceManager.";

	Terra::vkInstance->AddExtensionNames(Terra::display->GetRequiredExtensions());
	Terra::vkInstance->CreateInstance();

	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();
	EXPECT_NE(vkInstance, VK_NULL_HANDLE) << "Failed to initialise vkInstance.";
}

TEST_F(RendererVKTest, DebugLayerInitTest) {
	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();

#ifdef _DEBUG
	m_objectManager.CreateObject(Terra::debugLayer, { vkInstance }, 4u);
	EXPECT_NE(Terra::debugLayer, nullptr) << "Failed to initialise debugLayer.";
#endif
}

// Need to create a window for the rest
