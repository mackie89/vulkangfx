//
//  VulkanTexture.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 6/22/19.
//  Copyright © 2019 Michael Mackie. All rights reserved.
//

#include "VulkanTexture.hpp"
#include "VulkanRenderer.hpp"

#include "VulkanUtils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

VulkanTexture::VulkanTexture(const char* aTextureFile)
 : ITexture(aTextureFile)
{
}

VulkanTexture::~VulkanTexture()
{
    VulkanRenderer* renderer = VulkanRenderer::GetInstance();
    VkDevice& aDevice = renderer->GetLogicalDevice();
    
    vkDestroyImageView(aDevice, m_ImageView, nullptr);
    vkDestroyImage(aDevice, m_Image, nullptr);
    vkFreeMemory(aDevice, m_ImageMemory, nullptr);
}

bool VulkanTexture::Load()
{
    bool success = true;
    
    success &= CreateImage();
    
    if(success)
        success &= CreateImageView();
    
    
    return success;
}

bool VulkanTexture::CreateImage()
{
    VulkanRenderer* renderer = VulkanRenderer::GetInstance();
    VkDevice& aDevice = renderer->GetLogicalDevice();
    
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    
    stbi_uc* pixels = stbi_load(m_TextureFile.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    
    if(!pixels)
        return false;
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    // create staging buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!VulkanUtils::CreateBuffer(imageSize, usage, properties, stagingBuffer, stagingBufferMemory))
            return false;
    }
    
    void* data;
    vkMapMemory(aDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(aDevice, stagingBufferMemory);
    
    stbi_image_free(pixels);
    
    {
        const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        const VkBufferUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        VulkanUtils::CreateImage(texWidth, texHeight, m_MipLevels, format, tiling, usage, properties, m_Image, m_ImageMemory);
        VulkanUtils::TransitionImageLayout(m_Image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
        VulkanUtils::CopyBufferToImage(stagingBuffer, m_Image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        VulkanUtils::GenerateMipmaps(m_Image, texWidth, texHeight, m_MipLevels);
    }
    
    vkDestroyBuffer(aDevice, stagingBuffer, nullptr);
    vkFreeMemory(aDevice, stagingBufferMemory, nullptr);
    
    return true;
}

bool VulkanTexture::CreateImageView()
{
    return VulkanUtils::CreateImageView(m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_ImageView, m_MipLevels);
}
