/**
 * SDRangel
 * This is the web REST/JSON API of SDRangel SDR software. SDRangel is an Open Source Qt5/OpenGL 3.0+ (4.3+ in Windows) GUI and server Software Defined Radio and signal analyzer in software. It supports Airspy, BladeRF, HackRF, LimeSDR, PlutoSDR, RTL-SDR, SDRplay RSP1, USRP and FunCube    ---   Limitations and specifcities:    * In SDRangel GUI the first Rx device set cannot be deleted. Conversely the server starts with no device sets and its number of device sets can be reduced to zero by as many calls as necessary to /sdrangel/deviceset with DELETE method.   * Preset import and export from/to file is a server only feature.   * Device set focus is a GUI only feature.   * The following channels are not implemented (status 501 is returned): ATV and DATV demodulators, Channel Analyzer NG, LoRa demodulator   * The device settings and report structures contains only the sub-structure corresponding to the device type. The DeviceSettings and DeviceReport structures documented here shows all of them but only one will be or should be present at a time   * The channel settings and report structures contains only the sub-structure corresponding to the channel type. The ChannelSettings and ChannelReport structures documented here shows all of them but only one will be or should be present at a time    --- 
 *
 * OpenAPI spec version: 4.15.0
 * Contact: f4exb06@gmail.com
 *
 * NOTE: This class is auto generated by the swagger code generator program.
 * https://github.com/swagger-api/swagger-codegen.git
 * Do not edit the class manually.
 */


#include "SWGTestSourceSettings.h"

#include "SWGHelpers.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QObject>
#include <QDebug>

