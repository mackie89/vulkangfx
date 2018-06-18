//
//  Renderer.hpp
//  OpenGL
//
//  Created by Michael Mackie on 5/5/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef IRenderer_hpp
#define IRenderer_hpp

class IWindow;

class IRenderer
{
public:
    IRenderer(IWindow* aWindow);
    virtual ~IRenderer();
    
    virtual bool Init();
    virtual void Update();
    virtual void Shutdown();
    
    virtual void WaitForSafeShutdown();
    
protected:
    IWindow* m_Window;
};

#endif /* IRenderer_hpp */
