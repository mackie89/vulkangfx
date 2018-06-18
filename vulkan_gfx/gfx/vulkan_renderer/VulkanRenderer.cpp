//
//  VulkanRenderer.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/12/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "VulkanRenderer.hpp"
#include "VulkanShader.hpp"

#include "Core_Utils.hpp"
#include "GLWindow.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "obj_loader.h"

const std::string MODEL_PATH = "models/chalet.obj";
const std::string TEXTURE_PATH = "textures/chalet.jpg";

//---------------------------------------------------------------------------
// VulkanRenderer
//---------------------------------------------------------------------------
VulkanRenderer::VulkanRenderer(IWindow* aWindow)
 : IRenderer(aWindow)
 , m_PhysicalDevice(VK_NULL_HANDLE)
 , m_VKInstCreated(false)
 , m_VKDeviceCreated(false)
 , m_CurrentFrame(0)
{
}

VulkanRenderer::~VulkanRenderer()
{
}

#define CreateStep(func)    if(created)                                             \
                                created &= func();                                  \
                            if(!created)                                            \
                            {                                                       \
                                std::cout << "Failed in: " << #func << std::endl;   \
                                return false;                                       \
                            }

bool VulkanRenderer::Init()
{
    if(!IRenderer::Init())
        return false;
    
    bool created = true;

    CreateStep(CreateVKInstance);
#ifdef _DEBUG
    CreateStep(SetupDebugCallback);
#endif
    CreateStep(CreateSurface);
    CreateStep(SelectPhysicalDevice);
    CreateStep(CreateLogicalDevice)
    CreateStep(CreateSwapChain)
    CreateStep(CreateImageViews);
    CreateStep(CreateRenderPass)
    CreateStep(CreateDescriptorSetLayout)
    CreateStep(CreateGraphicsPipeline);
    CreateStep(CreateCommandPool);
    CreateStep(CreateDepthResources);
    CreateStep(CreateFrameBuffers);
    CreateStep(CreateTextureImage);
    CreateStep(CreateTextureImageView);
    CreateStep(CreateTextureSampler);
    CreateStep(CreateModelFromFile);
    CreateStep(CreateVertexBuffer);
    CreateStep(CreateIndexBuffer);
    CreateStep(CreateConstantBuffer);
    CreateStep(CreateDescriptorPool);
    CreateStep(CreateDescriptorSet)
    CreateStep(CreateCommandBuffers);
    CreateStep(CreateSyncObjects);
    
    return created;
}

bool VulkanRenderer::RecreateSwapChain()
{
    if (m_Window->GetWidth() == 0 || m_Window->GetHeight() == 0)
        return false;

    vkDeviceWaitIdle(m_Device);
    
    CleanupSwapChain();
    
    bool created = true;
    
    CreateStep(CreateSwapChain);
    CreateStep(CreateImageViews);
    CreateStep(CreateRenderPass);
    CreateStep(CreateGraphicsPipeline);
    CreateStep(CreateDepthResources);
    CreateStep(CreateFrameBuffers);
    CreateStep(CreateCommandBuffers);
    
    return created;
}

void VulkanRenderer::Update()
{
    IRenderer::Update();
    //UpdateConstantBuffer();
    DrawFrame();
}

void VulkanRenderer::WaitForSafeShutdown()
{
    vkDeviceWaitIdle(m_Device);
}

bool VulkanRenderer::CleanupSwapChain()
{
    vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
    vkDestroyImage(m_Device, m_DepthImage, nullptr);
    vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);
    
    for (VkFramebuffer& framebuffer : m_SwapChainFramebuffers)
    {
        vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
    }
    
    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
    
    vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    
    for (VkImageView& imageView : m_SwapChainImageViews)
    {
        vkDestroyImageView(m_Device, imageView, nullptr);
    }
    
    vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
    
    return true;
}

