/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2010 by Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "systeminfo.h"
#include "rbsettings.h"

#include <QSettings>
#include "Logger.h"

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif


// device settings
const static struct {
    SystemInfo::SystemInfos info;
    const char* name;
    const char* def;
} SystemInfosList[] = {
    { SystemInfo::ManualUrl,            "manual_url",           "" },
    { SystemInfo::BleedingUrl,          "bleeding_url",         "" },
    { SystemInfo::BootloaderUrl,        "bootloader_url",       "" },
    { SystemInfo::BootloaderInfoUrl,    "bootloader_info_url",  "" },
    { SystemInfo::ReleaseFontUrl,       "release_font_url",     "" },
    { SystemInfo::DailyFontUrl,         "daily_font_url",       "" },
    { SystemInfo::DailyVoiceUrl,        "daily_voice_url",      "" },
    { SystemInfo::ReleaseVoiceUrl,      "release_voice_url",    "" },
    { SystemInfo::DoomUrl,              "doom_url",             "" },
    { SystemInfo::Duke3DUrl,            "duke3d_url",           "" },
    { SystemInfo::PuzzFontsUrl,         "puzzfonts_url",        "" },
    { SystemInfo::QuakeUrl,             "quake_url",            "" },
    { SystemInfo::Wolf3DUrl,            "wolf3d_url",           "" },
    { SystemInfo::XWorldUrl,            "xworld_url",           "" },
    { SystemInfo::ReleaseUrl,           "release_url",          "" },
    { SystemInfo::DailyUrl,             "daily_url",            "" },
    { SystemInfo::BuildInfoUrl,         "build_info_url",       "" },
    { SystemInfo::GenlangUrl,           "genlang_url",          "" },
    { SystemInfo::ThemesUrl,            "themes_url",           "" },
    { SystemInfo::ThemesInfoUrl,        "themes_info_url",      "" },
    { SystemInfo::RbutilUrl,            "rbutil_url",           "" },
    { SystemInfo::CurPlatformName,      ":platform:/name",      "" },
    { SystemInfo::CurManual,            ":platform:/manualname","rockbox-:platform:" },
    { SystemInfo::CurBootloaderMethod,  ":platform:/bootloadermethod", "none" },
    { SystemInfo::CurBootloaderName,    ":platform:/bootloadername", "" },
    { SystemInfo::CurBootloaderFile,    ":platform:/bootloaderfile", "" },
    { SystemInfo::CurBootloaderFilter,  ":platform:/bootloaderfilter", "" },
    { SystemInfo::CurEncoder,           ":platform:/encoder",   "" },
    { SystemInfo::CurBrand,             ":platform:/brand",     "" },
    { SystemInfo::CurName,              ":platform:/name",      "" },
    { SystemInfo::CurBuildserverModel,  ":platform:/buildserver_modelname", "" },
    { SystemInfo::CurConfigureModel,    ":platform:/configure_modelname", "" },
    { SystemInfo::CurPlayerPicture,     ":platform:/playerpic", "" },
};

//! pointer to setting object to NULL
QSettings* SystemInfo::systemInfos = NULL;

void SystemInfo::ensureSystemInfoExists()
{
    //check and create settings object
    if(systemInfos == NULL)
    {
        // only use built-in rbutil.ini
        systemInfos = new QSettings(":/ini/rbutil.ini", QSettings::IniFormat, 0);
    }
}


QVariant SystemInfo::value(enum SystemInfos info)
{
    ensureSystemInfoExists();

    // locate setting item
    int i = 0;
    while(SystemInfosList[i].info != info)
        i++;
    QString platform = RbSettings::value(RbSettings::CurrentPlatform).toString();
    QString s = SystemInfosList[i].name;
    s.replace(":platform:", platform);
    QString d = SystemInfosList[i].def;
    d.replace(":platform:", platform);
    LOG_INFO() << "GET:" << s << systemInfos->value(s, d).toString();
    return systemInfos->value(s, d);
}

QVariant SystemInfo::platformValue(QString platform, enum SystemInfos info)
{
    ensureSystemInfoExists();

    // locate setting item
    int i = 0;
    while(SystemInfosList[i].info != info)
        i++;

    QString s = SystemInfosList[i].name;
    s.replace(":platform:", platform);
    QString d = SystemInfosList[i].def;
    d.replace(":platform:", platform);
    LOG_INFO() << "GET P:" << s << systemInfos->value(s, d).toString();
    return systemInfos->value(s, d);
}

QStringList SystemInfo::platforms(enum SystemInfo::PlatformType type, QString variant)
{
    ensureSystemInfoExists();

    QStringList result;
    systemInfos->beginGroup("platforms");
    QStringList a = systemInfos->childKeys();
    systemInfos->endGroup();
    for(int i = 0; i < a.size(); i++)
    {
        QString target = systemInfos->value("platforms/"+a.at(i), "null").toString();
        QRegExp regex("\\..*$");
        QString targetbase = target;
        targetbase.remove(regex);
        // only add target if its not disabled unless Platform*Disabled requested
        if(type != PlatformAllDisabled && type != PlatformBaseDisabled
                && type != PlatformVariantDisabled
                && systemInfos->value(target+"/status").toString() == "disabled")
            continue;
        // report only matching target if PlatformVariant* is requested
        if((type == PlatformVariant || type == PlatformVariantDisabled)
                && (targetbase != variant))
            continue;
        // report only base targets when PlatformBase* is requested
        if((type == PlatformBase || type == PlatformBaseDisabled))
            result.append(targetbase);
        else
            result.append(target);
    }
    result.removeDuplicates();
    return result;
}

QMap<QString, QStringList> SystemInfo::languages(void)
{
    ensureSystemInfoExists();

    QMap<QString, QStringList> result;
    systemInfos->beginGroup("languages");
    QStringList a = systemInfos->childKeys();
    for(int i = 0; i < a.size(); i++)
    {
        result.insert(a.at(i), systemInfos->value(a.at(i), "null").toStringList());
    }
    systemInfos->endGroup();
    return result;
}


QMap<int, QStringList> SystemInfo::usbIdMap(enum MapType type)
{
    ensureSystemInfoExists();

    QMap<int, QStringList> map;
    // get a list of ID -> target name
    QStringList platforms;
    systemInfos->beginGroup("platforms");
    platforms = systemInfos->childKeys();
    systemInfos->endGroup();

    QString t;
    switch(type) {
        case MapDevice:
            t = "usbid";
            break;
        case MapError:
            t = "usberror";
            break;
        case MapIncompatible:
            t = "usbincompat";
            break;
    }

    for(int i = 0; i < platforms.size(); i++)
    {
        systemInfos->beginGroup("platforms");
        QString target = systemInfos->value(platforms.at(i)).toString();
        systemInfos->endGroup();
        systemInfos->beginGroup(target);
        QStringList ids = systemInfos->value(t).toStringList();
        int j = ids.size();
        while(j--) {
            QStringList l;
            int id = ids.at(j).toInt(0, 16);
            if(id == 0) {
                continue;
            }
            if(map.keys().contains(id)) {
                l = map.take(id);
            }
            l.append(target);
            map.insert(id, l);
        }
        systemInfos->endGroup();
    }
    return map;
}


