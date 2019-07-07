//
//  GLWindow.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/12/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef GLWindow_hpp
#define GLWindow_hpp

#include "IWindow.hpp"
#include "VulkanCommon.hpp"

class GLFWwindow;

class GLWindow : public IWindow
{
public:
    GLWindow(int aWidth, int aHeight);
    ~GLWindow();
    
    bool Init() override;
    void Update() override;
    void Shutdown() override;
    
    bool ShouldCloseWindow() const override;
    void PollEvents() override;
    
    const char** GetExtensionList(uint32_t* outExtensionCount) override;

    VkResult CreateWindowSurface(VkInstance& anInstance, VkSurfaceKHR* aSurface);
    
private:
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);
    
    GLFWwindow* myInternalWindow;
};

#endif /* GLWindow_hpp */
