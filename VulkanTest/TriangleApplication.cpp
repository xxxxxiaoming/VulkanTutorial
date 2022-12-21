#include "TriangleApplication.h"

void TriangleApplication::run()
{
	initWindow();
	initVkn();

	mainLoop();
	cleanUp();
}

VkBool32 TriangleApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void TriangleApplication::mainLoop()
{
	if (window == nullptr)
	{
		return;
	}

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(logicalDevice);
}

void TriangleApplication::drawFrame()
{
	vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, imageAvaliableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvaliableSemaphores[currentFrame] };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer.");
	}

	VkPresentInfoKHR presentInfo{};
	VkSwapchainKHR swapchains[] = { swapchain };
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(presentationQueue, &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void TriangleApplication::initVkn()
{
	createInstance();
	setupDebugMessenger();
	// The window surface needs to be create right after the instance, because it can influence the pyhsical device selection.
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFrameBuffers();
	createCommanPool();
	allocateCommandBuffers();
	createSyncObjects();
}

void TriangleApplication::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void TriangleApplication::createInstance()
{
	if (enableLayerValidation && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Validation layer required, but not supported.");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Triangle_App_Vulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	/*uint32_t extensiontsCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensiontsCount);*/
	std::vector<const char*> extensions = getRequiredExtentions();
	
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableLayerValidation)
	{
		generateDebugMessengerCreateInfoEXT(debugCreateInfo);

		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledExtensionNames = nullptr;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vulkan instance.");
	}
}

void TriangleApplication::createSurface()
{
	/*
	* Although the VKSurfaceKHR object and its usage is platform agonistic, its creation isn't becatuse it depends on window system details.
	* GLFW actually has glfwCreateWindowSurface that handles the platform differences for us.
	* If you wanna know more about what it does behine the scenes, you can read this:
	* https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface
	*/
	if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

void TriangleApplication::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.queueFamilyIndex = indices.graphicFamliy.value();
	graphicsQueueCreateInfo.queueCount = 1;


	VkDeviceQueueCreateInfo presentationQueueCreateInfo{};
	presentationQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	presentationQueueCreateInfo.queueFamilyIndex = indices.presentationFamily.value();
	presentationQueueCreateInfo.queueCount = 1;

	/*
	* Vulkan lets you assign priorities to queues to influence the sceduling of
	* command buffer excution using floatging point numbers between [0, 1].
	*/
	float queuePriority = 1.0f;
	graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
	presentationQueueCreateInfo.pQueuePriorities = &queuePriority;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.push_back(graphicsQueueCreateInfo);
	queueCreateInfos.push_back(presentationQueueCreateInfo);

	VkPhysicalDeviceFeatures deviceFeatures{};
	VkDeviceCreateInfo createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExensions.size());
	createInfo.ppEnabledExtensionNames = deviceExensions.data();

	if (enableLayerValidation)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a logical device.");
	}

	vkGetDeviceQueue(logicalDevice, indices.graphicFamliy.value(), 0, &graphicQueue);
	vkGetDeviceQueue(logicalDevice, indices.presentationFamily.value(), 0, &presentationQueue);
}

void TriangleApplication::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapchainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentMode);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	/*
	* It is recommended to request at least one more image than the minimum.
	* Beacause we may sometimes have to wait on the driver to complete internal operations before we can acquire another image to render to
	* when simply sticking to this minimum.
	*/
	uint32_t imgCount = swapChainSupport.capabilities.minImageCount + 1;
	imgCount = std::clamp(imgCount, swapChainSupport.capabilities.minImageCount, swapChainSupport.capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imgCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;								// specifies the amount of layers each image consist of, always 1.			
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	// specifies what kind of opeartions we'll use the image in wht swap chain.

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicFamliy.value(), indices.presentationFamily.value() };
	
	if (indices.graphicFamliy.value() != indices.presentationFamily.value())
	{
		/*
		* VK_SHARING_MODE_CONCURRENT: Imgaes can be used across multiple queue families without explicit ownership transfers.
		*/
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		/*
		* VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family at a time.
		* Owership must be explicitly transferred before using it in another family.
		* This option offers the best performance!!!
		*/
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // We can specify that a certain transform(like a 90 degree clockwise rotation)
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// specifies if th alpha channel should be used for blending with other windows in the window system. For now we just ignore the alpha channel.
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain.");
	}

	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imgCount, nullptr);
	swapChainImages.resize(imgCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imgCount, swapChainImages.data());

	swapchainFormat = surfaceFormat.format;
	swapchainExtent = extent;
}

