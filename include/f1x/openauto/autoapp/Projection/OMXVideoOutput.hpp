/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_OMX
#pragma once

extern "C"
{
#include <ilclient.h>
}

#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <boost/circular_buffer.hpp>
#include <f1x/openauto/autoapp/Projection/VideoOutput.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{

struct DestRect
{
    DestRect();
    DestRect(OMX_S16 xOffset, OMX_S16 yOffset, OMX_S16 width, OMX_S16 height);

    OMX_BOOL fullscreen;
    OMX_S16 xOffset;
    OMX_S16 yOffset;
    OMX_S16 width;
    OMX_S16 height;
};

class OMXVideoOutput: public VideoOutput
{
public:
    OMXVideoOutput(configuration::IConfiguration::Pointer configuration, DestRect destRect=DestRect(), std::function<void(bool)> activeCallback=nullptr);

    bool open() override;
    bool init() override;
    void write(uint64_t timestamp, const aasdk::common::DataConstBuffer& buffer) override;
    void stop() override;
    void setOpacity(OMX_U32 alpha);
    void setDestRect(DestRect destRect);

private:
    bool createComponents();
    bool initClock();
    bool setupTunnels();
    bool enablePortBuffers();
    bool setupDisplayRegion();

    std::mutex mutex_;
    bool isActive_;
    bool portSettingsChanged_;
    ILCLIENT_T* client_;
    COMPONENT_T* components_[5];
    TUNNEL_T tunnels_[4];
    DestRect destRect_;
    OMX_U32 alpha_;
    std::function<void(bool)> activeCallback_;
};

}
}
}
}

#endif
