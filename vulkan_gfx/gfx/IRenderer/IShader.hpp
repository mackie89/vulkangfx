//
//  IShader.hpp
//  OpenGL
//
//  Created by Michael Mackie on 5/5/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef IShader_hpp
#define IShader_hpp

#include <string>

class IShader
{
public:
    IShader(const char* aShaderFile);
    virtual ~IShader();
    
    virtual bool Load() = 0;
    
protected:
    std::string m_ShaderFile;
};

#endif /* IShader_hpp */