void TriangleApplication::createImageViews()
{
	swapchainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainFormat;
		
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image view.");
		}
	}
}

void TriangleApplication::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 1 sample, no anti-aliaing

	/*
	* loadOp and storeOp determin what to do with the data in the attachment before rendering and after rendering
	* VK_ATTACHMENT_LOAD_OP_LOAD : Preserve the existing contents
	* VK_ATTACHMENT_LOAP_OP_CLEAR : Clear the values to constant at the start.
	* VK_ATTACHMENT_LOAD_OP_DONT_CARE : Existing contents are undefined; we don't care about them.
	* VK_ATTACHMENT_STORE_OP_STORE : Render contents will be stored in memeory and can be read later.
	* VK_ATTACHMENT_STORE_OP_DONT_CARE : Contents of the framebuffer will be undefined after the rendering operation
	*/
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // we want to see the rendered triangle, so we choose the store operation here.
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass.");
	}
}

void TriangleApplication::createGraphicsPipeline()
{
	// read the shader bytecode
	auto vertexShader = readFile("vert.spv");
	auto fragmentShader = readFile("frag.spv");

	// create shader module
	VkShaderModule vertexShaderModule = createShaderModule(vertexShader);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShader);

	// create vertex shader stage
	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.module = vertexShaderModule;
	vertexShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = fragmentShaderModule;
	fragmentShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	/*
	* pVertexBindingDescriptions : spacing between data and whether the data is per-vertex or per-instance.
	* pVertexAttributeDescriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
	* Since we are hard coding the vertex data directly in the vertex shader, we'll fill in this tow structs to specify that
	* there is no vertex data or to load for now.
	*/
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	/*
	* VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
	* VK_PRIMITIVE_TOPOLOGY_LINE_lIST: line from every 2vertices without reuse
	* VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex of the next line
	* VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
	* VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and the third vertex of every triangle are used as the first two vertices of the next triangle
	*/
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)swapchainExtent.width;
	viewport.height = (float)swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;

	// It's often convient to make viewport and scissor state dynamic as it gives a lot more flexibility.
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	/*
	* Without dynamic state, the viewport and scissor rectangle need to be set in the pipline.
	* This makes the viewport and scissor rectangle for this pipeline immutable.
	* Any changes required to these values would require a new pipeline to create with the new values.
	* viewportState.pViewports = &viewport;
	* viewportState.pScissor = &scissor;
	*/

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	//rasterizer.depthBiasConstantFactor = 0.0f; //optional
	//rasterizer.depthBiasClamp = 0.0f;	// optional
	//rasterizer.depthBiasSlopeFactor = 0.0f; // optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	/*
	* Two way of blending
	* Mix the old and the new value to produce a final color
	* Combine the old and new value using bitwise operation
	*/
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline.");
	}

	vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
}

VkShaderModule TriangleApplication::createShaderModule(const std::vector<char>& shader)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shader.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module.");
	}

	return shaderModule;
}

void TriangleApplication::createFrameBuffers()
{
	swapchainFrameBuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainFrameBuffers.size(); i++)
	{
		VkImageView attachments[] = {
			swapchainImageViews[i]
		};

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = swapchainExtent.width;
		createInfo.height = swapchainExtent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &createInfo, nullptr, &swapchainFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer.");
		}
	}
}

void TriangleApplication::createCommanPool()
{
	// we store commands on command buffer and submit them on one of the device queue.
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicFamliy.value();

	if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool.");
	}
}

void TriangleApplication::allocateCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo commandBufferAllocInfo{};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.commandPool = commandPool;
	/*
	* VK_COMMAND_BUFFER_LEVEL_PRIMARY : Can be submitted to a queue for excution, but cannot be called from other command buffer.
	* VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers.
	*/
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer.");
	}
}

void TriangleApplication::createSyncObjects()
{
	imageAvaliableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvaliableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}

}

VkResult TriangleApplication::createDebugMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void TriangleApplication::setupDebugMessenger()
{
	if (!enableLayerValidation)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	generateDebugMessengerCreateInfoEXT(createInfo);

	VkResult result = createDebugMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
	if ( result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger.");
	}
	else
	{ }
}

