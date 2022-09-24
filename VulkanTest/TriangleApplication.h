#pragma once
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>

class TriangleApplication
{
public:
	void run();

private:
	const uint32_t HEIGHT = 600;
	const uint32_t WIDTH  = 800;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	GLFWwindow* window   = nullptr;
	VkInstance  instance = nullptr;

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

	// Tool Functions
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtentions();

	void cleanUp();
};