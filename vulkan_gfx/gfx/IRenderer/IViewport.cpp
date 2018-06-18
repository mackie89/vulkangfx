//
//  IViewport.cpp
//  OpenGL
//
//  Created by Michael Mackie on 5/5/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "IViewport.hpp"
#include "IWindow.hpp"

IViewport::IViewport(const IWindow* aWindow)
 : m_Window(aWindow)
 , m_ViewportWidth(0)
 , m_ViewportHeight(0)
{
    
}

IViewport::~IViewport()
{

}

bool IViewport::Init()
{
    GetViewportSize(m_ViewportWidth, m_ViewportHeight);
    return true;
}