void TriangleApplication::pickPhysicalDevice()
{
	uint32_t deviciesCount;
	vkEnumeratePhysicalDevices(instance, &deviciesCount, nullptr);

	if (deviciesCount == 0)
	{
		throw std::runtime_error("failed to find devicies");
	}

	std::vector<VkPhysicalDevice> devicies(deviciesCount);
	vkEnumeratePhysicalDevices(instance, &deviciesCount, devicies.data());
	std::cout << "find " << deviciesCount << " physical devicies." << std::endl;

	for(VkPhysicalDevice device : devicies)
	{
		if (isDeviceSuitable(device))
		{
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("falied to find a suitable physical device.");
	}
}

bool TriangleApplication::isDeviceSuitable(VkPhysicalDevice& device)
{
	/*VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	bool result = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;*/
	
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool isExtensionSupport = checkDeviceExtensionsSupport(device);

	bool swapChainAdequate = false;
	if (isExtensionSupport)
	{
		SwapChainSupportDetails details = querySwapchainSupport(device);
		swapChainAdequate = !details.formats.empty() and !details.presentMode.empty();
	}

	return indices.isComplete() and isExtensionSupport and swapChainAdequate;
}

bool TriangleApplication::checkValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> avaliableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());

	if(layerCount > 0)
	{
		bool allLayersFound = true;
		for (const char* validationLayer : validationLayers)
		{
			bool layerFound = false;
			for (const auto& avaliableLayer : avaliableLayers)
			{
				if (strcmp(validationLayer, avaliableLayer.layerName))
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				allLayersFound = false;
				break;
			}
		}

		return allLayersFound;
	}
	else
	{
		return false;
	}
}

bool TriangleApplication::checkDeviceExtensionsSupport(VkPhysicalDevice device)
{
	uint32_t extensionsCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);

	std::vector<VkExtensionProperties> avaliableExtensions(extensionsCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, avaliableExtensions.data());

	int supportExtensionsCount = 0;
	std::vector<std::string> requiredExtensions(deviceExensions.begin(), deviceExensions.end());
	for (const auto& avaliableExtension : avaliableExtensions)
	{
		std::string name = avaliableExtension.extensionName;
		if (std::count(requiredExtensions.begin(), requiredExtensions.end(), name))
		{
			supportExtensionsCount++;
		}

	}

	return supportExtensionsCount == requiredExtensions.size();
}

QueueFamilyIndices TriangleApplication::findQueueFamilies(VkPhysicalDevice& device)
{
	QueueFamilyIndices indices;
	
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (int i = 0; i < queueFamilies.size(); i++)
	{
		const auto& queueFamily = queueFamilies[i];
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicFamliy = i;
		}
		else 
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport)
			{
				indices.presentationFamily = i;
			}
		}


		if (indices.isComplete())
		{
			break;
		}
	}

	return indices;
}

void TriangleApplication::generateDebugMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

std::vector<const char*> TriangleApplication::getRequiredExtentions()
{
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions = nullptr;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
	
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
	if (enableLayerValidation)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

SwapChainSupportDetails TriangleApplication::querySwapchainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t modeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, nullptr);

	if (modeCount != 0)
	{
		details.presentMode.resize(modeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, details.presentMode.data());
	}

	return details;

}

VkSurfaceFormatKHR TriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& avaliableFormats)
{
	for (const auto& format : avaliableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return avaliableFormats[0];
}

VkPresentModeKHR TriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& avaliableModes)
{
	for (const auto& mode : avaliableModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D TriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

std::vector<char> TriangleApplication::readFile(const std::string& path)
{
	/*
	* ios::ate : Start reading at the end of the file
	* The advantage of starting to read at the end of the file is that we
	* can use the read position to determine the of the file and allocate a buffer
	*/
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file.");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

void TriangleApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffers.");
	}

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = swapchainFrameBuffers[imageIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = swapchainExtent;

	/*
	* These two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, 
	* which we used as load operation for the color attachment.
	*/
	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to end command buffer");
	}
}

void TriangleApplication::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, debugMessenger, pAllocator);
	}
	else
	{
		throw std::runtime_error("failed to destroy messenger.");
	}
}

void TriangleApplication::cleanUp()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(logicalDevice, imageAvaliableSemaphores[i], nullptr);
		vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
	}
	
	
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

	for (auto frameBuffer : swapchainFrameBuffers)
	{
		vkDestroyFramebuffer(logicalDevice, frameBuffer, nullptr);
	}

	vkDestroyPipeline(logicalDevice, pipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

	for (auto imgView : swapchainImageViews)
	{
		vkDestroyImageView(logicalDevice, imgView, nullptr);
	}

	vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
	vkDestroyDevice(logicalDevice, nullptr);

	if (enableLayerValidation)
	{
		destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	// Make sure that the surface is destroyed before the instance.
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	
	glfwDestroyWindow(window);
	glfwTerminate();
}