#pragma once
//#define NDEBUG
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <set>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <optional>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicFamliy;
	std::optional<uint32_t> presentationFamily;

	bool isComplete()
	{
		return graphicFamliy.has_value() and presentationFamily.has_value();
	}
};

class TriangleApplication
{
public:
	void run();

	/* Validation layer callbbcak
	* @param the serverity of the message
	* @param the type of the message (e.g VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
	* @param refers to a VkDebugUtilsMessengerCallbackDataExt struct containing the details of the message itself
	* @param contains a pointer that was specified during the setup of the callback and allows you to pass your own data to it
	* @return a boolean indicates if the Vulkan call that triggered the validation layer message should be aborted
	*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
		);

private:
	const uint32_t HEIGHT = 600;
	const uint32_t WIDTH  = 800;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	GLFWwindow* window   = nullptr;
	VkInstance  instance = nullptr;
	VkSurfaceKHR surface;
	VkQueue		presentationQueue; // handle to interface with the queue
	VkQueue		graphicQueue;	// handle to interface with the queue
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice	logicalDevice;
	VkDebugUtilsMessengerEXT debugMessenger;

#ifdef NDEBUG
	bool enableLayerValidation = false;
#else
	bool enableLayerValidation = true;
#endif // NDEBUG

	// Loop Application
	void mainLoop();

	// Initialization
	void initVkn();
	void initWindow();

	// Create Vulkan Instance
	void createInstance();

	// Create Surface
	void createSurface();

	// Create Logical Device
	void createLogicalDevice();

	// Createa Vulkan Debug Messenger
	VkResult createDebugMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	// Tool Functions
	void setupDebugMessenger();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice& device);
	bool checkValidationLayerSupport();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice& device);
	void generateDebugMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	std::vector<const char*> getRequiredExtentions();

	// Clean up
	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void cleanUp();
};