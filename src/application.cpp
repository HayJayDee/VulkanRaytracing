#include "application.h"
#include <iostream>
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include "loader.h"

#define VK_ASSERT(stmt, msg) if(stmt != VK_SUCCESS){throw std::runtime_error(msg);}

void RayTracingApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void RayTracingApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
}

void RayTracingApplication::createInstance() {

    if(enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation Layer requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Raytracing";
    appInfo.pEngineName = "No Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(0,0,1);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    appInfo.engineVersion = VK_MAKE_VERSION(0,0,1);

    std::vector<const char*> requiredExtensions = getRequiredExtensions();
    checkAndAddInstanceExtensionSupport(requiredExtensions);

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    if(enableValidationLayers){
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        VkDebugUtilsMessengerCreateInfoEXT msgCreateInfo;
        populateMessenger(msgCreateInfo);
        instanceCreateInfo.pNext = &msgCreateInfo;
    }else {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.pNext = nullptr;
    }

    VK_ASSERT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "Failed to initialize Vulkan Instance!")
}

void RayTracingApplication::setupDebugMessenger() {
    if(!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateMessenger(createInfo);
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VK_ASSERT(func(instance, &createInfo, nullptr, &debugMessenger), "DebugUtilsMessenger could not be created");
    }else {
        throw std::runtime_error("Could not find InstanceProcAddr of vkCreateDebugUtilsMessengerEXT");
    }
}

void RayTracingApplication::populateMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

void RayTracingApplication::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    VK_ASSERT(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr), "Could not enumerate physical devices");
    if(deviceCount == 0) throw std::runtime_error("Could not enumerate physical devices!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_ASSERT(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()), "Could not enumerate physical devices");
    
    for(const auto& device : devices) {
        if(isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("Could not find any suitable physical device");
}

bool RayTracingApplication::isDeviceSuitable(VkPhysicalDevice phyDevice) {
    VkPhysicalDeviceProperties deviceProps;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(phyDevice, &deviceProps);
    vkGetPhysicalDeviceFeatures(phyDevice, &deviceFeatures);
    QueueFamilyIndicies indices = findQueueFamilies(phyDevice);

    bool swapAdequate = false;
    if(checkDeviceExtensionSupport(phyDevice)){
        SwapChainSupportDetails details = querySwapChainSupport(phyDevice);
        swapAdequate = !details.formats.empty() && !details.presentModes.empty();
    }

    return indices.isComplete() && swapAdequate;
}

VkSurfaceFormatKHR RayTracingApplication::chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for(const auto& format : formats) {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats[0];
}

VkPresentModeKHR RayTracingApplication::chooseSwapChainPresent(const std::vector<VkPresentModeKHR>& presentModes) {

    for(const auto& presentMode : presentModes){
        if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D RayTracingApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width,height;

    glfwGetFramebufferSize(window, &width, &height);
    
    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };
    
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return extent;
}

RayTracingApplication::QueueFamilyIndicies RayTracingApplication::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndicies indicies;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueProperties.data());

    uint32_t i = 0;
    for(const auto& prop : queueProperties) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indicies.graphicsFamily = i;
        }
        if(presentSupport){
            indicies.presentFamily = i;
        }
        if(indicies.isComplete()) break;
        i++;
    }

    return indicies;
}

void RayTracingApplication::checkAndAddInstanceExtensionSupport(std::vector<const char*>& requiredExtensions){

    // Get Instance Extensions
    uint32_t extensionCount = 0;
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr), "Failed to enumerate Instance Extensions!");
    std::vector<VkExtensionProperties> extensions(extensionCount);
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()), "Failed to enumerate Instance Extensions!");

    // Check if all required Extensions are valid
    for(auto& reqExt : requiredExtensions){
        bool foundExt = false;
        for(auto& vkExt : extensions) {
            if(strncmp(vkExt.extensionName, reqExt, 256) == 0){
                foundExt = true;
                break;
            }
        }
        if(!foundExt){
            throw std::runtime_error("Could not find required GLFW Extension: " + std::string(reqExt));
        }
    }

    for(auto& vkExt : extensions) {
        // If we have the GetPhysDevProp Extension we will add it, because if the
        // Device has the VK_KHR_portability_subset Extension, we have to add this too,
        // but we cannot add this later
        if(strncmp(vkExt.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, 256) == 0) {
            requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            break;
        }
    }
}

