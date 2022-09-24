#include "TriangleApplication.h"

void TriangleApplication::run()
{
	initWindow();
	initVkn();

	mainLoop();
	cleanUp();
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
	appInfo.apiVersion = VK_API_VERSION_1_3;

	

	/*uint32_t extensiontsCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensiontsCount);*/
	std::vector<const char*> extensions = getRequiredExtentions();
	
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableLayerValidation)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
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

void TriangleApplication::cleanUp()
{
	if (enableLayerValidation)
	{
		// TODO
	}

	vkDestroyInstance(instance, nullptr);
	
	glfwDestroyWindow(window);
	glfwTerminate();
}