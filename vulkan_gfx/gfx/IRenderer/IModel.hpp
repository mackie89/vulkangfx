//
//  IModel.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 9/2/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef IModel_hpp
#define IModel_hpp

#include <string>

class IModel
{
public:
    IModel(const char* aModelFile);
    virtual ~IModel();
    
    virtual bool Load() = 0;
    
protected:
    std::string m_ModelFile;
};

#endif /* IModel_hpp */
