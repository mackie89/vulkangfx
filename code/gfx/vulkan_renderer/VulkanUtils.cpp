//
//  VulkanUtils.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 9/2/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "VulkanUtils.hpp"

//#define VK_USE_PLATFORM_MACOS_MVK
//#include <vulkan/vulkan.h>

#include "VulkanRenderer.hpp"

namespace VulkanUtils
{
    VkCommandBuffer BeginSingleTimeCommands()
    {
        VulkanRenderer* renderer = VulkanRenderer::GetInstance();
        
        if(!renderer)
            return VkCommandBuffer();
        
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = renderer->GetCommandPool();
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(renderer->GetLogicalDevice(), &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        return commandBuffer;
    }
    
    void EndSingleTimeCommands(VkCommandBuffer& aCommandBuffer)
    {
        vkEndCommandBuffer(aCommandBuffer);
        
        VulkanRenderer* renderer = VulkanRenderer::GetInstance();
        
        if(!renderer)
            return;
        
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &aCommandBuffer;
        
        vkQueueSubmit(renderer->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(renderer->GetGraphicsQueue());
        
        vkFreeCommandBuffers(renderer->GetLogicalDevice(), renderer->GetCommandPool(), 1, &aCommandBuffer);
    }
    
    uint32_t FindMemoryType(VkPhysicalDevice aPhysicalDevice, uint32_t aTypeFilter, VkMemoryPropertyFlags someProperties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(aPhysicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((aTypeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & someProperties) == someProperties))
            {
                return i;
            }
        }
        
        return std::numeric_limits<uint32_t>::max();
    }
    
    bool HasStencilComponent(const VkFormat& aFormat)
    {
        return aFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || aFormat == VK_FORMAT_D24_UNORM_S8_UINT;
    }
    
    bool CreateBuffer(VkDeviceSize aSize, VkBufferUsageFlags aUsage, VkMemoryPropertyFlags someProperties, VkBuffer& aBuffer, VkDeviceMemory& aBufferMemory)
    {
        VulkanRenderer* renderer = VulkanRenderer::GetInstance();
        
        if(!renderer)
            return false;
        
        VkDevice& aDevice = renderer->GetLogicalDevice();
        VkPhysicalDevice& aPhysDevice = renderer->GetPhysicalDevice();
        
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = aSize;
        bufferInfo.usage = aUsage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if(vkCreateBuffer(aDevice, &bufferInfo, nullptr, &aBuffer) != VK_SUCCESS)
            return false;
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(aDevice, aBuffer, &memRequirements);
        
        uint32_t memoryIndex = FindMemoryType(aPhysDevice, memRequirements.memoryTypeBits, someProperties);
        
        if(memoryIndex == std::numeric_limits<uint32_t>::max())
            return false;
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryIndex;
        
        if (vkAllocateMemory(aDevice, &allocInfo, nullptr, &aBufferMemory) != VK_SUCCESS)
            return false;
        
        vkBindBufferMemory(aDevice, aBuffer, aBufferMemory, 0);
        
        return true;
    }
    
    void CopyBuffer(VkBuffer aSrcBuffer, VkBuffer aDstBuffer, VkDeviceSize aSize)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
        
        VkBufferCopy copyRegion = {};
        copyRegion.size = aSize;
        vkCmdCopyBuffer(commandBuffer, aSrcBuffer, aDstBuffer, 1, &copyRegion);
        
        EndSingleTimeCommands(commandBuffer);
    }
    
    void CopyBufferToImage(VkBuffer aBuffer, VkImage anImage, uint32_t aWidth, uint32_t aHeight)
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
    
    
    void GenerateMipmaps(VkImage& anImage, int32_t aTexWidth, int32_t aTexHeight, uint32_t aMipLevels)
    {
        VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands();
        
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
        
        VulkanUtils::EndSingleTimeCommands(commandBuffer);
    }
    
    bool TransitionImageLayout(VkImage anImage, VkFormat aFormat, VkImageLayout anOldLayout, VkImageLayout aNewLayout, uint32_t aMipLvl)
    {
        VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands();
        
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
        
        VulkanUtils::EndSingleTimeCommands(commandBuffer);
        
        return true;
    }
    
    bool CreateImageView(VkImage anImage, VkFormat aFormat, VkImageAspectFlags anAspectFlags, VkImageView& anImageView, uint32_t aMipLvl)
    {
        VulkanRenderer* renderer = VulkanRenderer::GetInstance();
        
        if(!renderer)
            return false;
        
        VkDevice& aDevice = renderer->GetLogicalDevice();
        
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
        
        return (vkCreateImageView(aDevice, &viewInfo, nullptr, &anImageView) == VK_SUCCESS);
    }
    
    
    bool CreateImage(uint32_t aWidth, uint32_t aHeight, uint32_t aMipLvl, VkFormat aFormat, VkImageTiling aTiling,
                     VkImageUsageFlags aUsage, VkMemoryPropertyFlags aProperties, VkImage& anImage, VkDeviceMemory& anImageMemory)
    {
        VulkanRenderer* renderer = VulkanRenderer::GetInstance();
        
        if(!renderer)
            return false;
        
        VkDevice& aDevice = renderer->GetLogicalDevice();
        VkPhysicalDevice& aPhysDevice = renderer->GetPhysicalDevice();
        
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
        
        if(vkCreateImage(aDevice, &imageInfo, nullptr, &anImage) != VK_SUCCESS)
            return false;
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(aDevice, anImage, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanUtils::FindMemoryType(aPhysDevice, memRequirements.memoryTypeBits, aProperties);
        
        if(vkAllocateMemory(aDevice, &allocInfo, nullptr, &anImageMemory) != VK_SUCCESS)
            return false;
        
        vkBindImageMemory(aDevice, anImage, anImageMemory, 0);
        
        return true;
    }

}
