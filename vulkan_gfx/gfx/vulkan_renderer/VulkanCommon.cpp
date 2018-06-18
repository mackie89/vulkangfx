//
//  VulkanCommon.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/19/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "VulkanCommon.hpp"

//---------------------------------------------------------------------------
// QueueFamilyIndices
//---------------------------------------------------------------------------
QueueFamilyIndices::QueueFamilyIndices()
: m_GraphicsFamily(-1)
, m_PresentFamily(-1)
{
}

bool QueueFamilyIndices::IsComplete()
{
    return m_GraphicsFamily >= 0 && m_PresentFamily >= 0;
}

//---------------------------------------------------------------------------
// SwapChainSupportDetails
//---------------------------------------------------------------------------
SwapChainSupportDetails::SwapChainSupportDetails()
{
}

//---------------------------------------------------------------------------
namespace VK_Common
{
    const std::vector<const char*> ourDeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    const std::vector<VkFormat> ourDepthFormats =
    {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    
    bool CheckForValidExtensions(uint32_t winExtensionCount, const char** winExtensions)
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        
        for(int reqIndex = 0; reqIndex < winExtensionCount; ++reqIndex)
        {
            bool found = false;
            const char* currentExt = winExtensions[reqIndex];
            
            for (const VkExtensionProperties& extension : extensions)
            {
                if(std::strcmp(currentExt, extension.extensionName) == 0)
                {
                    found = true;
                    continue;
                }
            }
            
            if(!found)
                return false;
        }
        
        return true;
    }
}

#ifdef _DEBUG
//---------------------------------------------------------------------------
namespace VK_Debug
{
    const std::vector<const char*> ourValidationLayers =
    {
        "VK_LAYER_LUNARG_standard_validation"
    };
    
    bool CheckValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        
        for (const char* layerName : ourValidationLayers)
        {
            bool layerFound = false;
            
            for (const VkLayerProperties& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }
            
            if (!layerFound)
                return false;
        }
        
        return true;
    }
    
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags,
                                                 VkDebugReportObjectTypeEXT objType,
                                                 uint64_t obj,
                                                 size_t location,
                                                 int32_t code,
                                                 const char* layerPrefix,
                                                 const char* msg,
                                                 void* userData)
    {
        std::cerr << "validation layer: " << msg << std::endl;
        return VK_FALSE;
    }
    
    VkResult CreateDebugReportCallbackEXT(VkInstance instance,
                                          const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkDebugReportCallbackEXT* pCallback)
    {
        auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pCallback);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    
    void DestroyDebugReportCallbackEXT(VkInstance instance,
                                       VkDebugReportCallbackEXT callback,
                                       const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        if (func != nullptr)
        {
            func(instance, callback, pAllocator);
        }
    }
}
#endif // _DEBUG
