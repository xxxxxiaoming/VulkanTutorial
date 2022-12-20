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
	}
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