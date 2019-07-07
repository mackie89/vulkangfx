//
//  VulkanCommon.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/19/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef VulkanCommon_hpp
#define VulkanCommon_hpp

#define VK_USE_PLATFORM_MACOS_MVK
#include <vulkan/vulkan.h>

#include <vector>
#include <array>
#include <set>
#include <unordered_map>
#include <thread>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Core_ScopedTimer.hpp"

struct PositionColorVertex
{
    glm::vec3 m_Pos;
    glm::vec3 m_Color;
    glm::vec2 m_UV;
    
    bool operator== (const PositionColorVertex& other) const
    {
        return m_Pos == other.m_Pos && m_Color == other.m_Color && m_UV == other.m_UV;
    }
    
    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(PositionColorVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    
    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
        
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(PositionColorVertex, m_Pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(PositionColorVertex, m_Color);
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(PositionColorVertex, m_UV);
        
        return attributeDescriptions;
    }
};

namespace std
{
    template<> struct hash<PositionColorVertex>
    {
        size_t operator()(PositionColorVertex const& vert) const
        {
            return ((hash<glm::vec3>()(vert.m_Pos) ^ (hash<glm::vec3>()(vert.m_Color) << 1)) >> 1) ^ (hash<glm::vec2>()(vert.m_UV) << 1);
        }
    };
}

struct ConstantBufferObject
{
    glm::mat4 m_Model;
    glm::mat4 m_View;
    glm::mat4 m_Proj;
};

//----------------------------------------------------------------------
struct QueueFamilyIndices
{
    QueueFamilyIndices();
    bool IsComplete();
    
    int m_GraphicsFamily;
    int m_PresentFamily;
};

//----------------------------------------------------------------------
struct SwapChainSupportDetails
{
    SwapChainSupportDetails();
    
    VkSurfaceCapabilitiesKHR        m_Capabilities;
    std::vector<VkSurfaceFormatKHR> m_Formats;
    std::vector<VkPresentModeKHR>   m_PresentModes;
};

//---------------------------------------------------------------------------
namespace VK_Common
{
    extern const std::vector<VkFormat> ourDepthFormats;
    extern const std::vector<const char*> ourDeviceExtensions;
    
    bool CheckForValidExtensions(uint32_t winExtensionCount, const char** winExtensions);
}

#ifdef _DEBUG
//---------------------------------------------------------------------------
namespace VK_Debug
{
    extern const std::vector<const char*> ourValidationLayers;
    
    bool CheckValidationLayerSupport();
    
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags,
                                                 VkDebugReportObjectTypeEXT objType,
                                                 uint64_t obj,
                                                 size_t location,
                                                 int32_t code,
                                                 const char* layerPrefix,
                                                 const char* msg,
                                                 void* userData);
    
    VkResult CreateDebugReportCallbackEXT(VkInstance instance,
                                          const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkDebugReportCallbackEXT* pCallback);
    
    void DestroyDebugReportCallbackEXT(VkInstance instance,
                                       VkDebugReportCallbackEXT callback,
                                       const VkAllocationCallbacks* pAllocator);
}
#endif // _DEBUG

#endif /* VulkanCommon_hpp */
