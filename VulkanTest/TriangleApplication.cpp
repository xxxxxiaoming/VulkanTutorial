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
	createInfo.enabledExtensionCount = 0;	// We won't need any device speicfic extentions for now.

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
	return indices.isComplete();
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