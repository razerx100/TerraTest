#include <gtest/gtest.h>
#include <ObjectManager.hpp>
#include <SimpleWindow.hpp>
#include <Terra.hpp>
#include <GenericCheckFunctions.hpp>
#include <VertexManagerVertexShader.hpp>
#include <VertexManagerMeshShader.hpp>

namespace SpecificValues {
	constexpr std::uint64_t testDisplayWidth = 2560u;
	constexpr std::uint64_t testDisplayHeight = 1440u;
	constexpr std::uint32_t windowWidth = 1920u;
	constexpr std::uint32_t windowHeight = 1080u;
	constexpr std::uint32_t bufferCount = 2u;
	constexpr const char* appName = "Terra";
	constexpr bool meshShader = true;
}

class RendererVKTest : public ::testing::Test {
protected:
	static inline void TearDownTestSuite() {
		s_objectManager.StartCleanUp();
	}

	static inline ObjectManager s_objectManager;

#ifdef TERRA_WIN32
	static inline SimpleWindow s_window{
		SpecificValues::windowWidth, SpecificValues::windowHeight,
		SpecificValues::appName
	};
#endif
};

TEST_F(RendererVKTest, DisplayInitTest) {
	Terra::InitDisplay(s_objectManager);

	ObjectInitCheck("display", Terra::display);
}

TEST_F(RendererVKTest, VkInstanceInitTest) {
	s_objectManager.CreateObject(Terra::vkInstance, { SpecificValues::appName }, 5u);
	ObjectInitCheck("vkInstance", Terra::vkInstance);

	Terra::vkInstance->AddExtensionNames(Terra::display->GetRequiredExtensions());
	Terra::vkInstance->CreateInstance();

	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();
	VkObjectInitCheck("VkInstance", vkInstance);
}

TEST_F(RendererVKTest, DebugLayerInitTest) {
#ifdef _DEBUG
	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();
	s_objectManager.CreateObject(Terra::debugLayer, { vkInstance }, 4u);
	ObjectInitCheck("debugLayer", Terra::debugLayer);
#endif
}

TEST_F(RendererVKTest, SurfaceWin32InitTest) {
	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();

#ifdef TERRA_WIN32
	void* windowHandle = s_window.GetWindowHandle();
	void* moduleHandle = s_window.GetModuleInstance();

	Terra::InitSurface(s_objectManager, vkInstance, windowHandle, moduleHandle);
	ObjectInitCheck("surface", Terra::surface);

	VkSurfaceKHR vkSurface = Terra::surface->GetSurface();
	VkObjectInitCheck("VkSurfaceKHR", vkSurface);
#endif
}

TEST_F(RendererVKTest, DeviceInitTest) {
	s_objectManager.CreateObject(Terra::device, 3u);
	ObjectInitCheck("device", Terra::device);

	VkSurfaceKHR vkSurface = Terra::surface->GetSurface();
	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();

	if (SpecificValues::meshShader)
		Terra::device->AddExtensionName("VK_EXT_mesh_shader");

	Terra::device->FindPhysicalDevice(vkInstance, vkSurface);
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();
	VkObjectInitCheck("VkPhysicalDevice", physicalDevice);

	Terra::device->CreateLogicalDevice(SpecificValues::meshShader);
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	VkObjectInitCheck("VkDevice", logicalDevice);
}

TEST_F(RendererVKTest, DisplayGetResolutionTest) {
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	auto [width, height] = Terra::display->GetDisplayResolution(physicalDevice, 0u);

	EXPECT_EQ(width, SpecificValues::testDisplayWidth) << "Display width doesn't match.";
	EXPECT_EQ(height, SpecificValues::testDisplayHeight)
		<< "Display height doesn't match.";
}

TEST_F(RendererVKTest, ResourcesInitTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	_vkResourceView::SetBufferAlignments(physicalDevice);

	Terra::InitResources(s_objectManager, physicalDevice, logicalDevice);
	ObjectInitCheck("gpuOnlyMemory", Terra::Resources::gpuOnlyMemory);
	ObjectInitCheck("cpuWriteMemory", Terra::Resources::cpuWriteMemory);
	ObjectInitCheck("uploadMemory", Terra::Resources::uploadMemory);
	ObjectInitCheck("uploadContainer", Terra::Resources::uploadContainer);
}

static std::string FormatCompName(
	const char* name, const char* objectName, size_t index
) noexcept {
	return std::string(name) + objectName + std::to_string(index);
}

static void CheckQueueVkObjects(
	const char* queueName, const std::unique_ptr<VKCommandBuffer>& cmdBuffer,
	std::unique_ptr<VkSyncObjects>& syncObjects, size_t count
) {
	for (size_t index = 0u; index < count; ++index) {
		VkCommandBuffer vkCmdBuffer = cmdBuffer->GetCommandBuffer(index);
		VkObjectInitCheck(FormatCompName(queueName, " CmdBuffer ", index), vkCmdBuffer);

		VkFence vkFence = syncObjects->GetFrontFence();
		VkObjectInitCheck(FormatCompName(queueName, " Fence ", index), vkFence);

		VkSemaphore vkSemaphore = syncObjects->GetFrontSemaphore();
		VkObjectInitCheck(FormatCompName(queueName, " Semaphore ", index), vkSemaphore);

		syncObjects->AdvanceSyncObjectsInQueue();
	}
}