bool RayTracingApplication::checkDeviceExtensionSupport(VkPhysicalDevice phyDevice) {
    uint32_t deviceExtensionCount;
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &deviceExtensionCount, nullptr), "Could not enumerate Device Extensions!");
    std::vector<VkExtensionProperties> availableExtensions(deviceExtensionCount);
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &deviceExtensionCount, availableExtensions.data()), "Could not enumerate Device Extensions!");
    
    std::set<std::string> reqExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for(const auto& ext : availableExtensions) {
        reqExtensions.erase(ext.extensionName);
    }

    return reqExtensions.empty();
}

RayTracingApplication::SwapChainSupportDetails RayTracingApplication::querySwapChainSupport(VkPhysicalDevice phyDevice) {
    SwapChainSupportDetails details;

    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevice, surface, &details.capabilities), "Could not get physical surface capabilities!");
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &formatCount, nullptr);
    if(formatCount != 0) {
        details.formats.resize(formatCount);    
        vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevice, surface, &presentCount, nullptr);
    if(presentCount != 0) {
        details.presentModes.resize(presentCount);    
        vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevice, surface, &presentCount, details.presentModes.data());
    }

    return details;
}

std::vector<const char*> RayTracingApplication::getRequiredExtensions() {
    uint32_t glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(enableValidationLayers) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    return extensions;
}

bool RayTracingApplication::checkValidationLayerSupport() {
    uint32_t layerCount = 0;
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), "Failed to enumerate Layer Properties");
    std::vector<VkLayerProperties> layers(layerCount);
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()), "Failed to enumerate Layer Properties");
    
    for(auto& layerName : validationLayers) {
        bool layerFound = false;
        for(const auto& layer : layers) {
            if(strncmp(layerName, layer.layerName, 256) == 0) {
                layerFound = true;
                break;
            }
        }

        if(!layerFound) return false;
    }
    
    return true;
}

void RayTracingApplication::createLogicalDevice() {
    QueueFamilyIndicies indices = findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriotity = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriotity;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Vulkan Specification
    // If the device supports VK_KHR_portability_subset it has to be added as an extension
    uint32_t deviceExtensionCount;
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr), "Could not enumerate Device Extensions!");
    std::vector<VkExtensionProperties> availableExtensions(deviceExtensionCount);
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableExtensions.data()), "Could not enumerate Device Extensions!");

    // Fill enabledExtensions with required Extensions
    std::vector<const char*> enabledExtensions(deviceExtensions.begin(), deviceExtensions.end());
    
    // Check if it has subset
    for(auto& ext : availableExtensions) {
        if(strncmp(ext.extensionName, "VK_KHR_portability_subset", 256) == 0) {
            enabledExtensions.push_back("VK_KHR_portability_subset");
        }
    }
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    if(enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }else {
        createInfo.enabledLayerCount = 0;
    }

    VK_ASSERT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "Could not create logical device!");

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void RayTracingApplication::createSwapChain() {
    SwapChainSupportDetails support = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapChainFormat(support.formats);
    VkPresentModeKHR present = chooseSwapChainPresent(support.presentModes);
    VkExtent2D extent = chooseSwapExtent(support.capabilities);

    uint32_t imageCount = support.capabilities.minImageCount + 1;

    // 0 means an infinite amount of images
    if(support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndicies indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndicies[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if(indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndicies;
    }else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = present;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_ASSERT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain), "Could not create Swapchain!");

    VK_ASSERT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr), "Could not get Swapchain Images!");
    swapChainImages.resize(imageCount);
    VK_ASSERT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data()), "Could not get Swapchain Images!");

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void RayTracingApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for(size_t i = 0; i < swapChainImageViews.size(); i++){
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_ASSERT(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]), "Could not create image view!");
    }
}

void RayTracingApplication::createGraphicsPipeline() {
    auto vertCode = Loader::readFile("vert.spv");
    auto fragCode = Loader::readFile("frag.spv");
}


void RayTracingApplication::createSurface(){
    VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Could not create window surface!");
}

void RayTracingApplication::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Raytracing", nullptr, nullptr);
}

void RayTracingApplication::mainLoop() {
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void RayTracingApplication::cleanup() {

    for(auto& imageView : swapChainImageViews){
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    if(enableValidationLayers){
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, nullptr);
        }
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

VKAPI_ATTR VkBool32 VKAPI_CALL RayTracingApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {
    std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
