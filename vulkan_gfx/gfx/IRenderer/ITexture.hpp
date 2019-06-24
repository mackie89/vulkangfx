//
//  ITexture.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 6/22/19.
//  Copyright © 2019 Michael Mackie. All rights reserved.
//

#ifndef ITexture_hpp
#define ITexture_hpp

#include <string>

class ITexture
{
public:
    ITexture(const char* aTextureFile);
    virtual ~ITexture();
    
    virtual bool Load() = 0;
    
protected:
    std::string m_TextureFile;
};


#endif /* ITexture_hpp */
