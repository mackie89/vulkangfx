//
//  Core_Application.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/11/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef Core_Application_hpp
#define Core_Application_hpp

#include "Core_Utils.hpp"

class IWindow;
class IRenderer;

class Core_Application
{
public:
    Core_Application(WindowType aWindowType, RenderType aRenderType);
    virtual ~Core_Application();
    
    virtual bool Run();
    
private:
    virtual bool Init();
    virtual void Update();
    virtual void Shutdown();
    
    IWindow* CreateWindow();
    IRenderer* CreateRenderer();
    
    IWindow* m_Window;
    IRenderer* m_Renderer;
    
    WindowType m_WindowType;
    RenderType m_RenderType;
    
    bool m_Initialized;
};

#endif /* Core_Application_hpp */
