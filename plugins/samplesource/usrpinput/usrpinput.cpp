///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017 Edouard Griffiths, F4EXB                                   //
// Copyright (C) 2020 Jon Beniston, M7RCE                                        //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <string.h>

#include <QMutexLocker>
#include <QDebug>
#include <QNetworkReply>
#include <QBuffer>

#include <uhd/usrp/multi_usrp.hpp>

#include "SWGDeviceSettings.h"
#include "SWGUSRPInputSettings.h"
#include "SWGDeviceState.h"
#include "SWGDeviceReport.h"
#include "SWGUSRPInputReport.h"

#include "device/deviceapi.h"
#include "dsp/dspcommands.h"
#include "dsp/dspengine.h"
#include "usrpinput.h"
#include "usrpinputthread.h"
#include "usrp/deviceusrpparam.h"
#include "usrp/deviceusrpshared.h"
#include "usrp/deviceusrp.h"

MESSAGE_CLASS_DEFINITION(USRPInput::MsgConfigureUSRP, Message)
MESSAGE_CLASS_DEFINITION(USRPInput::MsgGetStreamInfo, Message)
MESSAGE_CLASS_DEFINITION(USRPInput::MsgGetDeviceInfo, Message)
MESSAGE_CLASS_DEFINITION(USRPInput::MsgReportStreamInfo, Message)
MESSAGE_CLASS_DEFINITION(USRPInput::MsgStartStop, Message)

USRPInput::USRPInput(DeviceAPI *deviceAPI) :
    m_deviceAPI(deviceAPI),
    m_settings(),
    m_usrpInputThread(nullptr),
    m_deviceDescription("USRPInput"),
    m_running(false),
    m_channelAcquired(false)
{
    m_streamId = nullptr;
    suspendRxBuddies();
    suspendTxBuddies();
    openDevice();
    resumeTxBuddies();
    resumeRxBuddies();

    m_deviceAPI->setNbSourceStreams(1);

    m_networkManager = new QNetworkAccessManager();
    connect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkManagerFinished(QNetworkReply*)));
}

USRPInput::~USRPInput()
{
    disconnect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkManagerFinished(QNetworkReply*)));
    delete m_networkManager;

    if (m_running) {
        stop();
    }

    suspendRxBuddies();
    suspendTxBuddies();
    closeDevice();
    resumeTxBuddies();
    resumeRxBuddies();
}

void USRPInput::destroy()
{
    delete this;
}

bool USRPInput::openDevice()
{
    if (!m_sampleFifo.setSize(96000 * 4))
    {
        qCritical("USRPInput::openDevice: could not allocate SampleFifo");
        return false;
    }
    else
    {
        qDebug("USRPInput::openDevice: allocated SampleFifo");
    }

    int requestedChannel = m_deviceAPI->getDeviceItemIndex();

    // look for Rx buddies and get reference to common parameters
    // if there is a channel left take the first available
    if (m_deviceAPI->getSourceBuddies().size() > 0) // look source sibling first
    {
        qDebug("USRPInput::openDevice: look in Rx buddies");

        DeviceAPI *sourceBuddy = m_deviceAPI->getSourceBuddies()[0];
        //m_deviceShared = *((DeviceUSRPShared *) sourceBuddy->getBuddySharedPtr()); // copy shared data
        DeviceUSRPShared *deviceUSRPShared = (DeviceUSRPShared*) sourceBuddy->getBuddySharedPtr();

        if (deviceUSRPShared == 0)
        {
            qCritical("USRPInput::openDevice: the source buddy shared pointer is null");
            return false;
        }

        m_deviceShared.m_deviceParams = deviceUSRPShared->m_deviceParams;

        DeviceUSRPParams *deviceParams = m_deviceShared.m_deviceParams; // get device parameters

        if (deviceParams == 0)
        {
            qCritical("USRPInput::openDevice: cannot get device parameters from Rx buddy");
            return false; // the device params should have been created by the buddy
        }
        else
        {
            qDebug("USRPInput::openDevice: getting device parameters from Rx buddy");
        }

        if (m_deviceAPI->getSourceBuddies().size() == deviceParams->m_nbRxChannels)
        {
            qCritical("USRPInput::openDevice: no more Rx channels available in device");
            return false; // no more Rx channels available in device
        }
        else
        {
            qDebug("USRPInput::openDevice: at least one more Rx channel is available in device");
        }

        // check if the requested channel is busy and abort if so (should not happen if device management is working correctly)

        for (unsigned int i = 0; i < m_deviceAPI->getSourceBuddies().size(); i++)
        {
            DeviceAPI *buddy = m_deviceAPI->getSourceBuddies()[i];
            DeviceUSRPShared *buddyShared = (DeviceUSRPShared *) buddy->getBuddySharedPtr();

            if (buddyShared->m_channel == requestedChannel)
            {
                qCritical("USRPInput::openDevice: cannot open busy channel %u", requestedChannel);
                return false;
            }
        }

        m_deviceShared.m_channel = requestedChannel; // acknowledge the requested channel
    }
    // look for Tx buddies and get reference to common parameters
    // take the first Rx channel
    else if (m_deviceAPI->getSinkBuddies().size() > 0) // then sink
    {
        qDebug("USRPInput::openDevice: look in Tx buddies");

        DeviceAPI *sinkBuddy = m_deviceAPI->getSinkBuddies()[0];
        //m_deviceShared = *((DeviceUSRPShared *) sinkBuddy->getBuddySharedPtr()); // copy parameters
        DeviceUSRPShared *deviceUSRPShared = (DeviceUSRPShared*) sinkBuddy->getBuddySharedPtr();

        if (deviceUSRPShared == 0)
        {
            qCritical("USRPInput::openDevice: the sink buddy shared pointer is null");
            return false;
        }

        m_deviceShared.m_deviceParams = deviceUSRPShared->m_deviceParams;

        if (m_deviceShared.m_deviceParams == 0)
        {
            qCritical("USRPInput::openDevice: cannot get device parameters from Tx buddy");
            return false; // the device params should have been created by the buddy
        }
        else
        {
            qDebug("USRPInput::openDevice: getting device parameters from Tx buddy");
        }

        m_deviceShared.m_channel = requestedChannel; // acknowledge the requested channel
    }
    // There are no buddies then create the first USRP common parameters
    // open the device this will also populate common fields
    // take the first Rx channel
    else
    {
        qDebug("USRPInput::openDevice: open device here");

        m_deviceShared.m_deviceParams = new DeviceUSRPParams();
        char serial[256];
        strcpy(serial, qPrintable(m_deviceAPI->getSamplingDeviceSerial()));
        m_deviceShared.m_deviceParams->open(serial);
        m_deviceShared.m_channel = requestedChannel; // acknowledge the requested channel
    }

    m_deviceAPI->setBuddySharedPtr(&m_deviceShared); // propagate common parameters to API

    return true;
}

