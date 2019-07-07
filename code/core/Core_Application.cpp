//
//  Core_Application.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 5/11/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "Core_Application.hpp"

#include "GLWindow.hpp"
#include "VulkanRenderer.hpp"

const int WIDTH = 1000;
const int HEIGHT = 750;

using namespace std::chrono_literals;

Core_Application::Core_Application(WindowType aWindowType, RenderType aRenderType)
 : m_Window(nullptr)
 , m_Renderer(nullptr)
 , m_WindowType(aWindowType)
 , m_RenderType(aRenderType)
 , m_Initialized(false)
{
}
    
Core_Application::~Core_Application()
{
    Core_SafeDelete(m_Window);
    Core_SafeDelete(m_Renderer);
}
    
bool Core_Application::Init()
{
    bool success = true;
    
    m_Window = CreateWindow();
    success &= m_Window ? m_Window->Init() : false;
    
    if(success)
    {
        m_Renderer = CreateRenderer();
        success &= m_Renderer ? m_Renderer->Init() : false;
    }

    m_Initialized = success;
    return success;
}

void Core_Application::Shutdown()
{
    // shut down in reverse order to init
    
    if(m_Renderer)
        m_Renderer->Shutdown();
    
    if(m_Window)
        m_Window->Shutdown();
    
    m_Initialized = false;
}

void Core_Application::Update()
{
    constexpr std::chrono::nanoseconds timestep(1000ms);
    using clock = std::chrono::high_resolution_clock;
    auto time_start = clock::now();
    
    int counter = 0;
    
    while (!m_Window->ShouldCloseWindow())
    {
        {
            //Core_ScopedTimer timer("Poll Event", TimeDenom::MilliSeconds);
            m_Window->PollEvents();
        }
        
        m_Window->Update();
        m_Renderer->Update();
        
        ++counter;
        
        const auto now = clock::now();
        const auto delta_time =  now - time_start;
        if(delta_time >= timestep)
        {
            std::cout << "Current FPS:" << counter << std::endl;
            
            counter = 0;
            time_start = now;
        }
    }
    
    m_Renderer->WaitForSafeShutdown();
}

bool Core_Application::Run()
{
    bool initSuccess = Init();
    
    if(m_Initialized)
        Update();
    
    Shutdown();
    
    return initSuccess;
}

IWindow* Core_Application::CreateWindow()
{
    switch (m_WindowType)
    {
        case WINDOW_GLFW:
            return new GLWindow(WIDTH, HEIGHT);
        default:
            return nullptr;
    }
}

IRenderer* Core_Application::CreateRenderer()
{
    switch (m_RenderType)
    {
        case RENDER_VULKAN:
            return new VulkanRenderer(m_Window);
        default:
            return nullptr;
    }
}
