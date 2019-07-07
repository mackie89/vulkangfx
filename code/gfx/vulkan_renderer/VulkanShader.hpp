//
//  VulkanShader.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/20/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef VulkanShader_hpp
#define VulkanShader_hpp

#include "IShader.hpp"
#include "VulkanCommon.hpp"

class VulkanShader : public IShader
{
public:
    VulkanShader(const char* aShaderFile);
    
    bool Load() override;
    bool CreateShaderModule(VkDevice& aDevice, VkShaderModule& outShaderMod) const;
    
private:
    bool LoadFile();
    
    std::vector<char> myFileBuffer;
};

#endif /* VulkanShader_hpp */