void USRPInput::suspendRxBuddies()
{
    const std::vector<DeviceAPI*>& sourceBuddies = m_deviceAPI->getSourceBuddies();
    std::vector<DeviceAPI*>::const_iterator itSource = sourceBuddies.begin();

    qDebug("USRPInput::suspendRxBuddies (%lu)", sourceBuddies.size());

    for (; itSource != sourceBuddies.end(); ++itSource)
    {
        DeviceUSRPShared *buddySharedPtr = (DeviceUSRPShared *) (*itSource)->getBuddySharedPtr();

        if (buddySharedPtr->m_thread && buddySharedPtr->m_thread->isRunning())
        {
            buddySharedPtr->m_thread->stopWork();
            buddySharedPtr->m_threadWasRunning = true;
        }
        else
        {
            buddySharedPtr->m_threadWasRunning = false;
        }
    }
}

void USRPInput::suspendTxBuddies()
{
    const std::vector<DeviceAPI*>& sinkBuddies = m_deviceAPI->getSinkBuddies();
    std::vector<DeviceAPI*>::const_iterator itSink = sinkBuddies.begin();

    qDebug("USRPInput::suspendTxBuddies (%lu)", sinkBuddies.size());

    for (; itSink != sinkBuddies.end(); ++itSink)
    {
        DeviceUSRPShared *buddySharedPtr = (DeviceUSRPShared *) (*itSink)->getBuddySharedPtr();

        if ((buddySharedPtr->m_thread) && buddySharedPtr->m_thread->isRunning())
        {
            buddySharedPtr->m_thread->stopWork();
            buddySharedPtr->m_threadWasRunning = true;
        }
        else
        {
            buddySharedPtr->m_threadWasRunning = false;
        }
    }
}

void USRPInput::resumeRxBuddies()
{
    const std::vector<DeviceAPI*>& sourceBuddies = m_deviceAPI->getSourceBuddies();
    std::vector<DeviceAPI*>::const_iterator itSource = sourceBuddies.begin();

    qDebug("USRPInput::resumeRxBuddies (%lu)", sourceBuddies.size());

    for (; itSource != sourceBuddies.end(); ++itSource)
    {
        DeviceUSRPShared *buddySharedPtr = (DeviceUSRPShared *) (*itSource)->getBuddySharedPtr();

        if (buddySharedPtr->m_threadWasRunning) {
            buddySharedPtr->m_thread->startWork();
        }
    }
}

void USRPInput::resumeTxBuddies()
{
    const std::vector<DeviceAPI*>& sinkBuddies = m_deviceAPI->getSinkBuddies();
    std::vector<DeviceAPI*>::const_iterator itSink = sinkBuddies.begin();

    qDebug("USRPInput::resumeTxBuddies (%lu)", sinkBuddies.size());

    for (; itSink != sinkBuddies.end(); ++itSink)
    {
        DeviceUSRPShared *buddySharedPtr = (DeviceUSRPShared *) (*itSink)->getBuddySharedPtr();

        if (buddySharedPtr->m_threadWasRunning) {
            buddySharedPtr->m_thread->startWork();
        }
    }
}

void USRPInput::closeDevice()
{
    if (m_deviceShared.m_deviceParams->getDevice() == 0) { // was never open
        return;
    }

    if (m_running) { stop(); }

    m_deviceShared.m_channel = -1;

    // No buddies so effectively close the device

    if ((m_deviceAPI->getSinkBuddies().size() == 0) && (m_deviceAPI->getSourceBuddies().size() == 0))
    {
        m_deviceShared.m_deviceParams->close();
        delete m_deviceShared.m_deviceParams;
        m_deviceShared.m_deviceParams = 0;
    }
}

bool USRPInput::acquireChannel()
{
    suspendRxBuddies();
    suspendTxBuddies();

    try
    {
        // set up the stream
        std::string cpu_format("sc16");
        std::string wire_format("sc16");
        std::vector<size_t> channel_nums;
        channel_nums.push_back(m_deviceShared.m_channel);

        uhd::stream_args_t stream_args(cpu_format, wire_format);
        stream_args.channels = channel_nums;

        m_streamId = m_deviceShared.m_deviceParams->getDevice()->get_rx_stream(stream_args);
    }
    catch (std::exception& e)
    {
        qDebug() << "USRPInput::acquireChannel: exception: " << e.what();
    }

    resumeTxBuddies();
    resumeRxBuddies();

    m_channelAcquired = true;

    return true;
}

void USRPInput::releaseChannel()
{
    suspendRxBuddies();
    suspendTxBuddies();

    // destroy the stream - FIXME: Better way to do this?
    m_streamId = nullptr;

    resumeTxBuddies();
    resumeRxBuddies();

    // The channel will be effectively released to be reused in another device set only at close time

    m_channelAcquired = false;
}

void USRPInput::init()
{
    applySettings(m_settings, true);
}

bool USRPInput::start()
{
    if (!m_deviceShared.m_deviceParams->getDevice()) {
        return false;
    }

    if (m_running) { stop(); }

    if (!acquireChannel())
    {
        return false;
    }

    // start / stop streaming is done in the thread.

    m_usrpInputThread = new USRPInputThread(m_streamId, &m_sampleFifo);
    qDebug("USRPInput::start: thread created");

    applySettings(m_settings, true);

    m_usrpInputThread->setLog2Decimation(m_settings.m_log2SoftDecim);
    m_usrpInputThread->startWork();

    m_deviceShared.m_thread = m_usrpInputThread;
    m_running = true;

    return true;
}

