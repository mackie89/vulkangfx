//
//  Core_ScopedTimer.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 6/2/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef Core_ScopedTimer_hpp
#define Core_ScopedTimer_hpp

#include <chrono>
#include <string>

enum TimeDenom
{
    NanoSeconds,
    MicroSeconds,
    MilliSeconds,
    Seconds,
};

class Core_ScopedTimer
{
public:
    Core_ScopedTimer(const char* aName, TimeDenom aDenomitation);
    ~Core_ScopedTimer();

private:
    typedef std::chrono::duration<float> seconds_type;
    typedef std::chrono::duration<float,std::milli> milliseconds_type;
    typedef std::chrono::duration<long long, std::micro> microseconds_type;
    typedef std::chrono::duration<long long, std::nano> nanoseconds_type;
    
    using _Clock = std::chrono::high_resolution_clock;
    using _TimePoint = std::chrono::time_point<_Clock>;
    
    _TimePoint m_StartTime;
    TimeDenom m_TimeDenomination;
    std::string m_Name;
};

#define SCOPE_FUNCTION_NANO()   Core_ScopedTimer timer(__FUNCTION__, TimeDenom::NanoSeconds);
#define SCOPE_FUNCTION_MICRO()  Core_ScopedTimer timer(__FUNCTION__, TimeDenom::MicroSeconds);
#define SCOPE_FUNCTION_MILLI()  Core_ScopedTimer timer(__FUNCTION__, TimeDenom::MilliSeconds);
#define SCOPE_FUNCTION_SEC()    Core_ScopedTimer timer(__FUNCTION__, TimeDenom::Seconds);

#endif /* Core_ScopedTimer_hpp */
