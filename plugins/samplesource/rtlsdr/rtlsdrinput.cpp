///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <errno.h>
#include "rtlsdrinput.h"
#include "rtlsdrthread.h"
#include "rtlsdrgui.h"
#include "util/simpleserializer.h"

MESSAGE_CLASS_DEFINITION(RTLSDRInput::MsgConfigureRTLSDR, Message)
MESSAGE_CLASS_DEFINITION(RTLSDRInput::MsgReportRTLSDR, Message)

RTLSDRInput::Settings::Settings() :
	m_gain(0),
	m_samplerate(1024000)
{
}

void RTLSDRInput::Settings::resetToDefaults()
{
	m_gain = 0;
	m_samplerate = 1024000;
	m_loPpmCorrection = 0;
	m_log2Decim = 4;
}

QByteArray RTLSDRInput::Settings::serialize() const
{
	SimpleSerializer s(1);
	s.writeS32(1, m_gain);
	s.writeS32(2, m_samplerate);
	s.writeS32(3, m_loPpmCorrection);
	s.writeU32(4, m_log2Decim);
	return s.final();
}

bool RTLSDRInput::Settings::deserialize(const QByteArray& data)
{
	SimpleDeserializer d(data);

	if(!d.isValid()) {
		resetToDefaults();
		return false;
	}

	if(d.getVersion() == 1) {
		d.readS32(1, &m_gain, 0);
		//d.readS32(2, &m_samplerate, 0);
		d.readS32(3, &m_loPpmCorrection, 0);
		d.readU32(4, &m_log2Decim, 4);
		return true;
	} else {
		resetToDefaults();
		return false;
	}
}

RTLSDRInput::RTLSDRInput(MessageQueue* msgQueueToGUI) :
	SampleSource(msgQueueToGUI),
	m_settings(),
	m_dev(NULL),
	m_rtlSDRThread(NULL),
	m_deviceDescription()
{
}

RTLSDRInput::~RTLSDRInput()
{
	stopInput();
}

bool RTLSDRInput::startInput(int device)
{
	QMutexLocker mutexLocker(&m_mutex);

	if(m_dev != NULL)
		stopInput();

	char vendor[256];
	char product[256];
	char serial[256];
	int res;
	int numberOfGains;

	if(!m_sampleFifo.setSize(96000 * 4)) {
		qCritical("Could not allocate SampleFifo");
		return false;
	}

	if((res = rtlsdr_open(&m_dev, device)) < 0) {
		qCritical("could not open RTLSDR #%d: %s", device, strerror(errno));
		return false;
	}

	vendor[0] = '\0';
	product[0] = '\0';
	serial[0] = '\0';
	if((res = rtlsdr_get_usb_strings(m_dev, vendor, product, serial)) < 0) {
		qCritical("error accessing USB device");
		goto failed;
	}
	qWarning("RTLSDRInput open: %s %s, SN: %s", vendor, product, serial);
	m_deviceDescription = QString("%1 (SN %2)").arg(product).arg(serial);

	if((res = rtlsdr_set_sample_rate(m_dev, 1024000)) < 0) {
		qCritical("could not set sample rate: 1024k S/s");
		goto failed;
	}

	if((res = rtlsdr_set_tuner_gain_mode(m_dev, 1)) < 0) {
		qCritical("error setting tuner gain mode");
		goto failed;
	}
	if((res = rtlsdr_set_agc_mode(m_dev, 0)) < 0) {
		qCritical("error setting agc mode");
		goto failed;
	}

	numberOfGains = rtlsdr_get_tuner_gains(m_dev, NULL);
	if(numberOfGains < 0) {
		qCritical("error getting number of gain values supported");
		goto failed;
	}
	m_gains.resize(numberOfGains);
	if(rtlsdr_get_tuner_gains(m_dev, &m_gains[0]) < 0) {
		qCritical("error getting gain values");
		goto failed;
	}
	if((res = rtlsdr_reset_buffer(m_dev)) < 0) {
		qCritical("could not reset USB EP buffers: %s", strerror(errno));
		goto failed;
	}

	if((m_rtlSDRThread = new RTLSDRThread(m_dev, &m_sampleFifo)) == NULL) {
		qFatal("out of memory");
		goto failed;
	}
	m_rtlSDRThread->startWork();

	mutexLocker.unlock();
	applySettings(m_generalSettings, m_settings, true);

	qDebug("RTLSDRInput: start");
	MsgReportRTLSDR::create(m_gains)->submit(m_guiMessageQueue);

	return true;

failed:
	stopInput();
	return false;
}