void VulkanRenderer::Shutdown()
{
    CleanupSwapChain();
    
    vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    vkDestroyImageView(m_Device, m_TextureImageView, nullptr);
    
    vkDestroyImage(m_Device, m_TextureImage, nullptr);
    vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);
    
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    
    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    vkDestroyBuffer(m_Device, m_ConstantBuffer, nullptr);
    vkFreeMemory(m_Device, m_ConstantBufferMemory, nullptr);
    
    vkDestroyBuffer(m_Device, m_ModelIndexBuffer, nullptr);
    vkFreeMemory(m_Device, m_ModelIndexBufferMemory, nullptr);
    
    vkDestroyBuffer(m_Device, m_ModelVertexBuffer, nullptr);
    vkFreeMemory(m_Device, m_ModelVertexBufferMemory, nullptr);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        SwapChainLocks& lockInfo = m_SwapChainLocks[i];
        vkDestroySemaphore(m_Device, lockInfo.m_ImageAvailable, nullptr);
        vkDestroySemaphore(m_Device, lockInfo.m_RenderFinished, nullptr);
        vkDestroyFence(m_Device, lockInfo.m_InUse, nullptr);
    }
    
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    
    if(m_VKDeviceCreated)
    {
        vkDestroyDevice(m_Device, nullptr);
    }
    
    if(m_VKInstCreated)
    {
#ifdef _DEBUG
        VK_Debug::DestroyDebugReportCallbackEXT(m_VKInstance, m_DebugCallback, nullptr);
#endif
        vkDestroySurfaceKHR(m_VKInstance, m_Surface, nullptr);
        vkDestroyInstance(m_VKInstance, nullptr);
    }
    
    IRenderer::Shutdown();
}

void VulkanRenderer::UpdateConstantBuffer(uint32_t aFrameOffset)
{
    bool rotate =  false;
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    
    ConstantBufferObject cbo = {};
    
    if(rotate)
        cbo.m_Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(22.5f), glm::vec3(0.0f, 0.0f, 1.0f));
    else
        cbo.m_Model = glm::rotate(glm::mat4(1.0f), glm::radians(180.f), glm::vec3(0.0f, 0.0f, 1.0f));
    
    cbo.m_View = glm::lookAt(glm::vec3(1.3f, 0.f, 0.5f), glm::vec3(0.0f, 0.0f, 0.25f), glm::vec3(0.0f, 0.0f, 1.0f));
    cbo.m_Proj = glm::perspective(glm::radians(45.0f), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 0.1f, 10.0f);
    
    // flip the y axis as glm was designed for OpenGL
    cbo.m_Proj[1][1] *= -1;
    
    void* data;
    const size_t structSize = sizeof(ConstantBufferObject);
    vkMapMemory(m_Device, m_ConstantBufferMemory, m_MinConstantBufferSize * aFrameOffset, structSize, 0, &data);
    memcpy(data, &cbo, structSize);
    vkUnmapMemory(m_Device, m_ConstantBufferMemory);
}

