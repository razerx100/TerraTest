#include <gtest/gtest.h>
#include <ObjectManager.hpp>
#include <SimpleWindow.hpp>
#include <Terra.hpp>
#include <GenericCheckFunctions.hpp>
#include <VertexManagerVertexShader.hpp>
#include <VertexManagerMeshShader.hpp>
#include <VkResourceViews.hpp>
#include <VkShader.hpp>
#include <VKPipelineObject.hpp>
#include <PipelineLayout.hpp>
#include <VkQueueFamilyManager.hpp>
#include <VKRenderPass.hpp>

namespace SpecificValues {
	constexpr std::uint64_t testDisplayWidth = 2560u;
	constexpr std::uint64_t testDisplayHeight = 1440u;
	constexpr std::uint32_t windowWidth = 1920u;
	constexpr std::uint32_t windowHeight = 1080u;
	constexpr std::uint32_t bufferCount = 2u;
	constexpr VkDeviceSize testBufferSize = 128u;
	constexpr const char* appName = "Terra";
	constexpr const wchar_t* shaderPath = L"resources/shaders/";
	constexpr bool meshShader = true;
}

class RendererVKTest : public ::testing::Test {
protected:
	static inline void TearDownTestSuite() {
		s_testResourceView.reset();
		s_objectManager.StartCleanUp();
	}

	static inline ObjectManager s_objectManager;
	static inline std::unique_ptr<VkResourceView> s_testResourceView;
	static inline VkQueueFamilyMananger s_queFamilyMan;

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

	s_queFamilyMan = Terra::device->GetQueueFamilyManager();
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

	Terra::InitGraphicsQueue(
		s_objectManager, s_queFamilyMan.GetQueue(GraphicsQueue), logicalDevice,
		s_queFamilyMan.GetIndex(GraphicsQueue), SpecificValues::bufferCount
	);
	CheckQueueObjects(
		"graphics", Terra::graphicsQueue, Terra::graphicsCmdBuffer, Terra::graphicsSyncObjects
	);
	CheckQueueVkObjects(
		"Graphics", Terra::graphicsCmdBuffer, Terra::graphicsSyncObjects,
		SpecificValues::bufferCount
	);

	Terra::InitTransferQueue(
		s_objectManager, s_queFamilyMan.GetQueue(TransferQueue), logicalDevice,
		s_queFamilyMan.GetIndex(TransferQueue)
	);
	CheckQueueObjects(
		"transfer", Terra::transferQueue, Terra::transferCmdBuffer, Terra::transferSyncObjects
	);
	CheckQueueVkObjects("transfer", Terra::transferCmdBuffer, Terra::transferSyncObjects, 1u);

