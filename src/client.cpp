/*
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

#include <cstring>
#include <string>
#include <p8-platform/threads/threads.h>
#include <p8-platform/util/util.h>
#include <xbmc_pvr_dll.h>

#include "HDHomeRunTuners.h"
#include "Utils.h"

GlobalsType g;

#define MENUHOOK_SETTING_UPDATEGUIDE 1
#define MENUHOOK_SETTING_UPDATEDEVICE 2

class UpdateThread : public P8PLATFORM::CThread
{
public:
  void *Process()
  {
    for (;;)
    {
      for (int i = 0; i < 60*60; i++)
        if (P8PLATFORM::CThread::Sleep(1000))
          break;
      
      if (IsStopped())
        break;

      if (g.Tuners)
      {
        if (g.Tuners->Update(HDHomeRunTuners::UpdateLineUp | HDHomeRunTuners::UpdateGuide))
          g.PVR->TriggerChannelUpdate();
      }
    }
    return NULL;
  }
};

UpdateThread g_UpdateThread;

extern "C" {

void ADDON_ReadSettings(void)
{
  if (g.XBMC == NULL)
    return;

  if (!g.XBMC->GetSetting("hide_protected", &g.Settings.bHideProtected))
    g.Settings.bHideProtected = true;

  if (!g.XBMC->GetSetting("hide_duplicate", &g.Settings.bHideDuplicateChannels))
    g.Settings.bHideDuplicateChannels = true;

  if (!g.XBMC->GetSetting("mark_new", &g.Settings.bMarkNew))
    g.Settings.bMarkNew = true;

  if (!g.XBMC->GetSetting("debug", &g.Settings.bDebug))
    g.Settings.bDebug = false;
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  g.XBMC = new ADDON::CHelper_libXBMC_addon;
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

  KODI_LOG(ADDON::LOG_NOTICE, "%s - Creating the PVR HDHomeRun add-on", __FUNCTION__);

  g.currentStatus = ADDON_STATUS_UNKNOWN;

  g.Tuners = new HDHomeRunTuners;
  if (g.Tuners == NULL)
  {
    SAFE_DELETE(g.PVR);
    SAFE_DELETE(g.XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  ADDON_ReadSettings();
  g.Tuners->Update();
  if (!g_UpdateThread.CreateThread(false))
  {
    SAFE_DELETE(g.Tuners);
    SAFE_DELETE(g.PVR);
    SAFE_DELETE(g.XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR_MENUHOOK hook;
  hook.iHookId = MENUHOOK_SETTING_UPDATEGUIDE;
  hook.category = PVR_MENUHOOK_ALL;
  hook.iLocalizedStringId = 32101;
  g.PVR->AddMenuHook(&hook);

  hook.iHookId = MENUHOOK_SETTING_UPDATEDEVICE;
  hook.category = PVR_MENUHOOK_ALL;
  hook.iLocalizedStringId = 32102;
  g.PVR->AddMenuHook(&hook);

  g.currentStatus = ADDON_STATUS_OK;

  return ADDON_STATUS_OK;
}

ADDON_STATUS ADDON_GetStatus()
{
  return g.currentStatus;
}

void ADDON_Destroy()
{
  g_UpdateThread.StopThread();

  SAFE_DELETE(g.Tuners);
  SAFE_DELETE(g.PVR);
  SAFE_DELETE(g.XBMC);

  g.currentStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  if (g.Tuners == NULL)
    return ADDON_STATUS_OK;

  if (strcmp(settingName, "hide_protected") == 0)
  {
    g.Settings.bHideProtected = *(bool*)settingValue;
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (strcmp(settingName, "hide_duplicate") == 0)
  {
    g.Settings.bHideDuplicateChannels = *(bool*)settingValue;
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (strcmp(settingName, "mark_new") == 0)
    g.Settings.bMarkNew = *(bool*)settingValue;
  else if (strcmp(settingName, "debug") == 0)
    g.Settings.bDebug = *(bool*)settingValue;

  return ADDON_STATUS_OK;
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

void OnSystemSleep()
{
}

void OnSystemWake()
{
  if (g.Tuners && g.PVR)
  {
    g.Tuners->Update(HDHomeRunTuners::UpdateLineUp | HDHomeRunTuners::UpdateGuide);
    g.PVR->TriggerChannelUpdate();
  }
}

void OnPowerSavingActivated()
{
}

void OnPowerSavingDeactivated()
{
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
  pCapabilities->bSupportsRecordingsRename = false;
  pCapabilities->bSupportsRecordingsLifetimeChange = false;
  pCapabilities->bSupportsDescrambleInfo = false;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = "HDHomeRun PVR add-on";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static const char *strBackendVersion = "0.1";
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
  *iUsed  = 0;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  return g.Tuners ? g.Tuners->PvrGetEPGForChannel(handle, channel, iStart, iEnd) : PVR_ERROR_SERVER_ERROR;
}

int GetChannelsAmount(void)
{
  return g.Tuners ? g.Tuners->PvrGetChannelsAmount() : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  return g.Tuners ? g.Tuners->PvrGetChannels(handle, bRadio) : PVR_ERROR_SERVER_ERROR;
}

int GetChannelGroupsAmount(void)
{
  return g.Tuners ? g.Tuners->PvrGetChannelGroupsAmount() : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  return g.Tuners ? g.Tuners->PvrGetChannelGroups(handle, bRadio) : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  return g.Tuners ? g.Tuners->PvrGetChannelGroupMembers(handle, group) : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  strncpy(signalStatus.strAdapterName, "PVR HDHomeRun Adapter 1",
          sizeof(signalStatus.strAdapterName) - 1);
  signalStatus.strAdapterName[sizeof(signalStatus.strAdapterName) - 1] = '\0';
  strncpy(signalStatus.strAdapterStatus, "OK",
          sizeof(signalStatus.strAdapterStatus) - 1);
  signalStatus.strAdapterStatus[sizeof(signalStatus.strAdapterStatus) - 1] = '\0';

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

PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL* channel, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  if (!channel || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 2)
    return PVR_ERROR_INVALID_PARAMETERS;

  std::string strUrl = g.Tuners->_GetChannelStreamURL(channel->iUniqueId);
  if (strUrl.empty())
    return PVR_ERROR_FAILED;

  strncpy(properties[0].strName, PVR_STREAM_PROPERTY_STREAMURL, sizeof(properties[0].strName) - 1);
  strncpy(properties[0].strValue, strUrl.c_str(), sizeof(properties[0].strValue) - 1);
  strncpy(properties[1].strName, PVR_STREAM_PROPERTY_ISREALTIMESTREAM, sizeof(properties[1].strName) - 1);
  strncpy(properties[1].strValue, "true", sizeof(properties[1].strValue) - 1);

  *iPropertiesCount = 2;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CallMenuHook(const PVR_MENUHOOK& menuhook, const PVR_MENUHOOK_DATA&)
{
  int iMsg;
  switch (menuhook.iHookId)
  {
    case MENUHOOK_SETTING_UPDATEGUIDE:
      iMsg = 32111;
      g.Tuners->Update(HDHomeRunTuners::UpdateGuide);
      g.Tuners->TriggerEPGUpdate();
      break;
    case MENUHOOK_SETTING_UPDATEDEVICE:
      iMsg = 32112;
      g.Tuners->Update(HDHomeRunTuners::UpdateDiscover | HDHomeRunTuners::UpdateLineUp | HDHomeRunTuners::UpdateGuide);
      g.PVR->TriggerChannelUpdate();
      g.Tuners->TriggerEPGUpdate();
      break;
    default:
      return PVR_ERROR_INVALID_PARAMETERS;
  }
  char* msg = g.XBMC->GetLocalizedString(iMsg);
  g.XBMC->QueueNotification(ADDON::QUEUE_INFO, msg);
  g.XBMC->FreeString(msg);

  return PVR_ERROR_NO_ERROR;
}

/* UNUSED API FUNCTIONS */
PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO*) { return PVR_ERROR_NOT_IMPLEMENTED; }
// Channel
PVR_ERROR OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL&) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL&) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL&) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL&) { return PVR_ERROR_NOT_IMPLEMENTED; }
// Demux
void DemuxAbort(void) {}
void DemuxFlush(void) {}
DemuxPacket* DemuxRead(void) { return NULL; }
void DemuxReset(void) {}
// LiveStream
void CloseLiveStream(void) {}
bool OpenLiveStream(const PVR_CHANNEL&) { return false; }
int ReadLiveStream(unsigned char*, unsigned int) { return 0; }
long long LengthLiveStream(void) { return -1; }
long long SeekLiveStream(long long, int) { return -1; }
bool SeekTime(double,bool,double*) { return false; }
bool IsRealTimeStream(void) { return true; }
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES *times) { return PVR_ERROR_NOT_IMPLEMENTED; }
// Recording
bool OpenRecordedStream(const PVR_RECORDING&) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char*, unsigned int) { return 0; }
long long SeekRecordedStream(long long, int) { return 0; }
long long LengthRecordedStream(void) { return 0; }
PVR_ERROR GetRecordingStreamProperties(const PVR_RECORDING*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteRecording(const PVR_RECORDING&) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordings(ADDON_HANDLE, bool) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingsAmount(bool) { return -1; }
PVR_ERROR RenameRecording(const PVR_RECORDING&) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING&, int) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING&) { return -1; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING&, int) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLifetime(const PVR_RECORDING*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UndeleteRecording(const PVR_RECORDING&) { return PVR_ERROR_NOT_IMPLEMENTED; }
// Timers
PVR_ERROR AddTimer(const PVR_TIMER&) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER&, bool) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetTimersAmount(void) { return -1; }
PVR_ERROR GetTimers(ADDON_HANDLE) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER&) { return PVR_ERROR_NOT_IMPLEMENTED; }
// Timeshift
void PauseStream(bool bPaused) {}
void SetSpeed(int) {};
bool IsTimeshifting(void) { return false; }
// EPG
PVR_ERROR SetEPGTimeFrame(int) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagPlayable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagRecordable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagStreamProperties(const EPG_TAG*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagEdl(const EPG_TAG* epgTag, PVR_EDL_ENTRY edl[], int *size) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamReadChunkSize(int* chunksize) { return PVR_ERROR_NOT_IMPLEMENTED; }

}