void VulkanRenderer::DrawFrame()
{
    SwapChainLocks& lockInfo = m_SwapChainLocks[m_CurrentFrame];
    
    // wait incase this frame is still being used
    vkWaitForFences(m_Device, 1, &lockInfo.m_InUse, VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(m_Device, 1, &lockInfo.m_InUse);
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, std::numeric_limits<uint64_t>::max(), lockInfo.m_ImageAvailable, VK_NULL_HANDLE, &imageIndex);
    
    UpdateConstantBuffer(imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return;
    }
    
    VkSemaphore submitDoneSemaphores[] = { lockInfo.m_RenderFinished };
    
    // sumbit but wait for image to be aquired
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore waitSemaphores[] = { lockInfo.m_ImageAvailable };
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];
        
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = submitDoneSemaphores;
        
        bool submitted = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, lockInfo.m_InUse) == VK_SUCCESS;
        
        if(!submitted)
            return;
    }
    
    // present but wait for image to submitted and rendered
    {
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = submitDoneSemaphores;
        
        VkSwapchainKHR swapChains[] = { m_SwapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        
        vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    }
    
    //inc to next frame
    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool VulkanRenderer::GetRequiredExtensions(std::vector<const char*>& outExtensions)
{
    uint32_t winExtensionCount = 0;
    const char** winExtensions;
    
    winExtensions = m_Window->GetExtensionList(&winExtensionCount);
    
    if(!VK_Common::CheckForValidExtensions(winExtensionCount, winExtensions))
        return false;
    
    outExtensions.insert(outExtensions.end(), winExtensions, winExtensions + winExtensionCount);
    
#ifdef _DEBUG
    outExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
    return true;
}

bool VulkanRenderer::CreateVKInstance()
{
#ifdef _DEBUG
    if(!VK_Debug::CheckValidationLayerSupport())
        return false;
#endif

    std::vector<const char*> extensions;
    if(!GetRequiredExtensions(extensions))
        return false;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef _DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(VK_Debug::ourValidationLayers.size());
    createInfo.ppEnabledLayerNames = VK_Debug::ourValidationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif
    
    m_VKInstCreated = vkCreateInstance(&createInfo, nullptr, &m_VKInstance) == VK_SUCCESS;

    return m_VKInstCreated;
}

#ifdef _DEBUG
bool VulkanRenderer::SetupDebugCallback()
{
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = VK_Debug::DebugCallback;
    
    return VK_Debug::CreateDebugReportCallbackEXT(m_VKInstance, &createInfo, nullptr, &m_DebugCallback) == VK_SUCCESS;
}
#endif

bool VulkanRenderer::CreateSurface()
{
    VkResult result = static_cast<GLWindow*>(m_Window)->CreateWindowSurface(m_VKInstance, &m_Surface);
    return result == VK_SUCCESS;
}

bool VulkanRenderer::SelectPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_VKInstance, &deviceCount, nullptr);
    
    if (deviceCount == 0)
        return false;
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_VKInstance, &deviceCount, devices.data());
    
    for (const VkPhysicalDevice& device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            m_PhysicalDevice = device;
            m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);
            vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
            
            break;
        }
    }
    
    return m_PhysicalDevice != VK_NULL_HANDLE;
}

bool VulkanRenderer::DeviceSupportExtensions(const VkPhysicalDevice& aDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(aDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(aDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(VK_Common::ourDeviceExtensions.begin(), VK_Common::ourDeviceExtensions.end());
    
    for(const VkExtensionProperties& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool VulkanRenderer::IsDeviceSuitable(const VkPhysicalDevice& aDevice)
{
    bool isSuitable = false;
    bool isExtensionsSupported = false;
    bool isSwapChainAdequate = false;
    
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(aDevice, &deviceProperties);
    
    if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        QueueFamilyIndices indices = FindQueueFamilies(aDevice);
        isSuitable = indices.IsComplete();
    }
   
    isExtensionsSupported = DeviceSupportExtensions(aDevice);
    
    if (isExtensionsSupported)
    {
        SwapChainSupportDetails swapChainDetails;
        QuerySwapChainSupport(aDevice, swapChainDetails);
        isSwapChainAdequate = (!swapChainDetails.m_Formats.empty() && !swapChainDetails.m_PresentModes.empty());
    }
    
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(aDevice, &supportedFeatures);
    
    return isSuitable && isExtensionsSupported && isSwapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanRenderer::FindQueueFamilies(const VkPhysicalDevice& aDevice)
{
    QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0)
        {
            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.m_GraphicsFamily = i;
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(aDevice, i, m_Surface, &presentSupport);
            
            if(presentSupport)
                indices.m_PresentFamily = i;
        }
        
        if (indices.IsComplete())
            break;
        
        i++;
    }
    
    return indices;
}

bool VulkanRenderer::CreateLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {m_QueueFamilyIndices.m_GraphicsFamily, m_QueueFamilyIndices.m_PresentFamily};
    
    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    //specify the features we want
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(VK_Common::ourDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = VK_Common::ourDeviceExtensions.data();

#ifdef _DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(VK_Debug::ourValidationLayers.size());
    createInfo.ppEnabledLayerNames = VK_Debug::ourValidationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif
    
    m_VKDeviceCreated = (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) == VK_SUCCESS);
    
    if(m_VKDeviceCreated)
    {
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.m_GraphicsFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.m_PresentFamily, 0, &m_PresentQueue);
    }
    
    return m_VKDeviceCreated;
}

