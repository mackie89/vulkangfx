//
//  IWindow.hpp
//  OpenGL
//
//  Created by Michael Mackie on 5/5/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef IWindow_hpp
#define IWindow_hpp

#include <cstdlib>
#include <functional>
#include <map>

class IWindow
{
public:
    enum WindowEvent
    {
        WE_INVALID,
        WE_SIZE,
    };
    
    typedef std::function<void(WindowEvent)> WindowChangedCB;
    
    IWindow(int aWidth, int aHeight);
    virtual ~IWindow();
    
    virtual bool Init() { return true; }
    virtual void Update() {};
    virtual void Shutdown() {};
    
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    
    virtual bool ShouldCloseWindow() const { return true; }
    virtual void PollEvents() {}
    
    virtual const char** GetExtensionList(uint32_t* outExtensionCount);
    virtual const void* GetNativeWindow() const { return nullptr; }
    
    int RegisterWindowChangedCallback(const WindowChangedCB &cb);
    void UnregisterWindowChangedCallback(int anID);
    
protected:
    typedef std::map<int, WindowChangedCB> CallbackIDMap;
    
    CallbackIDMap m_WindowChangedCB;
    int m_CallbackUniqueID;
    
    int m_Width;
    int m_Height;
};

#endif /* IWindow_hpp */