namespace SWGSDRangel {

SWGTestSourceSettings::SWGTestSourceSettings(QString* json) {
    init();
    this->fromJson(*json);
}

SWGTestSourceSettings::SWGTestSourceSettings() {
    center_frequency = 0L;
    m_center_frequency_isSet = false;
    frequency_shift = 0;
    m_frequency_shift_isSet = false;
    sample_rate = 0;
    m_sample_rate_isSet = false;
    log2_decim = 0;
    m_log2_decim_isSet = false;
    fc_pos = 0;
    m_fc_pos_isSet = false;
    sample_size_index = 0;
    m_sample_size_index_isSet = false;
    amplitude_bits = 0;
    m_amplitude_bits_isSet = false;
    auto_corr_options = 0;
    m_auto_corr_options_isSet = false;
    modulation = 0;
    m_modulation_isSet = false;
    modulation_tone = 0;
    m_modulation_tone_isSet = false;
    am_modulation = 0;
    m_am_modulation_isSet = false;
    fm_deviation = 0;
    m_fm_deviation_isSet = false;
    dc_factor = 0.0f;
    m_dc_factor_isSet = false;
    i_factor = 0.0f;
    m_i_factor_isSet = false;
    q_factor = 0.0f;
    m_q_factor_isSet = false;
    phase_imbalance = 0.0f;
    m_phase_imbalance_isSet = false;
    use_reverse_api = 0;
    m_use_reverse_api_isSet = false;
    reverse_api_address = nullptr;
    m_reverse_api_address_isSet = false;
    reverse_api_port = 0;
    m_reverse_api_port_isSet = false;
    reverse_api_device_index = 0;
    m_reverse_api_device_index_isSet = false;
}

SWGTestSourceSettings::~SWGTestSourceSettings() {
    this->cleanup();
}

void
SWGTestSourceSettings::init() {
    center_frequency = 0L;
    m_center_frequency_isSet = false;
    frequency_shift = 0;
    m_frequency_shift_isSet = false;
    sample_rate = 0;
    m_sample_rate_isSet = false;
    log2_decim = 0;
    m_log2_decim_isSet = false;
    fc_pos = 0;
    m_fc_pos_isSet = false;
    sample_size_index = 0;
    m_sample_size_index_isSet = false;
    amplitude_bits = 0;
    m_amplitude_bits_isSet = false;
    auto_corr_options = 0;
    m_auto_corr_options_isSet = false;
    modulation = 0;
    m_modulation_isSet = false;
    modulation_tone = 0;
    m_modulation_tone_isSet = false;
    am_modulation = 0;
    m_am_modulation_isSet = false;
    fm_deviation = 0;
    m_fm_deviation_isSet = false;
    dc_factor = 0.0f;
    m_dc_factor_isSet = false;
    i_factor = 0.0f;
    m_i_factor_isSet = false;
    q_factor = 0.0f;
    m_q_factor_isSet = false;
    phase_imbalance = 0.0f;
    m_phase_imbalance_isSet = false;
    use_reverse_api = 0;
    m_use_reverse_api_isSet = false;
    reverse_api_address = new QString("");
    m_reverse_api_address_isSet = false;
    reverse_api_port = 0;
    m_reverse_api_port_isSet = false;
    reverse_api_device_index = 0;
    m_reverse_api_device_index_isSet = false;
}

void
SWGTestSourceSettings::cleanup() {

















    if(reverse_api_address != nullptr) { 
        delete reverse_api_address;
    }


}

SWGTestSourceSettings*
SWGTestSourceSettings::fromJson(QString &json) {
    QByteArray array (json.toStdString().c_str());
    QJsonDocument doc = QJsonDocument::fromJson(array);
    QJsonObject jsonObject = doc.object();
    this->fromJsonObject(jsonObject);
    return this;
}

void
SWGTestSourceSettings::fromJsonObject(QJsonObject &pJson) {
    ::SWGSDRangel::setValue(&center_frequency, pJson["centerFrequency"], "qint64", "");
    
    ::SWGSDRangel::setValue(&frequency_shift, pJson["frequencyShift"], "qint32", "");
    
    ::SWGSDRangel::setValue(&sample_rate, pJson["sampleRate"], "qint32", "");
    
    ::SWGSDRangel::setValue(&log2_decim, pJson["log2Decim"], "qint32", "");
    
    ::SWGSDRangel::setValue(&fc_pos, pJson["fcPos"], "qint32", "");
    
    ::SWGSDRangel::setValue(&sample_size_index, pJson["sampleSizeIndex"], "qint32", "");
    
    ::SWGSDRangel::setValue(&amplitude_bits, pJson["amplitudeBits"], "qint32", "");
    
    ::SWGSDRangel::setValue(&auto_corr_options, pJson["autoCorrOptions"], "qint32", "");
    
    ::SWGSDRangel::setValue(&modulation, pJson["modulation"], "qint32", "");
    
    ::SWGSDRangel::setValue(&modulation_tone, pJson["modulationTone"], "qint32", "");
    
    ::SWGSDRangel::setValue(&am_modulation, pJson["amModulation"], "qint32", "");
    
    ::SWGSDRangel::setValue(&fm_deviation, pJson["fmDeviation"], "qint32", "");
    
    ::SWGSDRangel::setValue(&dc_factor, pJson["dcFactor"], "float", "");
    
    ::SWGSDRangel::setValue(&i_factor, pJson["iFactor"], "float", "");
    
    ::SWGSDRangel::setValue(&q_factor, pJson["qFactor"], "float", "");
    
    ::SWGSDRangel::setValue(&phase_imbalance, pJson["phaseImbalance"], "float", "");
    
    ::SWGSDRangel::setValue(&use_reverse_api, pJson["useReverseAPI"], "qint32", "");
    
    ::SWGSDRangel::setValue(&reverse_api_address, pJson["reverseAPIAddress"], "QString", "QString");
    
    ::SWGSDRangel::setValue(&reverse_api_port, pJson["reverseAPIPort"], "qint32", "");
    
    ::SWGSDRangel::setValue(&reverse_api_device_index, pJson["reverseAPIDeviceIndex"], "qint32", "");
    
}

QString
SWGTestSourceSettings::asJson ()
{
    QJsonObject* obj = this->asJsonObject();

    QJsonDocument doc(*obj);
    QByteArray bytes = doc.toJson();
    delete obj;
    return QString(bytes);
}

QJsonObject*
SWGTestSourceSettings::asJsonObject() {
    QJsonObject* obj = new QJsonObject();
    if(m_center_frequency_isSet){
        obj->insert("centerFrequency", QJsonValue(center_frequency));
    }
    if(m_frequency_shift_isSet){
        obj->insert("frequencyShift", QJsonValue(frequency_shift));
    }
    if(m_sample_rate_isSet){
        obj->insert("sampleRate", QJsonValue(sample_rate));
    }
    if(m_log2_decim_isSet){
        obj->insert("log2Decim", QJsonValue(log2_decim));
    }
    if(m_fc_pos_isSet){
        obj->insert("fcPos", QJsonValue(fc_pos));
    }
    if(m_sample_size_index_isSet){
        obj->insert("sampleSizeIndex", QJsonValue(sample_size_index));
    }
    if(m_amplitude_bits_isSet){
        obj->insert("amplitudeBits", QJsonValue(amplitude_bits));
    }
    if(m_auto_corr_options_isSet){
        obj->insert("autoCorrOptions", QJsonValue(auto_corr_options));
    }
    if(m_modulation_isSet){
        obj->insert("modulation", QJsonValue(modulation));
    }
    if(m_modulation_tone_isSet){
        obj->insert("modulationTone", QJsonValue(modulation_tone));
    }
    if(m_am_modulation_isSet){
        obj->insert("amModulation", QJsonValue(am_modulation));
    }
    if(m_fm_deviation_isSet){
        obj->insert("fmDeviation", QJsonValue(fm_deviation));
    }
    if(m_dc_factor_isSet){
        obj->insert("dcFactor", QJsonValue(dc_factor));
    }
    if(m_i_factor_isSet){
        obj->insert("iFactor", QJsonValue(i_factor));
    }
    if(m_q_factor_isSet){
        obj->insert("qFactor", QJsonValue(q_factor));
    }
    if(m_phase_imbalance_isSet){
        obj->insert("phaseImbalance", QJsonValue(phase_imbalance));
    }
    if(m_use_reverse_api_isSet){
        obj->insert("useReverseAPI", QJsonValue(use_reverse_api));
    }
    if(reverse_api_address != nullptr && *reverse_api_address != QString("")){
        toJsonValue(QString("reverseAPIAddress"), reverse_api_address, obj, QString("QString"));
    }
    if(m_reverse_api_port_isSet){
        obj->insert("reverseAPIPort", QJsonValue(reverse_api_port));
    }
    if(m_reverse_api_device_index_isSet){
        obj->insert("reverseAPIDeviceIndex", QJsonValue(reverse_api_device_index));
    }

    return obj;
}

qint64
SWGTestSourceSettings::getCenterFrequency() {
    return center_frequency;
}
void
SWGTestSourceSettings::setCenterFrequency(qint64 center_frequency) {
    this->center_frequency = center_frequency;
    this->m_center_frequency_isSet = true;
}

qint32
SWGTestSourceSettings::getFrequencyShift() {
    return frequency_shift;
}
void
SWGTestSourceSettings::setFrequencyShift(qint32 frequency_shift) {
    this->frequency_shift = frequency_shift;
    this->m_frequency_shift_isSet = true;
}

qint32
SWGTestSourceSettings::getSampleRate() {
    return sample_rate;
}
void
SWGTestSourceSettings::setSampleRate(qint32 sample_rate) {
    this->sample_rate = sample_rate;
    this->m_sample_rate_isSet = true;
}

qint32
SWGTestSourceSettings::getLog2Decim() {
    return log2_decim;
}
void
SWGTestSourceSettings::setLog2Decim(qint32 log2_decim) {
    this->log2_decim = log2_decim;
    this->m_log2_decim_isSet = true;
}

qint32
SWGTestSourceSettings::getFcPos() {
    return fc_pos;
}
void
SWGTestSourceSettings::setFcPos(qint32 fc_pos) {
    this->fc_pos = fc_pos;
    this->m_fc_pos_isSet = true;
}

qint32
SWGTestSourceSettings::getSampleSizeIndex() {
    return sample_size_index;
}
void
SWGTestSourceSettings::setSampleSizeIndex(qint32 sample_size_index) {
    this->sample_size_index = sample_size_index;
    this->m_sample_size_index_isSet = true;
}

qint32
SWGTestSourceSettings::getAmplitudeBits() {
    return amplitude_bits;
}
void
SWGTestSourceSettings::setAmplitudeBits(qint32 amplitude_bits) {
    this->amplitude_bits = amplitude_bits;
    this->m_amplitude_bits_isSet = true;
}

qint32
SWGTestSourceSettings::getAutoCorrOptions() {
    return auto_corr_options;
}
void
SWGTestSourceSettings::setAutoCorrOptions(qint32 auto_corr_options) {
    this->auto_corr_options = auto_corr_options;
    this->m_auto_corr_options_isSet = true;
}

qint32
SWGTestSourceSettings::getModulation() {
    return modulation;
}
void
SWGTestSourceSettings::setModulation(qint32 modulation) {
    this->modulation = modulation;
    this->m_modulation_isSet = true;
}

qint32
SWGTestSourceSettings::getModulationTone() {
    return modulation_tone;
}
void
SWGTestSourceSettings::setModulationTone(qint32 modulation_tone) {
    this->modulation_tone = modulation_tone;
    this->m_modulation_tone_isSet = true;
}

qint32
SWGTestSourceSettings::getAmModulation() {
    return am_modulation;
}
void
SWGTestSourceSettings::setAmModulation(qint32 am_modulation) {
    this->am_modulation = am_modulation;
    this->m_am_modulation_isSet = true;
}

qint32
SWGTestSourceSettings::getFmDeviation() {
    return fm_deviation;
}
void
SWGTestSourceSettings::setFmDeviation(qint32 fm_deviation) {
    this->fm_deviation = fm_deviation;
    this->m_fm_deviation_isSet = true;
}

float
SWGTestSourceSettings::getDcFactor() {
    return dc_factor;
}
void
SWGTestSourceSettings::setDcFactor(float dc_factor) {
    this->dc_factor = dc_factor;
    this->m_dc_factor_isSet = true;
}

float
SWGTestSourceSettings::getIFactor() {
    return i_factor;
}
void
SWGTestSourceSettings::setIFactor(float i_factor) {
    this->i_factor = i_factor;
    this->m_i_factor_isSet = true;
}

float
SWGTestSourceSettings::getQFactor() {
    return q_factor;
}
void
SWGTestSourceSettings::setQFactor(float q_factor) {
    this->q_factor = q_factor;
    this->m_q_factor_isSet = true;
}

float
SWGTestSourceSettings::getPhaseImbalance() {
    return phase_imbalance;
}
void
SWGTestSourceSettings::setPhaseImbalance(float phase_imbalance) {
    this->phase_imbalance = phase_imbalance;
    this->m_phase_imbalance_isSet = true;
}

qint32
SWGTestSourceSettings::getUseReverseApi() {
    return use_reverse_api;
}
void
SWGTestSourceSettings::setUseReverseApi(qint32 use_reverse_api) {
    this->use_reverse_api = use_reverse_api;
    this->m_use_reverse_api_isSet = true;
}

QString*
SWGTestSourceSettings::getReverseApiAddress() {
    return reverse_api_address;
}
void
SWGTestSourceSettings::setReverseApiAddress(QString* reverse_api_address) {
    this->reverse_api_address = reverse_api_address;
    this->m_reverse_api_address_isSet = true;
}

qint32
SWGTestSourceSettings::getReverseApiPort() {
    return reverse_api_port;
}
void
SWGTestSourceSettings::setReverseApiPort(qint32 reverse_api_port) {
    this->reverse_api_port = reverse_api_port;
    this->m_reverse_api_port_isSet = true;
}

qint32
SWGTestSourceSettings::getReverseApiDeviceIndex() {
    return reverse_api_device_index;
}
void
SWGTestSourceSettings::setReverseApiDeviceIndex(qint32 reverse_api_device_index) {
    this->reverse_api_device_index = reverse_api_device_index;
    this->m_reverse_api_device_index_isSet = true;
}


bool
SWGTestSourceSettings::isSet(){
    bool isObjectUpdated = false;
    do{
        if(m_center_frequency_isSet){
            isObjectUpdated = true; break;
        }
        if(m_frequency_shift_isSet){
            isObjectUpdated = true; break;
        }
        if(m_sample_rate_isSet){
            isObjectUpdated = true; break;
        }
        if(m_log2_decim_isSet){
            isObjectUpdated = true; break;
        }
        if(m_fc_pos_isSet){
            isObjectUpdated = true; break;
        }
        if(m_sample_size_index_isSet){
            isObjectUpdated = true; break;
        }
        if(m_amplitude_bits_isSet){
            isObjectUpdated = true; break;
        }
        if(m_auto_corr_options_isSet){
            isObjectUpdated = true; break;
        }
        if(m_modulation_isSet){
            isObjectUpdated = true; break;
        }
        if(m_modulation_tone_isSet){
            isObjectUpdated = true; break;
        }
        if(m_am_modulation_isSet){
            isObjectUpdated = true; break;
        }
        if(m_fm_deviation_isSet){
            isObjectUpdated = true; break;
        }
        if(m_dc_factor_isSet){
            isObjectUpdated = true; break;
        }
        if(m_i_factor_isSet){
            isObjectUpdated = true; break;
        }
        if(m_q_factor_isSet){
            isObjectUpdated = true; break;
        }
        if(m_phase_imbalance_isSet){
            isObjectUpdated = true; break;
        }
        if(m_use_reverse_api_isSet){
            isObjectUpdated = true; break;
        }
        if(reverse_api_address && *reverse_api_address != QString("")){
            isObjectUpdated = true; break;
        }
        if(m_reverse_api_port_isSet){
            isObjectUpdated = true; break;
        }
        if(m_reverse_api_device_index_isSet){
            isObjectUpdated = true; break;
        }
    }while(false);
    return isObjectUpdated;
}
}