void VulkanRenderer::QuerySwapChainSupport(const VkPhysicalDevice& aDevice, SwapChainSupportDetails& outSomeDetails)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(aDevice, m_Surface, &outSomeDetails.m_Capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(aDevice, m_Surface, &formatCount, nullptr);
    
    if (formatCount != 0)
    {
        outSomeDetails.m_Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(aDevice, m_Surface, &formatCount, outSomeDetails.m_Formats.data());
    }
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(aDevice, m_Surface, &presentModeCount, nullptr);
    
    if (presentModeCount != 0)
    {
        outSomeDetails.m_PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(aDevice, m_Surface, &presentModeCount, outSomeDetails.m_PresentModes.data());
    }
}

VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    const int size = (int)availableFormats.size();
    
    if(size == 0)
        return {};
    
    if (size == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }
    
    for (const VkSurfaceFormatKHR& format : availableFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }
    
    return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
    
    for (const VkPresentModeKHR& presentMode : availablePresentModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentMode;
        }
        /*else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            bestMode = presentMode;
        }*/
    }
    
    return bestMode;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = {(uint32_t)m_Window->GetWidth(), (uint32_t)m_Window->GetHeight()};
        
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        
        return actualExtent;
    }
}

bool VulkanRenderer::CreateSwapChain()
{
    SwapChainSupportDetails swapChainSupport;
    QuerySwapChainSupport(m_PhysicalDevice, swapChainSupport);
    
    VkSurfaceCapabilitiesKHR& capabilities = swapChainSupport.m_Capabilities;
    
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_Formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_PresentModes);
    VkExtent2D extent = ChooseSwapExtent(capabilities);
    
    uint32_t imageCount = capabilities.minImageCount + 1;
    if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    uint32_t queueFamilyIndices[] = {(uint32_t)m_QueueFamilyIndices.m_GraphicsFamily,
                                     (uint32_t)m_QueueFamilyIndices.m_PresentFamily};
    
    if (m_QueueFamilyIndices.m_GraphicsFamily != m_QueueFamilyIndices.m_PresentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    
    createInfo.preTransform = swapChainSupport.m_Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    bool created = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain) == VK_SUCCESS;
    
    if(created)
    {
        uint32_t swapCount = 0;
        vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &swapCount, nullptr);
        m_SwapChainImages.resize(swapCount);
        vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &swapCount, m_SwapChainImages.data());
        
        m_SwapChainImageFormat = surfaceFormat.format;
        m_SwapChainExtent = extent;
    }
        
    return created;
}

bool VulkanRenderer::CreateImageViews()
{
    bool success = true;
    
    m_SwapChainImageViews.resize(m_SwapChainImages.size());
    
    for (size_t i = 0, e = m_SwapChainImages.size(); i < e; ++i)
    {
        success &= CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_SwapChainImageViews[i], 1);
    }
    
    return success;
}

bool VulkanRenderer::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_SwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkFormat depthFormat;
    
    if(FindDepthFormat(depthFormat))
        m_DepthFormat = depthFormat;
    else
        return false;
    
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = m_DepthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    return vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS;
}

bool VulkanRenderer::CreateDescriptorSetLayout()
{
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
    
    VkDescriptorSetLayoutBinding& cboLayoutBinding = bindings[0];
    cboLayoutBinding.binding = 0;
    cboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cboLayoutBinding.descriptorCount = 1;
    cboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    cboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding& samplerLayoutBinding = bindings[1];
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
        return false;
    
    return true;
}

