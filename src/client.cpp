/*
 *      Copyright (C) 2017 Matthew Lundberg <matthew.k.lundberg@gmail.com>
 *      https://github.com/MatthewLundberg/pvr.hdhomerun
 *
 *      Copyright (C) 2015 Zoltan Csizmadia <zcsizmadia@gmail.com>
 *      https://github.com/zcsizmadia/pvr.hdhomerun
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"
#include <xbmc_pvr_dll.h>
#include <p8-platform/util/util.h>
#include <p8-platform/threads/threads.h>
#include "HDHomeRunTuners.h"
#include "Utils.h"

using namespace ADDON;

namespace PVRHDHomeRun {

GlobalsType g;

class UpdateThread: public P8PLATFORM::CThread
{
public:
    void *Process()
    {
        for (;;)
        {
            for (int i = 0; i < 60 * 60; i++)
                if (P8PLATFORM::CThread::Sleep(1000))
                    break;

            if (IsStopped())
                break;

            if (g.lineup)
            {
                g.lineup->UpdateGuide();
                g.PVR->TriggerChannelUpdate();
            }
        }
        return nullptr;
    }
};

UpdateThread g_UpdateThread;
}; // namespace

using namespace PVRHDHomeRun;

void SetChannelName(const char* name)
{
    if (g.XBMC == nullptr)
        return;

    if (strcmp(name, "Tuner Name") == 0)
    {
        g.Settings.eChannelName = SettingsType::TUNER_NAME;
    }
    else if (strcmp(name, "Guide Name") == 0)
    {
        g.Settings.eChannelName = SettingsType::GUIDE_NAME;
    }
    else if (strcmp(name, "Affiliate") == 0)
    {
        g.Settings.eChannelName = SettingsType::AFFILIATE;
    }
}

extern "C"
{

void ADDON_ReadSettings(void)
{
    if (g.XBMC == nullptr)
        return;

    g.XBMC->GetSetting("hide_protected", &g.Settings.bHideProtected);
    g.XBMC->GetSetting("hide_duplicate", &g.Settings.bHideDuplicateChannels);
    g.XBMC->GetSetting("mark_new",       &g.Settings.bMarkNew);
    g.XBMC->GetSetting("debug",          &g.Settings.bDebug);
    g.XBMC->GetSetting("hide_unknown",   &g.Settings.bHideUnknownChannels);
    g.XBMC->GetSetting("use_legacy",     &g.Settings.bUseLegacy);

    char channel_name[64] = "Guide Name";
    g.XBMC->GetSetting("channel_name", channel_name);
    SetChannelName(channel_name);
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
    if (!hdl || !props)
        return ADDON_STATUS_UNKNOWN;

    PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*) props;

    g.XBMC = new CHelper_libXBMC_addon;
    if (!g.XBMC->RegisterMe(hdl))
    {
        SAFE_DELETE(g.XBMC);
        return ADDON_STATUS_PERMANENT_FAILURE;
    }

    g.PVR = new CHelper_libXBMC_pvr;
    if (!g.PVR->RegisterMe(hdl))
    {
        SAFE_DELETE(g.PVR);
        SAFE_DELETE(g.XBMC);
        return ADDON_STATUS_PERMANENT_FAILURE;
    }

    KODI_LOG(LOG_NOTICE, "%s - Creating the PVR HDHomeRun add-on",
            __FUNCTION__);

    g.currentStatus = ADDON_STATUS_UNKNOWN;
    g.strUserPath = pvrprops->strUserPath;
    g.strClientPath = pvrprops->strClientPath;

    ADDON_ReadSettings();

    KODI_LOG(LOG_DEBUG, "Creating new-style Lineup");
    g.lineup = new Lineup();
    if (g.lineup == nullptr)
        return ADDON_STATUS_PERMANENT_FAILURE;
    KODI_LOG(LOG_DEBUG, "Done with new-style Lineup");

    if (g.lineup)
    {
        g.lineup->Update();
        g_UpdateThread.CreateThread(false);
    }

    g.currentStatus = ADDON_STATUS_OK;
    g.bCreated = true;

    return ADDON_STATUS_OK;
}

ADDON_STATUS ADDON_GetStatus()
{
    return g.currentStatus;
}

void ADDON_Destroy()
{
    g_UpdateThread.StopThread();

    SAFE_DELETE(g.lineup);
    SAFE_DELETE(g.PVR);
    SAFE_DELETE(g.XBMC);

    g.bCreated = false;
    g.currentStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
    return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
    return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
    if (g.lineup == nullptr)
        return ADDON_STATUS_OK;

    if (strcmp(settingName, "hide_protected") == 0)
    {
        g.Settings.bHideProtected = *(bool*) settingValue;
        return ADDON_STATUS_NEED_RESTART;
    }
    else if (strcmp(settingName, "hide_duplicate") == 0)
    {
        g.Settings.bHideDuplicateChannels = *(bool*) settingValue;
        return ADDON_STATUS_NEED_RESTART;
    }
    else if (strcmp(settingName, "mark_new") == 0)
        g.Settings.bMarkNew = *(bool*) settingValue;
    else if (strcmp(settingName, "debug") == 0)
        g.Settings.bDebug = *(bool*) settingValue;
    else if (strcmp(settingName, "use_legacy") == 0)
    {
        g.Settings.bUseLegacy = *(bool*) settingValue;
        return ADDON_STATUS_NEED_RESTART;
    }
    else if (strcmp(settingName, "hide_unknown") == 0)
    {
        g.Settings.bHideUnknownChannels = *(bool*) settingValue;
        return ADDON_STATUS_NEED_RESTART;
    }
    else if (strcmp(settingName, "channel_name") == 0)
    {
        SetChannelName((char*) settingValue);
        return ADDON_STATUS_NEED_RESTART;
    }

    return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

void OnSystemSleep()
{
}

void OnSystemWake()
{
    if (g.lineup && g.PVR)
    {
        g.lineup->Update();
        g.PVR->TriggerChannelUpdate();
    }
}

void OnPowerSavingActivated()
{
}

void OnPowerSavingDeactivated()
{
}

const char* GetPVRAPIVersion(void)
{
    static const char *strApiVersion = XBMC_PVR_API_VERSION;
    return strApiVersion;
}

const char* GetMininumPVRAPIVersion(void)
{
    static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
    return strMinApiVersion;
}

const char* GetGUIAPIVersion(void)
{
    return ""; // GUI API not used
}

const char* GetMininumGUIAPIVersion(void)
{
    return ""; // GUI API not used
}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
    pCapabilities->bSupportsEPG = true;
    pCapabilities->bSupportsTV = true;
    pCapabilities->bSupportsRadio = false;
    pCapabilities->bSupportsChannelGroups = true;
    pCapabilities->bSupportsRecordings = false;
    pCapabilities->bSupportsRecordingsUndelete = false;
    pCapabilities->bSupportsTimers = false;

    return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
    static const char *strBackendName = "HDHomeRun PVR add-on";
    return strBackendName;
}

const char *GetBackendVersion(void)
{
    static const char *strBackendVersion = "0.2";
    return strBackendVersion;
}

const char *GetConnectionString(void)
{
    static const char *strConnectionString = "connected";
    return strConnectionString;
}

const char *GetBackendHostname(void)
{
    return "";
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
    *iTotal = 1024 * 1024 * 1024;
    *iUsed = 0;
    return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel,
        time_t iStart, time_t iEnd)
{
    return g.lineup ?
            g.lineup->PvrGetEPGForChannel(handle, channel, iStart, iEnd) :
            PVR_ERROR_SERVER_ERROR;
}

int GetChannelsAmount(void)
{
    return g.lineup ? g.lineup->PvrGetChannelsAmount() : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
    return g.lineup ?
            g.lineup->PvrGetChannels(handle, bRadio) : PVR_ERROR_SERVER_ERROR;
}

int GetChannelGroupsAmount(void)
{
    return g.lineup ?
            g.lineup->PvrGetChannelGroupsAmount() : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
    return g.lineup ?
            g.lineup->PvrGetChannelGroups(handle, bRadio) :
            PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle,
        const PVR_CHANNEL_GROUP &group)
{
    return g.lineup ?
            g.lineup->PvrGetChannelGroupMembers(handle, group) :
            PVR_ERROR_SERVER_ERROR;
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
    std::cout << "Client OpenLiveStream\n";
    return false;
}

void CloseLiveStream(void)
{
    std::cout << "Client CloseLiveStream\n";
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
    std::cout << "Client SwitchChannel\n";
    CloseLiveStream();

    return OpenLiveStream(channel);
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
    PVR_STRCPY(signalStatus.strAdapterName, "PVR HDHomeRun Adapter 1");
    PVR_STRCPY(signalStatus.strAdapterStatus, "OK");

    return PVR_ERROR_NO_ERROR;
}

bool CanPauseStream(void)
{
    return true;
}

bool CanSeekStream(void)
{
    return true;
}

/* UNUSED API FUNCTIONS */
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook,
        const PVR_MENUHOOK_DATA &item)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