static void CheckQueueObjects(
	const char* queueName, const std::unique_ptr<VkCommandQueue>& cmdQueue,
	const std::unique_ptr<VKCommandBuffer>& cmdBuffer,
	const std::unique_ptr<VkSyncObjects>& syncObjects
) {
	std::string queueNameStr = queueName;

	ObjectInitCheck(queueNameStr + "Queue", cmdQueue);
	ObjectInitCheck(queueNameStr + "CmdBuffer", cmdBuffer);
	ObjectInitCheck(queueNameStr + "SyncObjects", syncObjects);
}

TEST_F(RendererVKTest, QueuesInitTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::GraphicsQueue
	);
	Terra::InitGraphicsQueue(
		s_objectManager, graphicsQueueHandle, logicalDevice, graphicsQueueFamilyIndex,
		SpecificValues::bufferCount
	);
	CheckQueueObjects(
		"graphics", Terra::graphicsQueue, Terra::graphicsCmdBuffer, Terra::graphicsSyncObjects
	);
	CheckQueueVkObjects(
		"Graphics", Terra::graphicsCmdBuffer, Terra::graphicsSyncObjects,
		SpecificValues::bufferCount
	);

	auto [transferQueueHandle, transferQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::TransferQueue
	);
	Terra::InitTransferQueue(
		s_objectManager, transferQueueHandle, logicalDevice, transferQueueFamilyIndex
	);
	CheckQueueObjects(
		"transfer", Terra::transferQueue, Terra::transferCmdBuffer, Terra::transferSyncObjects
	);
	CheckQueueVkObjects("transfer", Terra::transferCmdBuffer, Terra::transferSyncObjects, 1u);

	auto [computeQueueHandle, computeQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::ComputeQueue
	);
	Terra::InitComputeQueue(
		s_objectManager, computeQueueHandle, logicalDevice, computeQueueFamilyIndex,
		SpecificValues::bufferCount
	);
	CheckQueueObjects(
		"compute", Terra::computeQueue, Terra::computeCmdBuffer, Terra::computeSyncObjects
	);
	CheckQueueVkObjects(
		"compute", Terra::computeCmdBuffer, Terra::computeSyncObjects,
		SpecificValues::bufferCount
	);
}

TEST_F(RendererVKTest, SwapchainInitTest) {
	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::GraphicsQueue
	);
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();
	VkSurfaceKHR vkSurface = Terra::surface->GetSurface();

	SwapChainManager::Args swapArguments{
		.device = logicalDevice,
		.surface = vkSurface,
		.surfaceInfo = QuerySurfaceCapabilities(physicalDevice, vkSurface),
		.width = SpecificValues::windowWidth,
		.height = SpecificValues::windowHeight,
		.bufferCount = SpecificValues::bufferCount,
		// Graphics and Present queues should be the same
		.presentQueue = graphicsQueueHandle
	};

	s_objectManager.CreateObject(Terra::swapChain, swapArguments, 1u);
	ObjectInitCheck("swapChain", Terra::swapChain);

	VkSwapchainKHR swapchain = Terra::swapChain->GetRef();
	VkObjectInitCheck("VkSwapchainKHR", swapchain);

	for (size_t index = 0u; index < SpecificValues::bufferCount; ++index) {
		VkFramebuffer frameBuffer = Terra::swapChain->GetFramebuffer(index);
		VkObjectNullCheck(std::string("VkFrameBuffer") + std::to_string(index), frameBuffer);
	}
}

static void DescriptorObjectsCheck(
	const char* name, std::unique_ptr<DescriptorSetManager>& descManager
) {
	VkDescriptorSetLayout const* descLayout = descManager->GetDescriptorSetLayouts();

	for (size_t index = 0u; index < SpecificValues::bufferCount; ++index) {
		VkObjectNullCheck(
			FormatCompName(name, " VkDescriptorSetLayout ", index), descLayout[index]
		);

		VkDescriptorSet descSet = descManager->GetDescriptorSet(index);
		VkObjectNullCheck(FormatCompName(name, " VkDescriptorSet ", index), descSet);
	}
}

TEST_F(RendererVKTest, DescriptorsInitTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	Terra::InitDescriptorSets(s_objectManager, logicalDevice, SpecificValues::bufferCount);
	ObjectInitCheck("graphicsDescriptorSet", Terra::graphicsDescriptorSet);
	ObjectInitCheck("computeDescriptorSet", Terra::computeDescriptorSet);

	DescriptorObjectsCheck("graphicsDescriptorSet", Terra::graphicsDescriptorSet);
	DescriptorObjectsCheck("computeDescriptorSet", Terra::computeDescriptorSet);
}
TEST_F(RendererVKTest, VertexManagerInitTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	std::vector<Vertex> verticesTest;
	verticesTest.emplace_back();
	verticesTest.emplace_back();
	verticesTest.emplace_back();

	std::vector<std::uint32_t> indicesTest;
	indicesTest.emplace_back();
	indicesTest.emplace_back();
	indicesTest.emplace_back();

	std::vector<Vertex> verticesCopy = verticesTest;
	std::vector<std::uint32_t> indicesCopy = indicesTest;

	VertexManagerVertexShader vertexManagerVS{ logicalDevice };
	vertexManagerVS.AddGVerticesAndIndices(
		logicalDevice, std::move(verticesTest), std::move(indicesTest)
	);

	std::vector<std::uint32_t> primIndices = indicesCopy;
	VertexManagerMeshShader vertexManagerMS{
		logicalDevice, SpecificValues::bufferCount, {0, 1}
	};
	vertexManagerMS.AddGVerticesAndPrimIndices(
		logicalDevice, std::move(verticesCopy), std::move(indicesCopy), std::move(primIndices)
	);
}
