//
//  GLWindow.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/12/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "GLWindow.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

GLWindow::GLWindow(int aWidth, int aHeight)
 : IWindow(aWidth, aHeight)
 , myInternalWindow(nullptr)
{
    
}

GLWindow::~GLWindow()
{
    
}

bool GLWindow::Init()
{
    if(!IWindow::Init())
        return false;
    
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    myInternalWindow = glfwCreateWindow(GetWidth(), GetHeight(), "Vulkan Engine", nullptr, nullptr);
    glfwSetWindowUserPointer(myInternalWindow, this);
    
    glfwSetWindowSizeCallback(myInternalWindow, WindowSizeCallback);
    
    return true;
}

void GLWindow::Update()
{
    
}

void GLWindow::Shutdown()
{
    glfwDestroyWindow(myInternalWindow);
    glfwTerminate();
    
    myInternalWindow = nullptr;
}

bool GLWindow::ShouldCloseWindow() const
{
    return myInternalWindow ? glfwWindowShouldClose(myInternalWindow) : true;
}

void GLWindow::PollEvents()
{
    glfwPollEvents();
}

const char** GLWindow::GetExtensionList(uint32_t* outExtensionCount)
{
    return glfwGetRequiredInstanceExtensions(outExtensionCount);
}

VkResult GLWindow::CreateWindowSurface(VkInstance& anInstance, VkSurfaceKHR* aSurface)
{
    return glfwCreateWindowSurface(anInstance, myInternalWindow, nullptr, aSurface);
}

void GLWindow::WindowSizeCallback(GLFWwindow* aWindow, int aWidth, int aHeight)
{
    GLWindow* thisWindow = static_cast<GLWindow*>(glfwGetWindowUserPointer(aWindow));
    thisWindow->m_Width = aWidth;
    thisWindow->m_Height = aHeight;
}