bool VulkanRenderer::CreateGraphicsPipeline()
{
    bool created = true;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    
    VulkanShader vertShader = VulkanShader("shaders/vert.spv");
    
    if(vertShader.Load())
        created &= vertShader.CreateShaderModule(m_Device, vertShaderModule);
    else
        return false;
    
    VulkanShader fragShader = VulkanShader("shaders/frag.spv");
    
    if(fragShader.Load())
        created &= fragShader.CreateShaderModule(m_Device, fragShaderModule);
    else
        return false;
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    auto bindingDescription = PositionColorVertex::GetBindingDescription();
    auto attributeDescriptions = PositionColorVertex::GetAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_SwapChainExtent.width;
    viewport.height = (float)m_SwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_SwapChainExtent;
    
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //_LINE _POINT
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional
    
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional
    
    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    
    created &= vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) == VK_SUCCESS;
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    created &= vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) == VK_SUCCESS;
    
    vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
    
    return created;
}

bool VulkanRenderer::CreateFrameBuffers()
{
    bool created = true;
    
    m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());
    
    for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 2> attachments =
        {
            m_SwapChainImageViews[i],
            m_DepthImageView
        };
        
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_SwapChainExtent.width;
        framebufferInfo.height = m_SwapChainExtent.height;
        framebufferInfo.layers = 1;
        
        created &= vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) == VK_SUCCESS;
    }
    
    return created;
}

bool VulkanRenderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_QueueFamilyIndices.m_GraphicsFamily;
    poolInfo.flags = 0; // Optional
    
    return vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) == VK_SUCCESS;
}

bool VulkanRenderer::CreateDepthResources()
{
    bool created = CreateImage(m_SwapChainExtent.width,
                               m_SwapChainExtent.height,
                               1,
                               m_DepthFormat,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               m_DepthImage,
                               m_DepthImageMemory);
    
    if(!created)
        return false;
    
    created = CreateImageView(m_DepthImage, m_DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_DepthImageView, 1);
    
    if(!created)
        return false;
    
    TransitionImageLayout(m_DepthImage, m_DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    
    return true;
}

bool VulkanRenderer::CreateTextureImage()
{
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    m_TextureMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    
    if(!pixels)
        return false;
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    // create staging buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!CreateBuffer(imageSize, usage, properties, stagingBuffer, stagingBufferMemory))
            return false;
    }
    
    void* data;
    vkMapMemory(m_Device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_Device, stagingBufferMemory);
    
    stbi_image_free(pixels);
    
    {
        const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        const VkBufferUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        CreateImage(texWidth, texHeight, m_TextureMipLevels, format, tiling, usage, properties, m_TextureImage, m_TextureImageMemory);
      
        TransitionImageLayout(m_TextureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_TextureMipLevels);
        CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        GenerateMipmaps(m_TextureImage, texWidth, texHeight, m_TextureMipLevels);
    }
    
    vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
    vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
    
    return true;
}

bool VulkanRenderer::CreateTextureImageView()
{
    return CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_TextureImageView, m_TextureMipLevels);
}

bool VulkanRenderer::CreateTextureSampler()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0; // Optional
    samplerInfo.maxLod = static_cast<float>(m_TextureMipLevels);
    
    return vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_TextureSampler) == VK_SUCCESS;
}

bool VulkanRenderer::CreateImageView(VkImage anImage, VkFormat aFormat, VkImageAspectFlags anAspectFlags, VkImageView& anImageView, uint32_t aMipLvl)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = anImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = aFormat;
    viewInfo.subresourceRange.aspectMask = anAspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = aMipLvl;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    return (vkCreateImageView(m_Device, &viewInfo, nullptr, &anImageView) == VK_SUCCESS);
}

