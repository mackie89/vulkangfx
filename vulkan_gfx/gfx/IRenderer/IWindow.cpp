//
//  IWindow.cpp
//  OpenGL
//
//  Created by Michael Mackie on 5/5/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "IWindow.hpp"

IWindow::IWindow(int aWidth, int aHeight)
: m_Width(aWidth)
, m_Height(aHeight)
, m_CallbackUniqueID(0)
{
}

IWindow::~IWindow()
{
}

const char** IWindow::GetExtensionList(uint32_t* outExtensionCount)
{
    *outExtensionCount = 0;
    return nullptr;
}

int IWindow::RegisterWindowChangedCallback(const WindowChangedCB &cb)
{
    const int newID = m_CallbackUniqueID++;
    m_WindowChangedCB[newID] = cb;
    return newID;
}

void IWindow::UnregisterWindowChangedCallback(int anID)
{
    CallbackIDMap::iterator it = m_WindowChangedCB.find(anID);
    
    if (it != m_WindowChangedCB.end())
        m_WindowChangedCB.erase(it);
}
