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

class VulkanRenderer : public IRenderer
{
public:
    VulkanRenderer(IWindow* aWindow);
    ~VulkanRenderer();
    
    bool Init() override;
    void Update() override;
    void Shutdown() override;
    
    void WaitForSafeShutdown() override;
    
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
    bool CreateTextureImage();
    bool CreateTextureImageView();
    bool CreateTextureSampler();
    bool CreateModelFromFile();
    bool CreateVertexBuffer();
    bool CreateIndexBuffer();
    bool CreateConstantBuffer();
    bool CreateDescriptorPool();
    bool CreateDescriptorSet();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    
    bool CleanupSwapChain();
    bool RecreateSwapChain();
    
    bool SelectPhysicalDevice();

    bool IsDeviceSuitable(const VkPhysicalDevice& aDevice);
    bool DeviceSupportExtensions(const VkPhysicalDevice& aDevice);
    bool GetRequiredExtensions(std::vector<const char*>& outExtensions);
    
    QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& aDevice);
    
    void QuerySwapChainSupport(const VkPhysicalDevice& aDevice, SwapChainSupportDetails& outSomeDetails);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    uint32_t FindMemoryType(uint32_t aTypeFilter, VkMemoryPropertyFlags someProperties);
    uint32_t FindSupportedFormat(const std::vector<VkFormat>& someCandidates, VkImageTiling aTiling, VkFormatFeatureFlags aFeatures);
    
    bool FindDepthFormat(VkFormat& outFormat);
    bool HasStencilComponent(const VkFormat& aFormat);
    
    bool CreateImageView(VkImage anImage, VkFormat aFormat, VkImageAspectFlags anAspectFlags, VkImageView& anImageView, uint32_t aMipLvl);
    bool CreateImage(uint32_t aWidth, uint32_t aHeight, uint32_t aMipLvl, VkFormat aFormat, VkImageTiling aTiling,
                     VkImageUsageFlags aUsage, VkMemoryPropertyFlags aProperties, VkImage& anImage, VkDeviceMemory& anImageMemory);
    
    void GenerateMipmaps(VkImage& anImage, int32_t aTexWidth, int32_t aTexHeight, uint32_t aMipLevels);
    
    bool CreateBuffer(VkDeviceSize aSize, VkBufferUsageFlags aUsage, VkMemoryPropertyFlags someProperties, VkBuffer& aBuffer, VkDeviceMemory& aBufferMemory);

    void CopyBuffer(VkBuffer aSrcBuffer, VkBuffer aDstBuffer, VkDeviceSize aSize);
    void CopyBufferToImage(VkBuffer aBuffer, VkImage anImage, uint32_t aWidth, uint32_t aHeight);
    
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer& aCommandBuffer);
    
    bool TransitionImageLayout(VkImage anImage, VkFormat aFormat, VkImageLayout anOldLayout, VkImageLayout aNewLayout, uint32_t aMipLvl);
    
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
    
    std::vector<PositionColorVertex>    m_ModelVertices;
    std::vector<uint32_t>               m_ModelIndices;
    uint32_t                            m_ModelIndexCount;
    VkBuffer                            m_ModelVertexBuffer;
    VkBuffer                            m_ModelIndexBuffer;
    VkDeviceMemory                      m_ModelVertexBufferMemory;
    VkDeviceMemory                      m_ModelIndexBufferMemory;
    
    VkBuffer        m_ConstantBuffer;
    VkDeviceMemory  m_ConstantBufferMemory;
    VkDeviceSize    m_MinConstantBufferSize;
    
    VkImage         m_DepthImage;
    VkImageView     m_DepthImageView;
    VkDeviceMemory  m_DepthImageMemory;
    VkFormat        m_DepthFormat;
    
    //textures
    uint32_t        m_TextureMipLevels;
    VkImage         m_TextureImage;
    VkImageView     m_TextureImageView;
    VkDeviceMemory  m_TextureImageMemory;
    VkSampler       m_TextureSampler;

    bool m_VKInstCreated;
    bool m_VKDeviceCreated;
    
#ifdef _DEBUG
    bool SetupDebugCallback();
    VkDebugReportCallbackEXT m_DebugCallback;
#endif
};

#endif /* VulkanRenderer_hpp */
