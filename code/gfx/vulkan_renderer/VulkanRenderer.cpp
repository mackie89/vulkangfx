//
//  VulkanRenderer.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/12/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "VulkanRenderer.hpp"
#include "VulkanShader.hpp"
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"
#include "VulkanUtils.hpp"

#include "Core_Utils.hpp"
#include "GLWindow.hpp"

const char* MODEL_PATH = "../data/models/chalet.obj";
const char* TEXTURE_PATH = "../data/textures/chalet.jpg";
const char* VERT_SHADER_PATH = "../data/shaders/compiled/vert.spv";
const char* FRAG_SHADER_PATH = "../data/shaders/compiled/frag.spv";

VulkanRenderer* VulkanRenderer::ourInstance = nullptr;

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
    ourInstance = nullptr;
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
    
    ourInstance = this;

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
    CreateStep(CreateTextures);
    CreateStep(CreateSamplers);
    CreateStep(CreateModels);
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
    
    vkDestroySampler(m_Device, m_HouseTextureSampler, nullptr);
    DeleteTextures();
    DeleteModels();
    
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    
    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    vkDestroyBuffer(m_Device, m_ConstantBuffer, nullptr);
    vkFreeMemory(m_Device, m_ConstantBufferMemory, nullptr);
    
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

void VulkanRenderer::DeleteTextures()
{
    Core_SafeDelete(m_HouseTexture);
}

void VulkanRenderer::DeleteModels()
{
    Core_SafeDelete(m_HouseModel);
}

void VulkanRenderer::UpdateConstantBuffer(uint32_t aFrameOffset)
{
    bool rotate =  true;
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    
    ConstantBufferObject cbo = {};
    
    if(rotate)
        cbo.m_Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(22.5f), glm::vec3(0.0f, 0.0f, 1.0f));
    else
        cbo.m_Model = glm::rotate(glm::mat4(1.0f), glm::radians(180.f), glm::vec3(0.0f, 0.0f, 1.0f));
    
    cbo.m_View = glm::lookAt(glm::vec3(2.5f, 0.f, 1.f), glm::vec3(0.0f, 0.0f, 0.25f), glm::vec3(0.0f, 0.0f, 1.0f));
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
    
    {
        //Core_ScopedTimer timer("Wait Fence", TimeDenom::MilliSeconds);
        // wait incase this frame is still being used
        vkWaitForFences(m_Device, 1, &lockInfo.m_InUse, VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkResetFences(m_Device, 1, &lockInfo.m_InUse);
    }
    
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
        
        m_SwapChainCount = swapCount;
        m_SwapChainImageFormat = surfaceFormat.format;
        m_SwapChainExtent = extent;
    }
        
    return created;
}

bool VulkanRenderer::CreateImageViews()
{
    bool success = true;
    
    m_SwapChainImageViews.resize(m_SwapChainCount);
    
    for (size_t i = 0; i < m_SwapChainCount; ++i)
    {
        success &= VulkanUtils::CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_SwapChainImageViews[i], 1);
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
    
    VulkanShader vertShader = VulkanShader(VERT_SHADER_PATH);
    
    if(vertShader.Load())
        created &= vertShader.CreateShaderModule(m_Device, vertShaderModule);
    else
        return false;
    
    VulkanShader fragShader = VulkanShader(FRAG_SHADER_PATH);
    
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
    
    m_SwapChainFramebuffers.resize(m_SwapChainCount);
    
    for (size_t i = 0; i < m_SwapChainCount; i++)
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
    bool created = VulkanUtils::CreateImage(m_SwapChainExtent.width,
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
    
    created = VulkanUtils::CreateImageView(m_DepthImage, m_DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_DepthImageView, 1);
    
    if(!created)
        return false;
    
    VulkanUtils::TransitionImageLayout(m_DepthImage, m_DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    
    return true;
}

bool VulkanRenderer::CreateTextures()
{
    SCOPE_FUNCTION_MILLI();
    
    m_HouseTexture = new VulkanTexture(TEXTURE_PATH);
    return m_HouseTexture->Load();
}

bool VulkanRenderer::CreateSamplers()
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
    samplerInfo.maxLod = static_cast<float>(m_HouseTexture->GetMipLevel());
    
    return vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_HouseTextureSampler) == VK_SUCCESS;
}

bool VulkanRenderer::CreateModels()
{
    SCOPE_FUNCTION_MILLI();
    
    m_HouseModel = new VulkanModel(MODEL_PATH);
    return m_HouseModel->Load();
}
    
bool VulkanRenderer::CreateConstantBuffer()
{
    const VkDeviceSize minDeviceOffset = m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
    const VkDeviceSize neededSize = sizeof(ConstantBufferObject);
    
    if(neededSize <= minDeviceOffset)
        m_MinConstantBufferSize = minDeviceOffset;
    else
        m_MinConstantBufferSize = ((neededSize / minDeviceOffset) + 1) * minDeviceOffset;
    
    const VkDeviceSize bufferSize = m_MinConstantBufferSize * m_SwapChainCount;
    const VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
    if(!VulkanUtils::CreateBuffer(bufferSize, usage, properties, m_ConstantBuffer, m_ConstantBufferMemory))
        return false;
    
    return true;
}

bool VulkanRenderer::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = m_SwapChainCount;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = m_SwapChainCount;
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<u_int32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 128;
    
    return (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) == VK_SUCCESS);
}

bool VulkanRenderer::CreateDescriptorSet()
{
    m_DescriptorSet.resize(m_SwapChainCount);
    
    std::vector<VkDescriptorSetLayout> layouts(m_SwapChainCount, m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = m_SwapChainCount;
    allocInfo.pSetLayouts = layouts.data();
    
    if(vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSet.data()) != VK_SUCCESS)
        return false;
    
    for(int i = 0; i < m_SwapChainCount; ++i)
    {
        VkDescriptorSet& currentSet = m_DescriptorSet[i];
        
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_ConstantBuffer;
        bufferInfo.offset = m_MinConstantBufferSize * i;
        bufferInfo.range = sizeof(ConstantBufferObject);
        
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_HouseTexture->GetImageView();
        imageInfo.sampler = m_HouseTextureSampler;
        
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
    m_CommandBuffers.resize(m_SwapChainCount);
    
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();
    
    bool created = vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) == VK_SUCCESS;
    
    if(created)
    {
        for (size_t i = 0, e = m_CommandBuffers.size(); i < e; ++i)
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
                
                m_HouseModel->Draw(currentCmdBuffer);
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