bool VulkanRenderer::CreateImage(uint32_t aWidth, uint32_t aHeight, uint32_t aMipLvl, VkFormat aFormat, VkImageTiling aTiling,
                                 VkImageUsageFlags aUsage, VkMemoryPropertyFlags aProperties, VkImage& anImage, VkDeviceMemory& anImageMemory)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = aWidth;
    imageInfo.extent.height = aHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = aMipLvl;
    imageInfo.arrayLayers = 1;
    imageInfo.format = aFormat;
    imageInfo.tiling = aTiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = aUsage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional
    
    if(vkCreateImage(m_Device, &imageInfo, nullptr, &anImage) != VK_SUCCESS)
        return false;
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, anImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, aProperties);
    
    if(vkAllocateMemory(m_Device, &allocInfo, nullptr, &anImageMemory) != VK_SUCCESS)
        return false;
    
    vkBindImageMemory(m_Device, anImage, anImageMemory, 0);
    
    return true;
}

void VulkanRenderer::GenerateMipmaps(VkImage& anImage, int32_t aTexWidth, int32_t aTexHeight, uint32_t aMipLevels)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = anImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    
    int32_t mipWidth = aTexWidth;
    int32_t mipHeight = aTexHeight;
    
    for (uint32_t i = 1; i < aMipLevels; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        
        VkImageBlit blit = {};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        
        vkCmdBlitImage(commandBuffer,
                       anImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       anImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        
        if (mipWidth > 1)
            mipWidth /= 2;
        
        if (mipHeight > 1)
            mipHeight /= 2;
    }
    
    barrier.subresourceRange.baseMipLevel = aMipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    EndSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer& aCommandBuffer)
{
    vkEndCommandBuffer(aCommandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &aCommandBuffer;
    
    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);
    
    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &aCommandBuffer);
}

void VulkanRenderer::CopyBuffer(VkBuffer aSrcBuffer, VkBuffer aDstBuffer, VkDeviceSize aSize)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    
    VkBufferCopy copyRegion = {};
    copyRegion.size = aSize;
    vkCmdCopyBuffer(commandBuffer, aSrcBuffer, aDstBuffer, 1, &copyRegion);
    
    EndSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::CopyBufferToImage(VkBuffer aBuffer, VkImage anImage, uint32_t aWidth, uint32_t aHeight)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {aWidth, aHeight, 1};
    
    vkCmdCopyBufferToImage(commandBuffer, aBuffer, anImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    EndSingleTimeCommands(commandBuffer);
}

bool VulkanRenderer::TransitionImageLayout(VkImage anImage, VkFormat aFormat, VkImageLayout anOldLayout, VkImageLayout aNewLayout, uint32_t aMipLvl)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = anOldLayout;
    barrier.newLayout = aNewLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = anImage;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = aMipLvl;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    if (aNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if (HasStencilComponent(aFormat))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    
    if (anOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && aNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (anOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && aNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (anOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && aNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        return false;
    }
    
    vkCmdPipelineBarrier(commandBuffer,
                         sourceStage, destinationStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    EndSingleTimeCommands(commandBuffer);
    
    return true;
}

bool VulkanRenderer::CreateBuffer(VkDeviceSize aSize, VkBufferUsageFlags aUsage, VkMemoryPropertyFlags someProperties, VkBuffer& aBuffer, VkDeviceMemory& aBufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = aSize;
    bufferInfo.usage = aUsage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &aBuffer) != VK_SUCCESS)
        return false;
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device, aBuffer, &memRequirements);
    
    uint32_t memoryIndex = FindMemoryType(memRequirements.memoryTypeBits, someProperties);
    
    if(memoryIndex == std::numeric_limits<uint32_t>::max())
        return false;
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryIndex;
    
    if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &aBufferMemory) != VK_SUCCESS)
        return false;
    
    vkBindBufferMemory(m_Device, aBuffer, aBufferMemory, 0);
    
    return true;
}