	Terra::InitComputeQueue(
		s_objectManager, s_queFamilyMan.GetQueue(ComputeQueue), logicalDevice,
		s_queFamilyMan.GetIndex(ComputeQueue), SpecificValues::bufferCount
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
		.presentQueue = s_queFamilyMan.GetQueue(GraphicsQueue)
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

TEST_F(RendererVKTest, VkResourceViewInitTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	s_testResourceView = std::make_unique<VkResourceView>(logicalDevice);
	ObjectInitCheck("testResourceView", s_testResourceView);

	{
		VkBuffer buffer = s_testResourceView->GetResource();
		VkObjectNullCheck("VkBuffer", buffer);
	}

	s_testResourceView->CreateResource(
		logicalDevice, SpecificValues::testBufferSize, SpecificValues::bufferCount,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);
	s_testResourceView->SetMemoryOffsetAndType(logicalDevice, MemoryType::gpuOnly);

	// Sanity test
	VkPhysicalDevice device = Terra::device->GetPhysicalDevice();
	VkPhysicalDeviceProperties deviceProperty{};
	vkGetPhysicalDeviceProperties(device, &deviceProperty);

	VkDeviceSize expectedSubAllocationSize = Align(
		SpecificValues::testBufferSize, deviceProperty.limits.minStorageBufferOffsetAlignment
	);

	VkDeviceSize subAllocationSize = s_testResourceView->GetSubAllocationOffset(1u);
	EXPECT_EQ(expectedSubAllocationSize, subAllocationSize)
		<< "SubAllocationSize doesn't match.";

	VkDeviceSize subBufferSize = s_testResourceView->GetSubBufferSize();
	EXPECT_EQ(subBufferSize, SpecificValues::testBufferSize) << "SubBufferSize doesn't match.";

	VkDeviceSize expectedBufferSize =
		subAllocationSize * static_cast<VkDeviceSize>(SpecificValues::bufferCount - 1u)
		+ subBufferSize;
	VkDeviceSize bufferSize = s_testResourceView->GetBufferSize();
	EXPECT_EQ(bufferSize, expectedBufferSize) << "BufferSize doesn't match.";

	VkBuffer buffer = s_testResourceView->GetResource();
	VkObjectInitCheck("VkBuffer", buffer);
}

TEST_F(RendererVKTest, MemoryCreationTest) {
	VkDeviceMemory gpuMemory = Terra::Resources::gpuOnlyMemory->GetMemoryHandle();
	VkObjectNullCheck("GPUMemory", gpuMemory);

	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	Terra::Resources::gpuOnlyMemory->AllocateMemory(logicalDevice);

	gpuMemory = Terra::Resources::gpuOnlyMemory->GetMemoryHandle();
	VkObjectInitCheck("GPUMemory", gpuMemory);
}

TEST_F(RendererVKTest, ResourceViewMemoryAndDescriptorTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	s_testResourceView->BindResourceToMemory(logicalDevice);

	DescriptorInfo inputDescInfo{
		.bindingSlot = 0u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	auto inputBufferInfos = s_testResourceView->GetDescBufferInfoSplit(
		SpecificValues::bufferCount
	);

	Terra::graphicsDescriptorSet->AddBuffersSplit(
		inputDescInfo, std::move(inputBufferInfos), VK_SHADER_STAGE_ALL
	);
}

TEST_F(RendererVKTest, DescriptorCreationTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	Terra::graphicsDescriptorSet->CreateDescriptorSets(logicalDevice);
	DescriptorSetManager const* descManager = Terra::graphicsDescriptorSet.get();
	const char* name = "graphics";

	VkDescriptorSetLayout const* descLayout = descManager->GetDescriptorSetLayouts();
	size_t descriptorSetCount = descManager->GetDescriptorSetCount();
	EXPECT_EQ(descriptorSetCount, SpecificValues::bufferCount)
		<< "DescCount doesn't match.";

	for (size_t index = 0u; index < SpecificValues::bufferCount; ++index) {
		VkObjectInitCheck(
			FormatCompName(name, " VkDescriptorSetLayout ", index), descLayout[index]
		);

		VkDescriptorSet descSet = descManager->GetDescriptorSet(index);
		VkObjectInitCheck(FormatCompName(name, " VkDescriptorSet ", index), descSet);
	}
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
		logicalDevice, SpecificValues::bufferCount,
		s_queFamilyMan.GetTransferAndGraphicsIndices()
	};
	vertexManagerMS.AddGVerticesAndPrimIndices(
		logicalDevice, std::move(verticesCopy), std::move(indicesCopy), std::move(primIndices)
	);
}

TEST_F(RendererVKTest, VkPipelineLayoutTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	DescriptorSetManager const* descManager = Terra::graphicsDescriptorSet.get();

	PipelineLayout layout{ logicalDevice };
	layout.CreateLayout(
		descManager->GetDescriptorSetLayouts(), descManager->GetDescriptorSetCount()
	);

	VkPipelineLayout pipeLayout = layout.GetLayout();
	VkObjectInitCheck("VkPipelineLayout", pipeLayout);
}

TEST_F(RendererVKTest, VkShaderInitTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	VkShader vertexShader{ logicalDevice };
	vertexShader.CreateShader(
		logicalDevice, SpecificValues::shaderPath + std::wstring(L"VertexShaderTest.spv")
	);

	VkShaderModule shaderModule = vertexShader.GetShaderModule();
	VkObjectInitCheck("VkShaderModule", shaderModule);
}

TEST_F(RendererVKTest, VkComputePSOTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	DescriptorSetManager const* descManager = Terra::graphicsDescriptorSet.get();

	PipelineLayout layout{ logicalDevice };
	layout.CreateLayout(
		descManager->GetDescriptorSetLayouts(), descManager->GetDescriptorSetCount()
	);

	VkPipelineLayout pipeLayout = layout.GetLayout();

	VkShader computeShader{ logicalDevice };
	computeShader.CreateShader(
		logicalDevice, SpecificValues::shaderPath + std::wstring(L"ComputeShaderTest.spv")
	);

	VkShaderModule shaderModule = computeShader.GetShaderModule();

	VkPipelineObject computePSO{ logicalDevice };
	computePSO.CreateComputePipeline(logicalDevice, pipeLayout, shaderModule);

	VkPipeline computePipeline = computePSO.GetPipeline();
	VkObjectInitCheck("VkComputePipeline", computePipeline);
}

TEST_F(RendererVKTest, VkRenderPassInitTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	VKRenderPass renderPass{ logicalDevice };
	renderPass.CreateRenderPass(logicalDevice, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT);

	VkRenderPass vkRenderPass = renderPass.GetRenderPass();
	VkObjectInitCheck("VkRenderPass", vkRenderPass);
}

TEST_F(RendererVKTest, VkGraphicsVertexPSOTest) {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	DescriptorSetManager const* descManager = Terra::graphicsDescriptorSet.get();

	PipelineLayout layout{ logicalDevice };
	layout.CreateLayout(
		descManager->GetDescriptorSetLayouts(), descManager->GetDescriptorSetCount()
	);

	VkPipelineLayout pipeLayout = layout.GetLayout();

	VkShader vertexShader{ logicalDevice };
	vertexShader.CreateShader(
		logicalDevice, SpecificValues::shaderPath + std::wstring(L"VertexShaderTest.spv")
	);

	VkShaderModule vertexShaderModule = vertexShader.GetShaderModule();

	VkShader fragmentShader{ logicalDevice };
	fragmentShader.CreateShader(
		logicalDevice, SpecificValues::shaderPath + std::wstring(L"FragmentShaderTest.spv")
	);

	VkShaderModule fragmentShaderModule = fragmentShader.GetShaderModule();

	VKRenderPass renderPass{ logicalDevice };
	renderPass.CreateRenderPass(logicalDevice, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT);

	VkRenderPass vkRenderPass = renderPass.GetRenderPass();

	VkPipelineObject graphicsVertexPSO{ logicalDevice };
	graphicsVertexPSO.CreateGraphicsPipeline(
		logicalDevice, pipeLayout, vkRenderPass,
		VertexLayout()
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.InitLayout(), vertexShaderModule, fragmentShaderModule
	);

	VkPipeline graphicsVertexPipeline = graphicsVertexPSO.GetPipeline();
	VkObjectInitCheck("VkGraphicsVertexPipeline", graphicsVertexPipeline);
}
