//
//  main.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/5/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//


#include "Core_Application.hpp"

int main()
{
    Core_Application app(WINDOW_GLFW, RENDER_VULKAN);
    
    if(app.Run())
        return 0;
    else
        return -1;
}