bool VulkanRenderer::CreateModelFromFile()
{
    SCOPE_FUNCTION_MILLI();
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str()))
        return false;
    
    std::unordered_map<PositionColorVertex, uint32_t> uniqueVertices = {};
    
    for (const tinyobj::shape_t& shape : shapes)
    {
        for (const tinyobj::index_t& currentIndex : shape.mesh.indices)
        {
            PositionColorVertex vertex = {};
            
            const int vertIndex = 3 * currentIndex.vertex_index;
            const int uvIndex = 2 * currentIndex.texcoord_index;
            
            vertex.m_Pos =
            {
                attrib.vertices[vertIndex],
                attrib.vertices[vertIndex + 1],
                attrib.vertices[vertIndex + 2]
            };
            
            //obj expect bottom-left / vulkan expects top-left
            vertex.m_UV =
            {
                attrib.texcoords[uvIndex],
                1.0f - attrib.texcoords[uvIndex + 1]
            };
            
            vertex.m_Color = {1.0f, 1.0f, 1.0f};
            
            uint32_t index = 0;
            std::unordered_map<PositionColorVertex, uint32_t>::const_iterator vertItr = uniqueVertices.find(vertex);
            
            if (vertItr == uniqueVertices.end())
            {
                index = static_cast<uint32_t>(m_ModelVertices.size());
                uniqueVertices[vertex] = index;
                m_ModelVertices.push_back(vertex);
            }
            else
            {
                index = vertItr->second;
            }
            
            m_ModelIndices.push_back(index);
        }
    }
    
    return true;
}

bool VulkanRenderer::CreateVertexBuffer()
{
    const VkDeviceSize size = sizeof(PositionColorVertex) * m_ModelVertices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    // create staging buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        if(!CreateBuffer(size, usage, properties, stagingBuffer, stagingBufferMemory))
            return false;
    }
    
    void* data;
    vkMapMemory(m_Device, stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, m_ModelVertices.data(), size);
    vkUnmapMemory(m_Device, stagingBufferMemory);
    
    // create device local buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        if(!CreateBuffer(size, usage, properties, m_ModelVertexBuffer, m_ModelVertexBufferMemory))
            return false;
    }
    
    CopyBuffer(stagingBuffer, m_ModelVertexBuffer, size);
    
    vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
    vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
    
    m_ModelVertices.clear();
    m_ModelVertices.shrink_to_fit();
    
    return true;
}

bool VulkanRenderer::CreateIndexBuffer()
{
    const VkDeviceSize bufferSize = sizeof(m_ModelIndices[0]) * m_ModelIndices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    // create staging buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!CreateBuffer(bufferSize, usage, properties, stagingBuffer, stagingBufferMemory))
            return false;
    }

    void* data;
    vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_ModelIndices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_Device, stagingBufferMemory);
    
    
    // create device local buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        if(!CreateBuffer(bufferSize, usage, properties, m_ModelIndexBuffer, m_ModelIndexBufferMemory))
            return false;
    }
    
    CopyBuffer(stagingBuffer, m_ModelIndexBuffer, bufferSize);
    
    vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
    vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
    
    m_ModelIndexCount = static_cast<uint32_t>(m_ModelIndices.size());
    m_ModelIndices.clear();
    m_ModelIndices.shrink_to_fit();
    
    return true;
}

bool VulkanRenderer::CreateConstantBuffer()
{
    const VkDeviceSize minDeviceOffset = m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
    const VkDeviceSize neededSize = sizeof(ConstantBufferObject);
    
    if(neededSize <= minDeviceOffset)
        m_MinConstantBufferSize = minDeviceOffset;
    else
        m_MinConstantBufferSize = ((neededSize / minDeviceOffset) + 1) * minDeviceOffset;
    
    const VkDeviceSize bufferSize = m_MinConstantBufferSize * MAX_FRAMES_IN_FLIGHT;
    const VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
    if(!CreateBuffer(bufferSize, usage, properties, m_ConstantBuffer, m_ConstantBufferMemory))
        return false;
    
    return true;
}

bool VulkanRenderer::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<u_int32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 128;
    
    return (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) == VK_SUCCESS);
}

