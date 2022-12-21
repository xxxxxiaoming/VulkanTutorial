#pragma once
//#define NDEBUG
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <set>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <optional>
#include <limits>
#include <algorithm>
#include <fstream>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicFamliy;
	std::optional<uint32_t> presentationFamily;

	bool isComplete()
	{
		return graphicFamliy.has_value() and presentationFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentMode;
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
	const int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	const std::vector<const char*> deviceExensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	uint32_t currentFrame = 0;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFrameBuffers;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvaliableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;


	GLFWwindow* window   = nullptr;
	VkInstance  instance = nullptr;
	VkSurfaceKHR surface;
	VkQueue		presentationQueue; // handle to interface with the queue
	VkQueue		graphicQueue;	// handle to interface with the queue
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice	logicalDevice;
	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkCommandPool commandPool;
	VkDebugUtilsMessengerEXT debugMessenger;

#ifdef NDEBUG
	bool enableLayerValidation = false;
#else
	bool enableLayerValidation = true;
#endif // NDEBUG

	// Loop Application
	void mainLoop();

	// Draw Frame !!!
	void drawFrame();

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

	// Swapchain Settings
	void createSwapChain();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& avaliableFormats);  // choose color depth and color space
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& avaliableModes); // choose a mode about how to present an img
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities); // resolution of images in swap chain

	// Createa ImageView Objects
	void createImageViews();

	// Create pipeline
	void createRenderPass();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& shader);

	// Create Frambuffers
	void createFrameBuffers();

	// Create Command Pool
	void createCommanPool();

	// Allocate Command Buffer
	void allocateCommandBuffers();

	// Create Sync Objects
	void createSyncObjects();

	// Tool Functions
	void setupDebugMessenger();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice& device);
	bool checkValidationLayerSupport();
	bool checkDeviceExtensionsSupport(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice& device);
	void generateDebugMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	std::vector<const char*> getRequiredExtentions();
	SwapChainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
	static std::vector<char> readFile(const std::string& path);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	// Clean up
	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void cleanUp();
};