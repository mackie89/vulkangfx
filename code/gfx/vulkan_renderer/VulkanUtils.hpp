//
//  VulkanUtils.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 9/2/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef VulkanUtils_hpp
#define VulkanUtils_hpp

#include "VulkanCommon.hpp"

namespace VulkanUtils
{
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer& aCommandBuffer);
    
    uint32_t FindMemoryType(VkPhysicalDevice aPhysicalDevice, uint32_t aTypeFilter, VkMemoryPropertyFlags someProperties);
    
    bool HasStencilComponent(const VkFormat& aFormat);
    
    bool CreateBuffer(VkDeviceSize aSize, VkBufferUsageFlags aUsage, VkMemoryPropertyFlags someProperties, VkBuffer& aBuffer, VkDeviceMemory& aBufferMemory);
    void CopyBuffer(VkBuffer aSrcBuffer, VkBuffer aDstBuffer, VkDeviceSize aSize);
    void CopyBufferToImage(VkBuffer aBuffer, VkImage anImage, uint32_t aWidth, uint32_t aHeight);
    
    bool CreateImageView(VkImage anImage, VkFormat aFormat, VkImageAspectFlags anAspectFlags, VkImageView& anImageView, uint32_t aMipLvl);
    void GenerateMipmaps(VkImage& anImage, int32_t aTexWidth, int32_t aTexHeight, uint32_t aMipLevels);
    bool TransitionImageLayout(VkImage anImage, VkFormat aFormat, VkImageLayout anOldLayout, VkImageLayout aNewLayout, uint32_t aMipLvl);
    
    bool CreateImage(uint32_t aWidth, uint32_t aHeight, uint32_t aMipLvl, VkFormat aFormat, VkImageTiling aTiling,
                     VkImageUsageFlags aUsage, VkMemoryPropertyFlags aProperties, VkImage& anImage, VkDeviceMemory& anImageMemory);
}

#endif /* VulkanUtils_hpp */
