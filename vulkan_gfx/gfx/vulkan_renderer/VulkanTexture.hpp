//
//  VulkanTexture.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 6/22/19.
//  Copyright © 2019 Michael Mackie. All rights reserved.
//

#ifndef VulkanTexture_hpp
#define VulkanTexture_hpp

#include "VulkanCommon.hpp"
#include "ITexture.hpp"

class VulkanTexture : public ITexture
{
public:
    VulkanTexture(const char* aTextureFile);
    ~VulkanTexture();
    
    virtual bool Load();
    
    uint32_t            GetMipLevel() const { return m_MipLevels; }
    const VkImageView&  GetImageView() const { return m_ImageView; }
    
private:
    
    bool CreateImage();
    bool CreateImageView();
    
    VkImage         m_Image;
    VkImageView     m_ImageView;
    VkDeviceMemory  m_ImageMemory;
    
    uint32_t        m_MipLevels;
};

#endif /* VulkanTexture_hpp */
