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
	if (enableLayerValidation)
	{
		destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroyInstance(instance, nullptr);
	
	glfwDestroyWindow(window);
	glfwTerminate();
}