//
//  Core_ScopedTimer.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 6/2/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "Core_ScopedTimer.hpp"

#include <iostream>
#include <iomanip>

Core_ScopedTimer::Core_ScopedTimer(const char* aName, TimeDenom aDemonitation)
: m_TimeDenomination(aDemonitation)
, m_StartTime(_Clock::now())
, m_Name(aName)
{
}

Core_ScopedTimer::~Core_ScopedTimer()
{
    auto timeTaken = _Clock::now() - m_StartTime;
    
    switch (m_TimeDenomination)
    {
        case TimeDenom::NanoSeconds:
            {
                nanoseconds_type time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(timeTaken);
                std::cout << m_Name << ": " << time_ns.count() << "(ns)" << std::endl;
            }
            break;
        case TimeDenom::MicroSeconds:
            {
                microseconds_type time_us = std::chrono::duration_cast<std::chrono::microseconds>(timeTaken);
                std::cout << m_Name << ": " << time_us.count() << "(us)" << std::endl;
            }
            break;
        case TimeDenom::MilliSeconds:
            {
                milliseconds_type time_ms = timeTaken;
                std::cout << m_Name << ": " << std::fixed << std::setprecision(3) << time_ms.count() << "(ms)" << std::endl;
            }
            break;
        case TimeDenom::Seconds:
            {
                seconds_type time_s = timeTaken;
                std::cout << m_Name << ": " << std::fixed << std::setprecision(4) << time_s.count() << "(s)" << std::endl;
            }
            break;
        default:
            break;
    }
}