bool VulkanRenderer::CreateDescriptorSet()
{
    m_DescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkDescriptorSetLayout layouts[] = {m_DescriptorSetLayout, m_DescriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;
    
    if(vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSet.data()) != VK_SUCCESS)
        return false;
    
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorSet& currentSet = m_DescriptorSet[i];
        
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_ConstantBuffer;
        bufferInfo.offset = m_MinConstantBufferSize * i;
        bufferInfo.range = sizeof(ConstantBufferObject);
        
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_TextureImageView;
        imageInfo.sampler = m_TextureSampler;
        
        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
        
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = currentSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr; // Optional
        descriptorWrites[0].pTexelBufferView = nullptr; // Optional
        
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = currentSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    
    return true;
}

bool VulkanRenderer::CreateCommandBuffers()
{
    m_CommandBuffers.resize(m_SwapChainFramebuffers.size());
    
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();
    
    bool created = vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) == VK_SUCCESS;
    
    if(created)
    {
        for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
        {
            VkCommandBuffer& currentCmdBuffer = m_CommandBuffers[i];
            
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr; // Optional
            
            created &= vkBeginCommandBuffer(currentCmdBuffer, &beginInfo) == VK_SUCCESS;

            if(!created)
                break;
            
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_RenderPass;
            renderPassInfo.framebuffer = m_SwapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = m_SwapChainExtent;
            
            const float greyColor = 0.0f;
    
            std::array<VkClearValue, 2> clearValues = {};
            clearValues[0].color = {greyColor, greyColor, greyColor, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();
            
            vkCmdBeginRenderPass(currentCmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(currentCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
                vkCmdBindDescriptorSets(currentCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet[i], 0, nullptr);
                
                VkBuffer vertexBuffers[] = {m_ModelVertexBuffer};
                VkDeviceSize offsets[] = {0};
                
                vkCmdBindVertexBuffers(currentCmdBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(currentCmdBuffer, m_ModelIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                
                vkCmdDrawIndexed(currentCmdBuffer, m_ModelIndexCount, 1, 0, 0, 0);
            }
            vkCmdEndRenderPass(currentCmdBuffer);
            
            created &= vkEndCommandBuffer(currentCmdBuffer) == VK_SUCCESS;
            
            if(!created)
                break;
        }
    }
    
    return created;
}

bool VulkanRenderer::CreateSyncObjects()
{
    bool created = true;
    
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        SwapChainLocks& lockInfo = m_SwapChainLocks[i];
        created &= vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &lockInfo.m_ImageAvailable) == VK_SUCCESS;
        created &= vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &lockInfo.m_RenderFinished) == VK_SUCCESS;
        
        created &= vkCreateFence(m_Device, &fenceInfo, nullptr, &lockInfo.m_InUse) == VK_SUCCESS;
    }
    
    return created;
}

uint32_t VulkanRenderer::FindMemoryType(uint32_t aTypeFilter, VkMemoryPropertyFlags someProperties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((aTypeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & someProperties) == someProperties))
        {
            return i;
        }
    }
    
    return std::numeric_limits<uint32_t>::max();
}

uint32_t VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat>& someCandidates, VkImageTiling aTiling, VkFormatFeatureFlags aFeatures)
{
    uint32_t index = 0;
    for (const VkFormat& format : someCandidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
        
        if (aTiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & aFeatures) == aFeatures)
        {
            return index;
        }
        else if (aTiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & aFeatures) == aFeatures)
        {
            return index;
        }
        
        ++index;
    }
    
    return std::numeric_limits<uint32_t>::max();
}

bool VulkanRenderer::FindDepthFormat(VkFormat& outFormat)
{
    u_int32_t index = FindSupportedFormat(VK_Common::ourDepthFormats,
                                          VK_IMAGE_TILING_OPTIMAL,
                                          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    
    if(index != std::numeric_limits<uint32_t>::max())
    {
        outFormat = VK_Common::ourDepthFormats[index];
        return true;
    }
    
    return false;
}

bool VulkanRenderer::HasStencilComponent(const VkFormat& aFormat)
{
    return aFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || aFormat == VK_FORMAT_D24_UNORM_S8_UINT;
}