void RTLSDRInput::stopInput()
{
	QMutexLocker mutexLocker(&m_mutex);

	if(m_rtlSDRThread != NULL) {
		m_rtlSDRThread->stopWork();
		delete m_rtlSDRThread;
		m_rtlSDRThread = NULL;
	}
	if(m_dev != NULL) {
		rtlsdr_close(m_dev);
		m_dev = NULL;
	}
	m_deviceDescription.clear();
}

const QString& RTLSDRInput::getDeviceDescription() const
{
	return m_deviceDescription;
}

int RTLSDRInput::getSampleRate() const
{
	int rate = m_settings.m_samplerate;
	return (rate / (1<<m_settings.m_log2Decim));
	/*
	if (rate < 800000)
		return (rate / 4);
	if ((rate == 1152000) || (rate == 2048000))
		return (rate / 8);
	return (rate / 16);
	*/
}

quint64 RTLSDRInput::getCenterFrequency() const
{
	return m_generalSettings.m_centerFrequency;
}

bool RTLSDRInput::handleMessage(Message* message)
{
	if(MsgConfigureRTLSDR::match(message)) {
		MsgConfigureRTLSDR* conf = (MsgConfigureRTLSDR*)message;
		if(!applySettings(conf->getGeneralSettings(), conf->getSettings(), false))
			qDebug("RTLSDR config error");
		message->completed();
		return true;
	} else {
		return false;
	}
}

bool RTLSDRInput::applySettings(const GeneralSettings& generalSettings, const Settings& settings, bool force)
{
	QMutexLocker mutexLocker(&m_mutex);

	if((m_settings.m_gain != settings.m_gain) || force) {
		m_settings.m_gain = settings.m_gain;
		if(m_dev != NULL) {
			if(rtlsdr_set_tuner_gain(m_dev, m_settings.m_gain) != 0)
				qDebug("rtlsdr_set_tuner_gain() failed");
		}
	}

	if((m_settings.m_samplerate != settings.m_samplerate) || force) {
		if(m_dev != NULL) {
			if( rtlsdr_set_sample_rate(m_dev, settings.m_samplerate) < 0)
				qCritical("could not set sample rate: %d", settings.m_samplerate);
			else {
				m_settings.m_samplerate = settings.m_samplerate;
				m_rtlSDRThread->setSamplerate(settings.m_samplerate);
			}
		}
	}

	if((m_settings.m_loPpmCorrection != settings.m_loPpmCorrection) || force) {
		if(m_dev != NULL) {
			if( rtlsdr_set_freq_correction(m_dev, settings.m_loPpmCorrection) < 0)
				qCritical("could not set LO ppm correction: %d", settings.m_loPpmCorrection);
			else {
				m_settings.m_loPpmCorrection = settings.m_loPpmCorrection;
				//m_rtlSDRThread->setSamplerate(settings.m_samplerate);
			}
		}
	}

	if((m_settings.m_log2Decim != settings.m_log2Decim) || force) {
		if(m_dev != NULL) {
			m_settings.m_log2Decim = settings.m_log2Decim;
			m_rtlSDRThread->setLog2Decimation(settings.m_log2Decim);
		}
	}

	m_generalSettings.m_centerFrequency = generalSettings.m_centerFrequency;
	if(m_dev != NULL) {
		qint64 centerFrequency = m_generalSettings.m_centerFrequency + (m_settings.m_samplerate / 4);

		if (m_settings.m_log2Decim == 0) { // Little wooby-doop if no decimation
			centerFrequency = m_generalSettings.m_centerFrequency;
		} else {
			centerFrequency = m_generalSettings.m_centerFrequency + (m_settings.m_samplerate / 4);
		}

		if(rtlsdr_set_center_freq( m_dev, centerFrequency ) != 0)
			qDebug("osmosdr_set_center_freq(%lld) failed", m_generalSettings.m_centerFrequency);
	}

	return true;
}

void RTLSDRInput::set_ds_mode(int on)
{
	rtlsdr_set_direct_sampling(m_dev, on);
}