void USRPInput::stop()
{
    qDebug("USRPInput::stop");

    if (m_usrpInputThread)
    {
        m_usrpInputThread->stopWork();
        delete m_usrpInputThread;
        m_usrpInputThread = nullptr;
    }

    m_deviceShared.m_thread = 0;
    m_running = false;

    releaseChannel();
}

QByteArray USRPInput::serialize() const
{
    return m_settings.serialize();
}

bool USRPInput::deserialize(const QByteArray& data)
{
    bool success = true;

    if (!m_settings.deserialize(data))
    {
        m_settings.resetToDefaults();
        success = false;
    }

    MsgConfigureUSRP* message = MsgConfigureUSRP::create(m_settings, true);
    m_inputMessageQueue.push(message);

    if (m_guiMessageQueue)
    {
        MsgConfigureUSRP* messageToGUI = MsgConfigureUSRP::create(m_settings, true);
        m_guiMessageQueue->push(messageToGUI);
    }

    return success;
}

const QString& USRPInput::getDeviceDescription() const
{
    return m_deviceDescription;
}

int USRPInput::getSampleRate() const
{
    int rate = m_settings.m_devSampleRate;
    return (rate / (1<<m_settings.m_log2SoftDecim));
}

quint64 USRPInput::getCenterFrequency() const
{
    return m_settings.m_centerFrequency;
}

void USRPInput::setCenterFrequency(qint64 centerFrequency)
{
    USRPInputSettings settings = m_settings;
    settings.m_centerFrequency = centerFrequency;

    MsgConfigureUSRP* message = MsgConfigureUSRP::create(settings, false);
    m_inputMessageQueue.push(message);

    if (m_guiMessageQueue)
    {
        MsgConfigureUSRP* messageToGUI = MsgConfigureUSRP::create(settings, false);
        m_guiMessageQueue->push(messageToGUI);
    }
}

std::size_t USRPInput::getChannelIndex()
{
    return m_deviceShared.m_channel;
}

void USRPInput::getLORange(float& minF, float& maxF) const
{
    minF = m_deviceShared.m_deviceParams->m_loRangeRx.start();
    maxF = m_deviceShared.m_deviceParams->m_loRangeRx.stop();
}

void USRPInput::getSRRange(float& minF, float& maxF) const
{
    minF = m_deviceShared.m_deviceParams->m_srRangeRx.start();
    maxF = m_deviceShared.m_deviceParams->m_srRangeRx.stop();
}

void USRPInput::getLPRange(float& minF, float& maxF) const
{
    minF = m_deviceShared.m_deviceParams->m_lpfRangeRx.start();
    maxF = m_deviceShared.m_deviceParams->m_lpfRangeRx.stop();
}

void USRPInput::getGainRange(float& minF, float& maxF) const
{
    minF = m_deviceShared.m_deviceParams->m_gainRangeRx.start();
    maxF = m_deviceShared.m_deviceParams->m_gainRangeRx.stop();
}

QStringList USRPInput::getRxAntennas() const
{
    return m_deviceShared.m_deviceParams->m_rxAntennas;
}

QStringList USRPInput::getRxGainNames() const
{
    return m_deviceShared.m_deviceParams->m_rxGainNames;
}

QStringList USRPInput::getClockSources() const
{
    return m_deviceShared.m_deviceParams->m_clockSources;
}

