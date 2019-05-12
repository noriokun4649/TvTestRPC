// TvTestRPC.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include <time.h>
class CMyPlugin : public TVTest::CTVTestPlugin
{
	DiscordEventHandlers handlers;
	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);

public:
	bool pluginState;
	void discordInit();
	void UpdateState();
	time_t SystemTime2Timet(const SYSTEMTIME& st);
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override
	{
		pInfo->Type = TVTest::PLUGIN_TYPE_NORMAL;
		pInfo->Flags = 0;
		pInfo->pszPluginName = L"TvTest RPC";
		pInfo->pszCopyright = L"(c) 2019 noriokun4649";
		pInfo->pszDescription = L"DiscordRPCをTvTestで実現します。Discordで視聴中のチャンネルなどの情報が通知されます。";
		return true; 
	}

	bool Initialize() override
	{
		discordInit();
		m_pApp->SetEventCallback(EventCallback, this);
		return true; 
	}
	bool Finalize() override
	{
		Discord_Shutdown();
		return true;
	}
};

void CMyPlugin::discordInit()
{
	Discord_Initialize("577065126084214816", &handlers, 1, NULL);
}

void CMyPlugin::UpdateState()
{
	TVTest::ChannelInfo ChannelInfo;
	TVTest::ProgramInfo Info;
	ChannelInfo.Size = sizeof(ChannelInfo);
	Info.Size = sizeof(Info);
	wchar_t eventName[128];
	wchar_t eventText[128];
	wchar_t eventExtText[128];
	Info.pszEventName = eventName;
	Info.MaxEventName = sizeof(eventName) / sizeof(eventName[0]);
	Info.pszEventText = eventText;
	Info.MaxEventText = sizeof(eventText) / sizeof(eventText[0]);
	Info.pszEventExtText = eventExtText;
	Info.MaxEventExtText = sizeof(eventExtText) / sizeof(eventExtText[0]);
	if (m_pApp->GetCurrentChannelInfo(&ChannelInfo) && m_pApp->GetCurrentProgramInfo(&Info)) {
		DiscordRichPresence discordPresence;
		memset(&discordPresence, 0, sizeof(discordPresence));
		discordPresence.details = wide_to_utf8(ChannelInfo.szChannelName).c_str();
		discordPresence.state = wide_to_utf8(Info.pszEventName).c_str();
		discordPresence.startTimestamp = SystemTime2Timet(Info.StartTime);
		discordPresence.endTimestamp = SystemTime2Timet(Info.StartTime) + Info.Duration;
		discordPresence.largeImageKey = "tvtest";
		discordPresence.partyId = "";
		discordPresence.partySize = 0;
		discordPresence.partyMax = 0;
		discordPresence.matchSecret = "";
		discordPresence.joinSecret = "";
		discordPresence.spectateSecret = "";
		discordPresence.instance = 0;
		Discord_UpdatePresence(&discordPresence);
	}
}

time_t CMyPlugin::SystemTime2Timet(const SYSTEMTIME& st)
{
	struct tm gm = { st.wSecond, st.wMinute, st.wHour, st.wDay, st.wMonth - 1, st.wYear - 1900, st.wDayOfWeek, 0, 0 };
	return mktime(&gm);
}

TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CMyPlugin;
}
LRESULT CALLBACK CMyPlugin::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CMyPlugin *pThis = static_cast<CMyPlugin *>(pClientData);
	bool fEnable = lParam1 != 0;
	switch (Event)
	{
	case TVTest::EVENT_PLUGINENABLE:
		if (fEnable) {
			pThis->UpdateState();
		}
		else
		{
			pThis->pluginState = false;
			Discord_ClearPresence();
		}
		return TRUE;
	case TVTest::EVENT_CHANNELCHANGE:
		pThis->UpdateState();
		return TRUE;
	}
	return 0;
}