//
//  VulkanRenderer.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/12/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef VulkanRenderer_hpp
#define VulkanRenderer_hpp

#include "IRenderer.hpp"
#include "VulkanCommon.hpp"

class VulkanModel;
class VulkanTexture;

class VulkanRenderer : public IRenderer
{
public:
    VulkanRenderer(IWindow* aWindow);
    ~VulkanRenderer();
    
    bool Init() override;
    void Update() override;
    void Shutdown() override;
    
    void WaitForSafeShutdown() override;
    
    static VulkanRenderer* GetInstance() { return ourInstance; }
    
    VkCommandPool&       GetCommandPool() { return m_CommandPool; }
    VkPhysicalDevice&    GetPhysicalDevice() { return m_PhysicalDevice; }
    VkDevice&            GetLogicalDevice() { return m_Device; }
    VkQueue&             GetGraphicsQueue() { return m_GraphicsQueue; }
private:
    static const int MAX_FRAMES_IN_FLIGHT = 2;
    
    struct SwapChainLocks
    {
        VkSemaphore m_ImageAvailable;
        VkSemaphore m_RenderFinished;
        VkFence     m_InUse;
    };
    
    bool CreateVKInstance();
    bool CreateLogicalDevice();
    bool CreateSurface();
    bool CreateSwapChain();
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateDescriptorSetLayout();
    bool CreateGraphicsPipeline();
    bool CreateCommandPool();
    bool CreateDepthResources();
    bool CreateFrameBuffers();
    bool CreateTextures();
    bool CreateSamplers();
    bool CreateModels();
    bool CreateConstantBuffer();
    bool CreateDescriptorPool();
    bool CreateDescriptorSet();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    
    bool CleanupSwapChain();
    bool RecreateSwapChain();
    void DeleteModels();
    void DeleteTextures();
    
    bool SelectPhysicalDevice();

    bool IsDeviceSuitable(const VkPhysicalDevice& aDevice);
    bool DeviceSupportExtensions(const VkPhysicalDevice& aDevice);
    bool GetRequiredExtensions(std::vector<const char*>& outExtensions);
    
    QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& aDevice);
    
    void QuerySwapChainSupport(const VkPhysicalDevice& aDevice, SwapChainSupportDetails& outSomeDetails);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    uint32_t FindSupportedFormat(const std::vector<VkFormat>& someCandidates, VkImageTiling aTiling, VkFormatFeatureFlags aFeatures);
    
    bool FindDepthFormat(VkFormat& outFormat);
    
    void UpdateConstantBuffer(uint32_t aFrameOffset);
    void DrawFrame();
    
    VkInstance          m_VKInstance;
    VkPhysicalDevice    m_PhysicalDevice;
    VkDevice            m_Device;
    VkQueue             m_GraphicsQueue;
    VkQueue             m_PresentQueue;
    VkSurfaceKHR        m_Surface;

    QueueFamilyIndices m_QueueFamilyIndices;
    
    VkPhysicalDeviceProperties m_DeviceProperties;

    VkSwapchainKHR                  m_SwapChain;
    VkFormat                        m_SwapChainImageFormat;
    VkExtent2D                      m_SwapChainExtent;
    std::vector<VkImage>            m_SwapChainImages;
    std::vector<VkImageView>        m_SwapChainImageViews;
    std::vector<VkFramebuffer>      m_SwapChainFramebuffers;
    uint32_t                        m_SwapChainCount;
    

    VkRenderPass                    m_RenderPass;
    VkDescriptorSetLayout           m_DescriptorSetLayout;
    VkDescriptorPool                m_DescriptorPool;
    std::vector<VkDescriptorSet>    m_DescriptorSet;
    VkPipelineLayout                m_PipelineLayout;
    VkPipeline                      m_GraphicsPipeline;
    
    VkCommandPool                   m_CommandPool;
    std::vector<VkCommandBuffer>    m_CommandBuffers;
    
    SwapChainLocks  m_SwapChainLocks[MAX_FRAMES_IN_FLIGHT];
    int             m_CurrentFrame;
    
    VkBuffer        m_ConstantBuffer;
    VkDeviceMemory  m_ConstantBufferMemory;
    VkDeviceSize    m_MinConstantBufferSize;
    
    VkImage         m_DepthImage;
    VkImageView     m_DepthImageView;
    VkDeviceMemory  m_DepthImageMemory;
    VkFormat        m_DepthFormat;
    
    //textures & samplers
    VulkanTexture*  m_HouseTexture;
    VkSampler       m_HouseTextureSampler;
    
    //models
    VulkanModel*    m_HouseModel;
    
    bool m_VKInstCreated;
    bool m_VKDeviceCreated;
    
    static VulkanRenderer* ourInstance;
#ifdef _DEBUG
    bool SetupDebugCallback();
    VkDebugReportCallbackEXT m_DebugCallback;
#endif
};

#endif /* VulkanRenderer_hpp */
