//
//  IViewport.hpp
//  OpenGL
//
//  Created by Michael Mackie on 5/5/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef IViewport_hpp
#define IViewport_hpp

class IWindow;

class IViewport
{
public:
    IViewport(const IWindow* aWindow);
    virtual ~IViewport();
    
    virtual bool Init();
    
    int GetWidth() const { return m_ViewportWidth; }
    int GetHeight() const { return m_ViewportHeight; }
    
protected:
    virtual void GetViewportSize(int& aWidth, int& aHeight) = 0;
    
    const IWindow* m_Window;
    
private:
    int m_ViewportWidth;
    int m_ViewportHeight;
};

#endif /* IViewport_hpp */