bool USRPInput::handleMessage(const Message& message)
{
    if (MsgConfigureUSRP::match(message))
    {
        MsgConfigureUSRP& conf = (MsgConfigureUSRP&) message;
        qDebug() << "USRPInput::handleMessage: MsgConfigureUSRP";

        if (!applySettings(conf.getSettings(), conf.getForce()))
        {
            qDebug("USRPInput::handleMessage config error");
        }

        return true;
    }
    else if (DeviceUSRPShared::MsgReportBuddyChange::match(message))
    {
        DeviceUSRPShared::MsgReportBuddyChange& report = (DeviceUSRPShared::MsgReportBuddyChange&) message;

        if (report.getRxElseTx())
        {
            m_settings.m_devSampleRate   = report.getDevSampleRate();
            m_settings.m_centerFrequency = report.getCenterFrequency();
        }
        else if (m_running)
        {
            double host_Hz;

            host_Hz = m_deviceShared.m_deviceParams->getDevice()->get_rx_rate(m_deviceShared.m_channel);
            m_settings.m_devSampleRate = roundf(host_Hz);

            qDebug() << "USRPInput::handleMessage: MsgReportBuddyChange:"
                     << " m_devSampleRate: " << m_settings.m_devSampleRate;
        }

        DSPSignalNotification *notif = new DSPSignalNotification(
                m_settings.m_devSampleRate/(1<<m_settings.m_log2SoftDecim),
                m_settings.m_centerFrequency);
        m_deviceAPI->getDeviceEngineInputMessageQueue()->push(notif);

        if (getMessageQueueToGUI())
        {
            DeviceUSRPShared::MsgReportBuddyChange *reportToGUI = DeviceUSRPShared::MsgReportBuddyChange::create(
                    m_settings.m_devSampleRate, m_settings.m_centerFrequency, true);
            getMessageQueueToGUI()->push(reportToGUI);
        }

        return true;
    }
    else if (DeviceUSRPShared::MsgReportClockSourceChange::match(message))
    {
        DeviceUSRPShared::MsgReportClockSourceChange& report = (DeviceUSRPShared::MsgReportClockSourceChange&) message;

        m_settings.m_clockSource  = report.getClockSource();

        if (getMessageQueueToGUI())
        {
            DeviceUSRPShared::MsgReportClockSourceChange *reportToGUI = DeviceUSRPShared::MsgReportClockSourceChange::create(
                    m_settings.m_clockSource);
            getMessageQueueToGUI()->push(reportToGUI);
        }

        return true;
    }
    else if (MsgGetStreamInfo::match(message))
    {
        if (m_deviceAPI->getSamplingDeviceGUIMessageQueue())
        {
            if (m_streamId != nullptr)
            {
                bool active;
                quint32 overflows;
                quint32 timeouts;

                m_usrpInputThread->getStreamStatus(active, overflows, timeouts);
                MsgReportStreamInfo *report = MsgReportStreamInfo::create(
                        true,
                        active,
                        overflows,
                        timeouts);
                m_deviceAPI->getSamplingDeviceGUIMessageQueue()->push(report);
            }
            else
            {
                MsgReportStreamInfo *report = MsgReportStreamInfo::create(false, false, 0, 0);
                m_deviceAPI->getSamplingDeviceGUIMessageQueue()->push(report);
            }
        }

        return true;
    }
    else if (MsgStartStop::match(message))
    {
        MsgStartStop& cmd = (MsgStartStop&) message;
        qDebug() << "USRPInput::handleMessage: MsgStartStop: " << (cmd.getStartStop() ? "start" : "stop");

        if (cmd.getStartStop())
        {
            if (m_deviceAPI->initDeviceEngine())
            {
                m_deviceAPI->startDeviceEngine();
            }
        }
        else
        {
            m_deviceAPI->stopDeviceEngine();
        }

        if (m_settings.m_useReverseAPI) {
            webapiReverseSendStartStop(cmd.getStartStop());
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool USRPInput::applySettings(const USRPInputSettings& settings, bool force)
{
    bool forwardChangeOwnDSP = false;
    bool forwardChangeRxDSP  = false;
    bool forwardChangeAllDSP = false;
    bool forwardClockSource  = false;
    bool ownThreadWasRunning = false;
    bool reapplySomeSettings = false;
    QList<QString> reverseAPIKeys;

    try
    {
        qint64 deviceCenterFrequency = settings.m_centerFrequency;
        deviceCenterFrequency -= settings.m_transverterMode ? settings.m_transverterDeltaFrequency : 0;
        deviceCenterFrequency = deviceCenterFrequency < 0 ? 0 : deviceCenterFrequency;

        // apply settings

        if ((m_settings.m_clockSource != settings.m_clockSource) || force)
        {
            reverseAPIKeys.append("clockSource");

            if (m_deviceShared.m_deviceParams->getDevice())
            {
                try
                {
                    m_deviceShared.m_deviceParams->getDevice()->set_clock_source(settings.m_clockSource.toStdString(), 0);
                    forwardClockSource = true;
                    reapplySomeSettings = true;
                    qDebug() << "USRPInput::applySettings: clock set to " << settings.m_clockSource;
                }
                catch (std::exception &e)
                {
                    // An exception will be thrown if the clock is not detected
                    // however, get_clock_source called below will still say the clock has is set
                    qCritical() << "USRPInput::applySettings: could not set clock " << settings.m_clockSource;
                    // So, default back to internal
                    m_deviceShared.m_deviceParams->getDevice()->set_clock_source("internal", 0);
                    // notify GUI that source couldn't be set
                    forwardClockSource = true;
                }
            }
            else
            {
                qCritical() << "USRPInput::applySettings: could not set clock " << settings.m_clockSource;
            }
        }

        if ((m_settings.m_devSampleRate != settings.m_devSampleRate) || force)
        {
            reverseAPIKeys.append("devSampleRate");
            forwardChangeAllDSP = true;

            if (m_deviceShared.m_deviceParams->getDevice() && m_channelAcquired)
            {
                m_deviceShared.m_deviceParams->getDevice()->set_rx_rate(settings.m_devSampleRate, m_deviceShared.m_channel);
                double actualSampleRate = m_deviceShared.m_deviceParams->getDevice()->get_rx_rate(m_deviceShared.m_channel);
                qDebug("USRPInput::applySettings: set sample rate set to %d - actual rate %f", settings.m_devSampleRate,
                        actualSampleRate);
                m_deviceShared.m_deviceParams->m_sampleRate = m_settings.m_devSampleRate;
                reapplySomeSettings = true;
            }
        }

        if ((m_settings.m_centerFrequency != settings.m_centerFrequency)
            || (m_settings.m_transverterMode != settings.m_transverterMode)
            || (m_settings.m_transverterDeltaFrequency != settings.m_transverterDeltaFrequency)
            || force)
        {
            reverseAPIKeys.append("centerFrequency");
            reverseAPIKeys.append("transverterMode");
            reverseAPIKeys.append("transverterDeltaFrequency");
            forwardChangeRxDSP = true;

            if (m_deviceShared.m_deviceParams->getDevice() && m_channelAcquired)
            {
                uhd::tune_request_t tune_request(deviceCenterFrequency);
                m_deviceShared.m_deviceParams->getDevice()->set_rx_freq(tune_request, m_deviceShared.m_channel);
                m_deviceShared.m_centerFrequency = deviceCenterFrequency; // for buddies
                qDebug("USRPInput::applySettings: frequency set to %lld", deviceCenterFrequency);
            }
        }

        if ((m_settings.m_dcBlock != settings.m_dcBlock) || force)
        {
            reverseAPIKeys.append("dcBlock");
            if (m_deviceShared.m_deviceParams->getDevice())
                m_deviceShared.m_deviceParams->getDevice()->set_rx_dc_offset(settings.m_dcBlock, m_deviceShared.m_channel);
        }

        if ((m_settings.m_iqCorrection != settings.m_iqCorrection) || force)
        {
            reverseAPIKeys.append("iqCorrection");
            if (m_deviceShared.m_deviceParams->getDevice())
                m_deviceShared.m_deviceParams->getDevice()->set_rx_iq_balance(settings.m_iqCorrection, m_deviceShared.m_channel);
        }

        if ((m_settings.m_gainMode != settings.m_gainMode) || force)
        {
            reverseAPIKeys.append("gainMode");

            if (m_deviceShared.m_deviceParams->getDevice() && m_channelAcquired)
            {
                if (settings.m_gainMode == USRPInputSettings::GAIN_AUTO)
                {
                    m_deviceShared.m_deviceParams->getDevice()->set_rx_agc(true, m_deviceShared.m_channel);
                    qDebug() << "USRPInput::applySettings: AGC enabled for channel " << m_deviceShared.m_channel;
                }
                else
                {
                    m_deviceShared.m_deviceParams->getDevice()->set_rx_agc(false, m_deviceShared.m_channel);
                    m_deviceShared.m_deviceParams->getDevice()->set_rx_gain(settings.m_gain, m_deviceShared.m_channel);
                    qDebug() << "USRPInput::applySettings: AGC disabled for channel " << m_deviceShared.m_channel << " set to " << settings.m_gain;
                }
            }
        }

        if ((m_settings.m_gain != settings.m_gain) || force)
        {
            reverseAPIKeys.append("gain");

            if ((settings.m_gainMode != USRPInputSettings::GAIN_AUTO) && m_deviceShared.m_deviceParams->getDevice() && m_channelAcquired)
            {
                m_deviceShared.m_deviceParams->getDevice()->set_rx_gain(settings.m_gain, m_deviceShared.m_channel);
                qDebug() << "USRPInput::applySettings: Gain set to " << settings.m_gain << " for channel " << m_deviceShared.m_channel;
            }
        }

        if ((m_settings.m_lpfBW != settings.m_lpfBW) || force)
        {
            reverseAPIKeys.append("lpfBW");
            m_deviceShared.m_deviceParams->getDevice()->set_rx_bandwidth(settings.m_lpfBW, m_deviceShared.m_channel);
            qDebug("USRPOutput::applySettings: LPF BW: %f for channel %d", settings.m_lpfBW, m_deviceShared.m_channel);
        }

        if ((m_settings.m_log2SoftDecim != settings.m_log2SoftDecim) || force)
        {
            reverseAPIKeys.append("log2SoftDecim");
            forwardChangeOwnDSP = true;
            m_deviceShared.m_log2Soft = settings.m_log2SoftDecim; // for buddies

            if (m_usrpInputThread)
            {
                m_usrpInputThread->setLog2Decimation(settings.m_log2SoftDecim);
                qDebug() << "USRPInput::applySettings: set soft decimation to " << (1<<settings.m_log2SoftDecim);
            }
        }

        if ((m_settings.m_antennaPath != settings.m_antennaPath) || force)
        {
            reverseAPIKeys.append("antennaPath");

            if (m_deviceShared.m_deviceParams->getDevice() && m_channelAcquired)
            {
                m_deviceShared.m_deviceParams->getDevice()->set_rx_antenna(settings.m_antennaPath.toStdString(), m_deviceShared.m_channel);
                qDebug("USRPInput::applySettings: set antenna path to %s on channel %d", qPrintable(settings.m_antennaPath), m_deviceShared.m_channel);
            }
        }

        if (settings.m_useReverseAPI)
        {
            bool fullUpdate = ((m_settings.m_useReverseAPI != settings.m_useReverseAPI) && settings.m_useReverseAPI) ||
                    (m_settings.m_reverseAPIAddress != settings.m_reverseAPIAddress) ||
                    (m_settings.m_reverseAPIPort != settings.m_reverseAPIPort) ||
                    (m_settings.m_reverseAPIDeviceIndex != settings.m_reverseAPIDeviceIndex);
            webapiReverseSendSettings(reverseAPIKeys, settings, fullUpdate || force);
        }

        if (reapplySomeSettings)
        {
            // Need to re-set bandwidth and AGG after changing samplerate (and possibly clock source)
            m_deviceShared.m_deviceParams->getDevice()->set_rx_bandwidth(settings.m_lpfBW, m_deviceShared.m_channel);
            if (settings.m_gainMode == USRPInputSettings::GAIN_AUTO)
                m_deviceShared.m_deviceParams->getDevice()->set_rx_agc(true, m_deviceShared.m_channel);
            else
            {
                m_deviceShared.m_deviceParams->getDevice()->set_rx_agc(false, m_deviceShared.m_channel);
                m_deviceShared.m_deviceParams->getDevice()->set_rx_gain(settings.m_gain, m_deviceShared.m_channel);
            }
        }

        m_settings = settings;

        // forward changes to buddies or oneself

        if (forwardChangeAllDSP)
        {
            qDebug("USRPInput::applySettings: forward change to all buddies");

            // send to self first
            DSPSignalNotification *notif = new DSPSignalNotification(
                    m_settings.m_devSampleRate/(1<<m_settings.m_log2SoftDecim),
                    m_settings.m_centerFrequency);
            m_deviceAPI->getDeviceEngineInputMessageQueue()->push(notif);

            // send to source buddies
            const std::vector<DeviceAPI*>& sourceBuddies = m_deviceAPI->getSourceBuddies();
            std::vector<DeviceAPI*>::const_iterator itSource = sourceBuddies.begin();

            for (; itSource != sourceBuddies.end(); ++itSource)
            {
                DeviceUSRPShared::MsgReportBuddyChange *report = DeviceUSRPShared::MsgReportBuddyChange::create(
                        m_settings.m_devSampleRate, m_settings.m_centerFrequency, true);
                (*itSource)->getSamplingDeviceInputMessageQueue()->push(report);
            }

            // send to sink buddies
            const std::vector<DeviceAPI*>& sinkBuddies = m_deviceAPI->getSinkBuddies();
            std::vector<DeviceAPI*>::const_iterator itSink = sinkBuddies.begin();

            for (; itSink != sinkBuddies.end(); ++itSink)
            {
                DeviceUSRPShared::MsgReportBuddyChange *report = DeviceUSRPShared::MsgReportBuddyChange::create(
                        m_settings.m_devSampleRate, m_settings.m_centerFrequency, true);
                (*itSink)->getSamplingDeviceInputMessageQueue()->push(report);
            }
        }
        else if (forwardChangeRxDSP)
        {
            qDebug("USRPInput::applySettings: forward change to Rx buddies");

            int sampleRate = m_settings.m_devSampleRate/(1<<m_settings.m_log2SoftDecim);

            // send to self first
            DSPSignalNotification *notif = new DSPSignalNotification(sampleRate, m_settings.m_centerFrequency);
            m_deviceAPI->getDeviceEngineInputMessageQueue()->push(notif);

            // send to source buddies
            const std::vector<DeviceAPI*>& sourceBuddies = m_deviceAPI->getSourceBuddies();
            std::vector<DeviceAPI*>::const_iterator itSource = sourceBuddies.begin();

            for (; itSource != sourceBuddies.end(); ++itSource)
            {
                DeviceUSRPShared::MsgReportBuddyChange *report = DeviceUSRPShared::MsgReportBuddyChange::create(
                        m_settings.m_devSampleRate, m_settings.m_centerFrequency, true);
                (*itSource)->getSamplingDeviceInputMessageQueue()->push(report);
            }
        }
        else if (forwardChangeOwnDSP)
        {
            qDebug("USRPInput::applySettings: forward change to self only");

            int sampleRate = m_settings.m_devSampleRate/(1<<m_settings.m_log2SoftDecim);
            DSPSignalNotification *notif = new DSPSignalNotification(sampleRate, m_settings.m_centerFrequency);
            m_deviceAPI->getDeviceEngineInputMessageQueue()->push(notif);
        }

        if (forwardClockSource)
        {
            // get what clock is actually set, in case requested clock couldn't be set
            if (m_deviceShared.m_deviceParams->getDevice())
            {
                try
                {
                    m_settings.m_clockSource = QString::fromStdString(m_deviceShared.m_deviceParams->getDevice()->get_clock_source(0));
                    qDebug() << "USRPInput::applySettings: clock source is " << m_settings.m_clockSource;
                }
                catch (std::exception &e)
                {
                    qDebug() << "USRPInput::applySettings: could not get clock source";
                }
            }

            // send to GUI in case requested clock isn't detected
            if (m_deviceAPI->getSamplingDeviceGUIMessageQueue())
            {
                DeviceUSRPShared::MsgReportClockSourceChange *report = DeviceUSRPShared::MsgReportClockSourceChange::create(
                        m_settings.m_clockSource);
                m_deviceAPI->getSamplingDeviceGUIMessageQueue()->push(report);
            }

            // send to source buddies
            const std::vector<DeviceAPI*>& sourceBuddies = m_deviceAPI->getSourceBuddies();
            std::vector<DeviceAPI*>::const_iterator itSource = sourceBuddies.begin();

            for (; itSource != sourceBuddies.end(); ++itSource)
            {
                DeviceUSRPShared::MsgReportClockSourceChange *report = DeviceUSRPShared::MsgReportClockSourceChange::create(
                        m_settings.m_clockSource);
                (*itSource)->getSamplingDeviceInputMessageQueue()->push(report);
            }

            // send to sink buddies
            const std::vector<DeviceAPI*>& sinkBuddies = m_deviceAPI->getSinkBuddies();
            std::vector<DeviceAPI*>::const_iterator itSink = sinkBuddies.begin();

            for (; itSink != sinkBuddies.end(); ++itSink)
            {
                DeviceUSRPShared::MsgReportClockSourceChange *report = DeviceUSRPShared::MsgReportClockSourceChange::create(
                        m_settings.m_clockSource);
                (*itSink)->getSamplingDeviceInputMessageQueue()->push(report);
            }
        }

        QLocale loc;

        qDebug().noquote() << "USRPInput::applySettings: center freq: " << m_settings.m_centerFrequency << " Hz"
                << " m_transverterMode: " << m_settings.m_transverterMode
                << " m_transverterDeltaFrequency: " << m_settings.m_transverterDeltaFrequency
                << " deviceCenterFrequency: " << deviceCenterFrequency
                << " device stream sample rate: " << loc.toString(m_settings.m_devSampleRate) << "S/s"
                << " sample rate with soft decimation: " << loc.toString( m_settings.m_devSampleRate/(1<<m_settings.m_log2SoftDecim)) << "S/s"
                << " m_log2SoftDecim: " << m_settings.m_log2SoftDecim
                << " m_gain: " << m_settings.m_gain
                << " m_lpfBW: " << loc.toString(static_cast<int>(m_settings.m_lpfBW))
                << " m_antennaPath: " << m_settings.m_antennaPath
                << " m_clockSource: " << m_settings.m_clockSource
                << " force: " << force;

        return true;
    }
    catch (std::exception &e)
    {
        qDebug() << "USRPInput::applySettings: exception: " << e.what();
        return false;
    }
}

int USRPInput::webapiSettingsGet(
                SWGSDRangel::SWGDeviceSettings& response,
                QString& errorMessage)
{
    (void) errorMessage;
    response.setUsrpInputSettings(new SWGSDRangel::SWGUSRPInputSettings());
    response.getUsrpInputSettings()->init();
    webapiFormatDeviceSettings(response, m_settings);
    return 200;
}

int USRPInput::webapiSettingsPutPatch(
                bool force,
                const QStringList& deviceSettingsKeys,
                SWGSDRangel::SWGDeviceSettings& response, // query + response
                QString& errorMessage)
{
    (void) errorMessage;
    USRPInputSettings settings = m_settings;
    webapiUpdateDeviceSettings(settings, deviceSettingsKeys, response);

    MsgConfigureUSRP *msg = MsgConfigureUSRP::create(settings, force);
    m_inputMessageQueue.push(msg);

    if (m_guiMessageQueue) // forward to GUI if any
    {
        MsgConfigureUSRP *msgToGUI = MsgConfigureUSRP::create(settings, force);
        m_guiMessageQueue->push(msgToGUI);
    }

    webapiFormatDeviceSettings(response, settings);
    return 200;
}

void USRPInput::webapiUpdateDeviceSettings(
        USRPInputSettings& settings,
        const QStringList& deviceSettingsKeys,
        SWGSDRangel::SWGDeviceSettings& response)
{
    if (deviceSettingsKeys.contains("antennaPath")) {
        settings.m_antennaPath = *response.getUsrpInputSettings()->getAntennaPath();
    }
    if (deviceSettingsKeys.contains("centerFrequency")) {
        settings.m_centerFrequency = response.getUsrpInputSettings()->getCenterFrequency();
    }
    if (deviceSettingsKeys.contains("dcBlock")) {
        settings.m_dcBlock = response.getUsrpInputSettings()->getDcBlock() != 0;
    }
    if (deviceSettingsKeys.contains("devSampleRate")) {
        settings.m_devSampleRate = response.getUsrpInputSettings()->getDevSampleRate();
    }
    if (deviceSettingsKeys.contains("clockSource")) {
        settings.m_clockSource = *response.getUsrpInputSettings()->getClockSource();
    }
    if (deviceSettingsKeys.contains("gain")) {
        settings.m_gain = response.getUsrpInputSettings()->getGain();
    }
    if (deviceSettingsKeys.contains("gainMode")) {
        settings.m_gainMode = (USRPInputSettings::GainMode) response.getUsrpInputSettings()->getGainMode();
    }
    if (deviceSettingsKeys.contains("iqCorrection")) {
        settings.m_iqCorrection = response.getUsrpInputSettings()->getIqCorrection() != 0;
    }
    if (deviceSettingsKeys.contains("log2SoftDecim")) {
        settings.m_log2SoftDecim = response.getUsrpInputSettings()->getLog2SoftDecim();
    }
    if (deviceSettingsKeys.contains("lpfBW")) {
        settings.m_lpfBW = response.getUsrpInputSettings()->getLpfBw();
    }
    if (deviceSettingsKeys.contains("transverterDeltaFrequency")) {
        settings.m_transverterDeltaFrequency = response.getUsrpInputSettings()->getTransverterDeltaFrequency();
    }
    if (deviceSettingsKeys.contains("transverterMode")) {
        settings.m_transverterMode = response.getUsrpInputSettings()->getTransverterMode() != 0;
    }
    if (deviceSettingsKeys.contains("useReverseAPI")) {
        settings.m_useReverseAPI = response.getUsrpInputSettings()->getUseReverseApi() != 0;
    }
    if (deviceSettingsKeys.contains("reverseAPIAddress")) {
        settings.m_reverseAPIAddress = *response.getUsrpInputSettings()->getReverseApiAddress();
    }
    if (deviceSettingsKeys.contains("reverseAPIPort")) {
        settings.m_reverseAPIPort = response.getUsrpInputSettings()->getReverseApiPort();
    }
    if (deviceSettingsKeys.contains("reverseAPIDeviceIndex")) {
        settings.m_reverseAPIDeviceIndex = response.getUsrpInputSettings()->getReverseApiDeviceIndex();
    }
}

void USRPInput::webapiFormatDeviceSettings(SWGSDRangel::SWGDeviceSettings& response, const USRPInputSettings& settings)
{
    response.getUsrpInputSettings()->setAntennaPath(new QString(settings.m_antennaPath));
    response.getUsrpInputSettings()->setCenterFrequency(settings.m_centerFrequency);
    response.getUsrpInputSettings()->setDcBlock(settings.m_dcBlock ? 1 : 0);
    response.getUsrpInputSettings()->setDevSampleRate(settings.m_devSampleRate);
    response.getUsrpInputSettings()->setClockSource(new QString(settings.m_clockSource));
    response.getUsrpInputSettings()->setGain(settings.m_gain);
    response.getUsrpInputSettings()->setGainMode((int) settings.m_gainMode);
    response.getUsrpInputSettings()->setIqCorrection(settings.m_iqCorrection ? 1 : 0);
    response.getUsrpInputSettings()->setLog2SoftDecim(settings.m_log2SoftDecim);
    response.getUsrpInputSettings()->setLpfBw(settings.m_lpfBW);
    response.getUsrpInputSettings()->setTransverterDeltaFrequency(settings.m_transverterDeltaFrequency);
    response.getUsrpInputSettings()->setTransverterMode(settings.m_transverterMode ? 1 : 0);
    response.getUsrpInputSettings()->setUseReverseApi(settings.m_useReverseAPI ? 1 : 0);

    if (response.getUsrpInputSettings()->getReverseApiAddress()) {
        *response.getUsrpInputSettings()->getReverseApiAddress() = settings.m_reverseAPIAddress;
    } else {
        response.getUsrpInputSettings()->setReverseApiAddress(new QString(settings.m_reverseAPIAddress));
    }

    response.getUsrpInputSettings()->setReverseApiPort(settings.m_reverseAPIPort);
    response.getUsrpInputSettings()->setReverseApiDeviceIndex(settings.m_reverseAPIDeviceIndex);
}

int USRPInput::webapiReportGet(
        SWGSDRangel::SWGDeviceReport& response,
        QString& errorMessage)
{
    (void) errorMessage;
    response.setUsrpInputReport(new SWGSDRangel::SWGUSRPInputReport());
    response.getUsrpInputReport()->init();
    webapiFormatDeviceReport(response);
    return 200;
}

int USRPInput::webapiRunGet(
        SWGSDRangel::SWGDeviceState& response,
        QString& errorMessage)
{
    (void) errorMessage;
    m_deviceAPI->getDeviceEngineStateStr(*response.getState());
    return 200;
}

int USRPInput::webapiRun(
        bool run,
        SWGSDRangel::SWGDeviceState& response,
        QString& errorMessage)
{
    (void) errorMessage;
    m_deviceAPI->getDeviceEngineStateStr(*response.getState());
    MsgStartStop *message = MsgStartStop::create(run);
    m_inputMessageQueue.push(message);

    if (m_guiMessageQueue) // forward to GUI if any
    {
        MsgStartStop *msgToGUI = MsgStartStop::create(run);
        m_guiMessageQueue->push(msgToGUI);
    }

    return 200;
}

void USRPInput::webapiFormatDeviceReport(SWGSDRangel::SWGDeviceReport& response)
{
    bool success = false;
    double temp = 0.0;
    bool active = false;
    quint32 overflows = 0;
    quint32 timeouts = 0;

    if (m_streamId != nullptr)
    {
        m_usrpInputThread->getStreamStatus(active, overflows, timeouts);
        success = true;
    }

    response.getUsrpInputReport()->setSuccess(success ? 1 : 0);
    response.getUsrpInputReport()->setStreamActive(active ? 1 : 0);
    response.getUsrpInputReport()->setOverrunCount(overflows);
    response.getUsrpInputReport()->setTimeoutCount(timeouts);
}

void USRPInput::webapiReverseSendSettings(QList<QString>& deviceSettingsKeys, const USRPInputSettings& settings, bool force)
{
    SWGSDRangel::SWGDeviceSettings *swgDeviceSettings = new SWGSDRangel::SWGDeviceSettings();
    swgDeviceSettings->setDirection(0); // single Rx
    swgDeviceSettings->setOriginatorIndex(m_deviceAPI->getDeviceSetIndex());
    swgDeviceSettings->setDeviceHwType(new QString("USRP"));
    swgDeviceSettings->setUsrpInputSettings(new SWGSDRangel::SWGUSRPInputSettings());
    SWGSDRangel::SWGUSRPInputSettings *swgUsrpInputSettings = swgDeviceSettings->getUsrpInputSettings();

    // transfer data that has been modified. When force is on transfer all data except reverse API data

    if (deviceSettingsKeys.contains("antennaPath") || force) {
        swgUsrpInputSettings->setAntennaPath(new QString(settings.m_antennaPath));
    }
    if (deviceSettingsKeys.contains("centerFrequency") || force) {
        swgUsrpInputSettings->setCenterFrequency(settings.m_centerFrequency);
    }
    if (deviceSettingsKeys.contains("dcBlock") || force) {
        swgUsrpInputSettings->setDcBlock(settings.m_dcBlock ? 1 : 0);
    }
    if (deviceSettingsKeys.contains("devSampleRate") || force) {
        swgUsrpInputSettings->setDevSampleRate(settings.m_devSampleRate);
    }
    if (deviceSettingsKeys.contains("clockSource") || force) {
        swgUsrpInputSettings->setClockSource(new QString(settings.m_clockSource));
    }
    if (deviceSettingsKeys.contains("gain") || force) {
        swgUsrpInputSettings->setGain(settings.m_gain);
    }
    if (deviceSettingsKeys.contains("gainMode") || force) {
        swgUsrpInputSettings->setGainMode((int) settings.m_gainMode);
    }
    if (deviceSettingsKeys.contains("iqCorrection") || force) {
        swgUsrpInputSettings->setIqCorrection(settings.m_iqCorrection ? 1 : 0);
    }
    if (deviceSettingsKeys.contains("log2SoftDecim") || force) {
        swgUsrpInputSettings->setLog2SoftDecim(settings.m_log2SoftDecim);
    }
    if (deviceSettingsKeys.contains("lpfBW") || force) {
        swgUsrpInputSettings->setLpfBw(settings.m_lpfBW);
    }
    if (deviceSettingsKeys.contains("transverterDeltaFrequency") || force) {
        swgUsrpInputSettings->setTransverterDeltaFrequency(settings.m_transverterDeltaFrequency);
    }
    if (deviceSettingsKeys.contains("transverterMode") || force) {
        swgUsrpInputSettings->setTransverterMode(settings.m_transverterMode ? 1 : 0);
    }

    QString deviceSettingsURL = QString("http://%1:%2/sdrangel/deviceset/%3/device/settings")
            .arg(settings.m_reverseAPIAddress)
            .arg(settings.m_reverseAPIPort)
            .arg(settings.m_reverseAPIDeviceIndex);
    m_networkRequest.setUrl(QUrl(deviceSettingsURL));
    m_networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QBuffer *buffer = new QBuffer();
    buffer->open((QBuffer::ReadWrite));
    buffer->write(swgDeviceSettings->asJson().toUtf8());
    buffer->seek(0);

    // Always use PATCH to avoid passing reverse API settings
    QNetworkReply *reply = m_networkManager->sendCustomRequest(m_networkRequest, "PATCH", buffer);
    buffer->setParent(reply);

    delete swgDeviceSettings;
}

void USRPInput::webapiReverseSendStartStop(bool start)
{
    SWGSDRangel::SWGDeviceSettings *swgDeviceSettings = new SWGSDRangel::SWGDeviceSettings();
    swgDeviceSettings->setDirection(0); // single Rx
    swgDeviceSettings->setOriginatorIndex(m_deviceAPI->getDeviceSetIndex());
    swgDeviceSettings->setDeviceHwType(new QString("USRP"));

    QString deviceSettingsURL = QString("http://%1:%2/sdrangel/deviceset/%3/device/run")
            .arg(m_settings.m_reverseAPIAddress)
            .arg(m_settings.m_reverseAPIPort)
            .arg(m_settings.m_reverseAPIDeviceIndex);
    m_networkRequest.setUrl(QUrl(deviceSettingsURL));
    m_networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QBuffer *buffer = new QBuffer();
    buffer->open((QBuffer::ReadWrite));
    buffer->write(swgDeviceSettings->asJson().toUtf8());
    buffer->seek(0);
    QNetworkReply *reply;

    if (start) {
        reply = m_networkManager->sendCustomRequest(m_networkRequest, "POST", buffer);
    } else {
        reply = m_networkManager->sendCustomRequest(m_networkRequest, "DELETE", buffer);
    }

    buffer->setParent(reply);
    delete swgDeviceSettings;
}

void USRPInput::networkManagerFinished(QNetworkReply *reply)
{
    QNetworkReply::NetworkError replyError = reply->error();

    if (replyError)
    {
        qWarning() << "USRPInput::networkManagerFinished:"
                << " error(" << (int) replyError
                << "): " << replyError
                << ": " << reply->errorString();
    }
    else
    {
        QString answer = reply->readAll();
        answer.chop(1); // remove last \n
        qDebug("USRPInput::networkManagerFinished: reply:\n%s", answer.toStdString().c_str());
    }

    reply->deleteLater();
}
