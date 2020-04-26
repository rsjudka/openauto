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

#pragma once

#include <f1x/openauto/autoapp/Service/IServiceFactory.hpp>
#include <f1x/openauto/autoapp/Configuration/IConfiguration.hpp>
#include <f1x/openauto/autoapp/Projection/InputDevice.hpp>
#include <f1x/openauto/autoapp/Projection/OMXVideoOutput.hpp>
#include <f1x/openauto/autoapp/Projection/QtVideoOutput.hpp>
#include <f1x/openauto/autoapp/Service/SensorService.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace service
{

class ServiceFactory: public IServiceFactory
{
public:
    ServiceFactory(boost::asio::io_service& ioService, configuration::IConfiguration::Pointer configuration, QWidget* activeArea=nullptr, std::function<void(bool)> activeCallback=nullptr, bool nightMode = false);
    ServiceList create(aasdk::messenger::IMessenger::Pointer messenger) override;
    void setOpacity(unsigned int alpha);
    void resize();
    void setNightMode(bool nightMode);
    static QRect mapActiveAreaToGlobal(QWidget* activeArea);
#ifdef USE_OMX
    static projection::DestRect QRectToDestRect(QRect rect);
#endif

private:
    IService::Pointer createVideoService(aasdk::messenger::IMessenger::Pointer messenger);
    IService::Pointer createBluetoothService(aasdk::messenger::IMessenger::Pointer messenger);
    IService::Pointer createInputService(aasdk::messenger::IMessenger::Pointer messenger);
    void createAudioServices(ServiceList& serviceList, aasdk::messenger::IMessenger::Pointer messenger);

    boost::asio::io_service& ioService_;
    configuration::IConfiguration::Pointer configuration_;
    QWidget* activeArea_;
    QRect screenGeometry_;
    std::function<void(bool)> activeCallback_;
    std::shared_ptr<projection::InputDevice> inputDevice_;
#ifdef USE_OMX
    std::shared_ptr<projection::OMXVideoOutput> omxVideoOutput_;
#else
    projection::QtVideoOutput *qtVideoOutput_;
#endif
    bool nightMode_;
    std::shared_ptr<SensorService> sensorService_;
};

}
}
}
}
