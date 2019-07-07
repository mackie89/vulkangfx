//
//  Core_Utils.h
//  VulkanGfx
//
//  Created by Michael Mackie on 5/11/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef Core_Utils_h
#define Core_Utils_h

enum RenderType
{
    RENDER_VULKAN
};

enum WindowType
{
    WINDOW_GLFW
};

#define Core_SafeDelete(arg)    if(arg)             \
                                    delete arg;     \
                                arg = nullptr;

#endif /* Core_Utils_h */
