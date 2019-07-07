//
//  VulkanShader.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/20/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "VulkanShader.hpp"
#include <fstream>

VulkanShader::VulkanShader(const char* aShaderFile)
 : IShader(aShaderFile)
{
}
    
bool VulkanShader::Load()
{
    return LoadFile();
}

bool VulkanShader::LoadFile()
{
    std::ifstream file(m_ShaderFile, std::ios::ate | std::ios::binary);
    
    if (!file.is_open())
        return false;
    
    size_t fileSize = (size_t) file.tellg();
    myFileBuffer.resize(fileSize);

    file.seekg(0);
    file.read(myFileBuffer.data(), fileSize);
    
    file.close();
    
    return true;
}

bool VulkanShader::CreateShaderModule(VkDevice& aDevice, VkShaderModule& outShaderMod) const
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = myFileBuffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(myFileBuffer.data());
    
    bool success = vkCreateShaderModule(aDevice, &createInfo, nullptr, &outShaderMod) == VK_SUCCESS;

    return success;
}


