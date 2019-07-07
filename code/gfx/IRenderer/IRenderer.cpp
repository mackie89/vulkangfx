//
//  Renderer.cpp
//  OpenGL
//
//  Created by Michael Mackie on 5/5/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "IRenderer.hpp"

#include "IWindow.hpp"

IRenderer::IRenderer(IWindow* aWindow)
 : m_Window(aWindow)
{
}

IRenderer::~IRenderer()
{
    m_Window = nullptr;
}

bool IRenderer::Init()
{
    return true;
}

void IRenderer::Update()
{

}

void IRenderer::Shutdown()
{
    
}

void IRenderer::WaitForSafeShutdown()
{
    
}
