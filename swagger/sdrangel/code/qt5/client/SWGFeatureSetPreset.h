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

/*
 * SWGFeatureSetPreset.h
 *
 * Represents a Feature Set Preset object
 */

#ifndef SWGFeatureSetPreset_H_
#define SWGFeatureSetPreset_H_

#include <QJsonObject>


#include "SWGFeatureConfig.h"
#include <QList>
#include <QString>

#include "SWGObject.h"
#include "export.h"

namespace SWGSDRangel {

class SWG_API SWGFeatureSetPreset: public SWGObject {
public:
    SWGFeatureSetPreset();
    SWGFeatureSetPreset(QString* json);
    virtual ~SWGFeatureSetPreset();
    void init();
    void cleanup();

    virtual QString asJson () override;
    virtual QJsonObject* asJsonObject() override;
    virtual void fromJsonObject(QJsonObject &json) override;
    virtual SWGFeatureSetPreset* fromJson(QString &jsonString) override;

    QString* getGroup();
    void setGroup(QString* group);

    QString* getDescription();
    void setDescription(QString* description);

    QList<SWGFeatureConfig*>* getFeatureConfigs();
    void setFeatureConfigs(QList<SWGFeatureConfig*>* feature_configs);


    virtual bool isSet() override;

private:
    QString* group;
    bool m_group_isSet;

    QString* description;
    bool m_description_isSet;

    QList<SWGFeatureConfig*>* feature_configs;
    bool m_feature_configs_isSet;

};

}

#endif /* SWGFeatureSetPreset_H_ */