int GetRecordingsAmount(bool deleted)
{
    return -1;
}
PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
int GetTimersAmount(void)
{
    return -1;
}
PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR OpenDialogChannelScan(void)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL &channel)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL &channel)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
bool OpenRecordedStream(const PVR_RECORDING &recording)
{
    return false;
}
void CloseRecordedStream(void)
{
}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
    return 0;
}
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
    return 0;
}
long long PositionRecordedStream(void)
{
    return -1;
}
long long LengthRecordedStream(void)
{
    return 0;
}
void DemuxReset(void)
{
}
void DemuxFlush(void)
{
}
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
    return 0;
}
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
    return -1;
}
long long PositionLiveStream(void)
{
    return -1;
}
long long LengthLiveStream(void)
{
    return -1;
}
const char * GetLiveStreamURL(const PVR_CHANNEL &channel)
{
    if (g.lineup)
        return g.lineup->GetLiveStreamURL(channel);
    return "";
}
PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording,
        int lastplayedposition)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
    return -1;
}
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
;
PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
void DemuxAbort(void)
{
}
DemuxPacket* DemuxRead(void)
{
    return nullptr;
}
unsigned int GetChannelSwitchDelay(void)
{
    return 0;
}
void PauseStream(bool bPaused)
{
}
bool SeekTime(double, bool, double*)
{
    return false;
}
void SetSpeed(int)
{
}
;
time_t GetPlayingTime()
{
    return 0;
}
time_t GetBufferTimeStart()
{
    return 0;
}
time_t GetBufferTimeEnd()
{
    return 0;
}
PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR DeleteAllRecordingsFromTrash()
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
bool IsTimeshifting(void)
{
    return false;
}
bool IsRealTimeStream(void)
{
    return true;
}
PVR_ERROR SetEPGTimeFrame(int)
{
    return PVR_ERROR_NOT_IMPLEMENTED;
}
} // extern "C"
