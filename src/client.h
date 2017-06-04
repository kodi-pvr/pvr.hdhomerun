#pragma once
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

#include <libXBMC_addon.h>
#include <libXBMC_pvr.h>

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

namespace PVRHDHomeRun {

class HDHomeRunTuners;
class Lineup;


struct SettingsType
{
    enum CHANNEL_NAME {
        TUNER_NAME,
        GUIDE_NAME,
        AFFILIATE
    };

    bool bHideProtected         = true;
    bool bHideDuplicateChannels = true;
    bool bDebug                 = false;
    bool bMarkNew               = false;
    bool bUseLegacy             = false;
    bool bHideUnknownChannels   = true;
    CHANNEL_NAME eChannelName   = AFFILIATE;
};

struct GlobalsType
{
    bool         bCreated                = false;
    ADDON_STATUS currentStatus           = ADDON_STATUS_UNKNOWN;
    std::string  strUserPath;
    std::string  strClientPath;
    ADDON::CHelper_libXBMC_addon* XBMC   = nullptr;
    CHelper_libXBMC_pvr*          PVR    = nullptr;
    Lineup*                       lineup = nullptr;

    SettingsType Settings;
};

extern GlobalsType g;

};

extern "C"
{
	extern DLL_EXPORT void ADDON_ReadSettings(void);
	extern DLL_EXPORT ADDON_STATUS ADDON_Create(void* hdl, void* props);
	extern DLL_EXPORT ADDON_STATUS ADDON_GetStatus();
	extern DLL_EXPORT void ADDON_Destroy();
	extern DLL_EXPORT bool ADDON_HasSettings();
	extern DLL_EXPORT ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue);
	extern DLL_EXPORT void OnSystemSleep();
	extern DLL_EXPORT void OnSystemWake();
	extern DLL_EXPORT void OnPowerSavingActivated();
	extern DLL_EXPORT void OnPowerSavingDeactivated();
	extern DLL_EXPORT PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities);
	extern DLL_EXPORT const char *GetBackendName(void);
	extern DLL_EXPORT const char *GetBackendVersion(void);
	extern DLL_EXPORT const char *GetConnectionString(void);
	extern DLL_EXPORT const char *GetBackendHostname(void);
	extern DLL_EXPORT PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed);
	extern DLL_EXPORT PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel,
		time_t iStart, time_t iEnd);
	extern DLL_EXPORT int GetChannelsAmount(void);
	extern DLL_EXPORT PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
	extern DLL_EXPORT int GetChannelGroupsAmount(void);
	extern DLL_EXPORT PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
	extern DLL_EXPORT PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle,
		const PVR_CHANNEL_GROUP &group);
	extern DLL_EXPORT bool OpenLiveStream(const PVR_CHANNEL &channel);
	extern DLL_EXPORT void CloseLiveStream(void);
	extern DLL_EXPORT bool SwitchChannel(const PVR_CHANNEL &channel);
	extern DLL_EXPORT PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);
	extern DLL_EXPORT bool CanPauseStream(void);
	extern DLL_EXPORT bool CanSeekStream(void);

	extern DLL_EXPORT PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook,
		const PVR_MENUHOOK_DATA &item);
	extern DLL_EXPORT PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties);
	extern DLL_EXPORT int GetRecordingsAmount(bool deleted);
	extern DLL_EXPORT PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted);
	extern DLL_EXPORT int GetTimersAmount(void);
	extern DLL_EXPORT PVR_ERROR GetTimers(ADDON_HANDLE handle);
	extern DLL_EXPORT PVR_ERROR OpenDialogChannelScan(void);
	extern DLL_EXPORT PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel);
	extern DLL_EXPORT PVR_ERROR RenameChannel(const PVR_CHANNEL &channel);
	extern DLL_EXPORT PVR_ERROR MoveChannel(const PVR_CHANNEL &channel);
	extern DLL_EXPORT PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL &channel);
	extern DLL_EXPORT PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL &channel);
	extern DLL_EXPORT bool OpenRecordedStream(const PVR_RECORDING &recording);
	extern DLL_EXPORT void CloseRecordedStream(void);
	extern DLL_EXPORT int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
	extern DLL_EXPORT long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */);
	extern DLL_EXPORT long long PositionRecordedStream(void);
	extern DLL_EXPORT long long LengthRecordedStream(void);
	extern DLL_EXPORT void DemuxReset(void);
	extern DLL_EXPORT int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
	extern DLL_EXPORT long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */);
	extern DLL_EXPORT long long LengthLiveStream(void);
	extern DLL_EXPORT const char * GetLiveStreamURL(const PVR_CHANNEL &channel);
	extern DLL_EXPORT PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);
	extern DLL_EXPORT PVR_ERROR RenameRecording(const PVR_RECORDING &recording);
	extern DLL_EXPORT PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count);
	extern DLL_EXPORT PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording,
		int lastplayedposition);
	extern DLL_EXPORT int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording);
	extern DLL_EXPORT PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*);
	extern DLL_EXPORT PVR_ERROR AddTimer(const PVR_TIMER &timer);
	extern DLL_EXPORT PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
	extern DLL_EXPORT PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
	extern DLL_EXPORT void DemuxAbort(void);
	extern DLL_EXPORT DemuxPacket* DemuxRead(void);
	extern DLL_EXPORT unsigned int GetChannelSwitchDelay(void);
	extern DLL_EXPORT void PauseStream(bool bPaused);
	extern DLL_EXPORT bool SeekTime(double, bool, double*);
	extern DLL_EXPORT void SetSpeed(int);
	extern DLL_EXPORT time_t GetPlayingTime();
	extern DLL_EXPORT time_t GetBufferTimeStart();
	extern DLL_EXPORT time_t GetBufferTimeEnd();
	extern DLL_EXPORT PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording);
	extern DLL_EXPORT PVR_ERROR DeleteAllRecordingsFromTrash();
	extern DLL_EXPORT PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size);
	extern DLL_EXPORT bool IsTimeshifting(void);
	extern DLL_EXPORT bool IsRealTimeStream(void);
	extern DLL_EXPORT PVR_ERROR SetEPGTimeFrame(int);

} // extern "C"

