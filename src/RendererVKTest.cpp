#include <gtest/gtest.h>
#include <ObjectManager.hpp>
#include <SimpleWindow.hpp>
#include <Terra.hpp>

namespace DeviceSpecificValues {
	constexpr std::uint64_t testDisplayWidth = 2560u;
	constexpr std::uint64_t testDisplayHeight = 1440u;
	constexpr std::uint32_t windowWidth = 1920u;
	constexpr std::uint32_t windowHeight = 1080u;
	constexpr const char* appName = "Terra";
}

class RendererVKTest : public ::testing::Test {
protected:
	static inline void TearDownTestSuite() {
		s_objectManager.StartCleanUp();
	}

	static inline ObjectManager s_objectManager;

#ifdef TERRA_WIN32
	static inline SimpleWindow s_window{
		DeviceSpecificValues::windowWidth, DeviceSpecificValues::windowHeight,
		DeviceSpecificValues::appName
	};
#endif
};

TEST_F(RendererVKTest, DisplayInitTest) {
	Terra::InitDisplay(s_objectManager);

	EXPECT_NE(Terra::display, nullptr) << "Failed to initialise display.";
}

TEST_F(RendererVKTest, VkInstanceInitTest) {
	s_objectManager.CreateObject(Terra::vkInstance, { DeviceSpecificValues::appName }, 5u);
	EXPECT_NE(Terra::vkInstance, nullptr) << "Failed to initialise vkInstanceManager.";

	Terra::vkInstance->AddExtensionNames(Terra::display->GetRequiredExtensions());
	Terra::vkInstance->CreateInstance();

	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();
	EXPECT_NE(vkInstance, VK_NULL_HANDLE) << "Failed to initialise the vkInstance.";
}

TEST_F(RendererVKTest, DebugLayerInitTest) {
#ifdef _DEBUG
	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();
	s_objectManager.CreateObject(Terra::debugLayer, { vkInstance }, 4u);
	EXPECT_NE(Terra::debugLayer, nullptr) << "Failed to initialise debugLayer.";
#endif
}

TEST_F(RendererVKTest, SurfaceWin32InitTest) {
	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();

#ifdef TERRA_WIN32
	void* windowHandle = s_window.GetWindowHandle();
	void* moduleHandle = s_window.GetModuleInstance();

	Terra::InitSurface(s_objectManager, vkInstance, windowHandle, moduleHandle);
	EXPECT_NE(Terra::surface, nullptr) << "Failed to initialise surfaceManager.";

	VkSurfaceKHR vkSurface = Terra::surface->GetSurface();
	EXPECT_NE(vkSurface, VK_NULL_HANDLE) << "Failed to initialise VkSurfaceKHR.";
#endif
}

TEST_F(RendererVKTest, DeviceInitTest) {
	s_objectManager.CreateObject(Terra::device, 3u);
	EXPECT_NE(Terra::device, nullptr) << "Failed to initialise DeviceManager.";

	VkSurfaceKHR vkSurface = Terra::surface->GetSurface();
	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();

	Terra::device->FindPhysicalDevice(vkInstance, vkSurface);
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();
	EXPECT_NE(physicalDevice, VK_NULL_HANDLE) << "Failed to find a suitable Physical Device.";

	Terra::device->CreateLogicalDevice();
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	EXPECT_NE(logicalDevice, VK_NULL_HANDLE) << "Failed to initialise the logicalDevice.";
}

TEST_F(RendererVKTest, DisplayGetResolutionTest) {
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	auto [width, height] = Terra::display->GetDisplayResolution(physicalDevice, 0u);

	EXPECT_EQ(width, DeviceSpecificValues::testDisplayWidth) << "Display width doesn't match.";
	EXPECT_EQ(height, DeviceSpecificValues::testDisplayHeight)
		<< "Display height doesn't match.";
}
