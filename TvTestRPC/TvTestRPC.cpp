// TvTestRPC.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include <time.h>
#include <shlwapi.h>
#include "resource.h"
#include <vector>
#pragma comment(lib,"shlwapi.lib")

class CMyPlugin : public TVTest::CTVTestPlugin
{
	DiscordEventHandlers handlers{};
	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void* pClientData);
	static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void* pClientData);
	bool ShowDialog(HWND hwndOwner);
	TCHAR m_szIniFileName[MAX_PATH];
	bool conf_mode = false;
	bool conf_isFinalized = false;
	bool InitSettings();
	bool pluginState = false;
	void InitDiscord();
	void UpdateState();
	const std::vector<WORD> knownIds = { 32736, 32737, 32738, 32739, 32740, 32741, 3274, 32742, 32327, 32375, 32391};

public:
	time_t SystemTime2Timet(const SYSTEMTIME&);

	DWORD GetVersion() override
	{
		return TVTEST_PLUGIN_VERSION_(0, 0, 1);
	}

	bool GetPluginInfo(TVTest::PluginInfo* pInfo) override
	{
		pInfo->Type = TVTest::PLUGIN_TYPE_NORMAL;
		pInfo->Flags = TVTest::PLUGIN_FLAG_HASSETTINGS;
		pInfo->pszPluginName = L"TvTest RPC";
		pInfo->pszCopyright = L"(c) 2019-2020 noriokun4649";
		pInfo->pszDescription = L"DiscordRPCをTvTestで実現します。Discordで視聴中のチャンネルなどの情報が通知されます。";
		return true;
	}

	bool Initialize() override
	{
		InitDiscord();
		m_pApp->SetEventCallback(EventCallback, this);
		return true;
	}
	bool Finalize() override
	{
		Discord_Shutdown();
		return true;
	}
};

bool CMyPlugin::InitSettings() {
	::GetModuleFileName(g_hinstDLL, m_szIniFileName, MAX_PATH);
	::PathRenameExtension(m_szIniFileName, TEXT(".ini"));
	conf_mode = ::GetPrivateProfileInt(TEXT("Settings"), TEXT("Mode"), conf_mode, m_szIniFileName) != 0;
	m_pApp->AddLog(m_szIniFileName);
	conf_isFinalized = true;
	return true;
}

void CMyPlugin::InitDiscord()
{
	Discord_Initialize("577065126084214816", &handlers, 1, NULL);
}

void CMyPlugin::UpdateState()
{
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	TVTest::ProgramInfo Info;
	TVTest::ServiceInfo Service;
	TVTest::ChannelInfo ChannelInfo;
	Info.Size = sizeof(Info);
	Service.Size = sizeof(Service);
	ChannelInfo.Size = sizeof(ChannelInfo);
	wchar_t eventName[128];
	wchar_t eventText[128];
	wchar_t eventExtText[128];
	Info.pszEventName = eventName;
	Info.MaxEventName = sizeof(eventName) / sizeof(eventName[0]);
	Info.pszEventText = eventText;
	Info.MaxEventText = sizeof(eventText) / sizeof(eventText[0]);
	Info.pszEventExtText = eventExtText;
	Info.MaxEventExtText = sizeof(eventExtText) / sizeof(eventExtText[0]);
	std::string channelName;
	std::string eventNamed;

	if (m_pApp->GetCurrentProgramInfo(&Info)) {
		eventNamed = wide_to_utf8(Info.pszEventName);
		auto start = SystemTime2Timet(Info.StartTime);
		auto end = SystemTime2Timet(Info.StartTime) + Info.Duration;
		discordPresence.startTimestamp = start;
		discordPresence.endTimestamp = end;
	}

	if (m_pApp->GetServiceInfo(0, &Service) && m_pApp->GetCurrentChannelInfo(&ChannelInfo)) {
		if (!conf_mode) {
			channelName = wide_to_utf8(Service.szServiceName);
		}
		else {
			channelName = wide_to_utf8(ChannelInfo.szChannelName);
		}

		auto id = ChannelInfo.NetworkID;
		auto isId = find(knownIds.begin(), knownIds.end(), id) != knownIds.end();

		if (isId) {
			auto netId = std::to_string(id);
			discordPresence.largeImageKey = netId.c_str();
			discordPresence.smallImageKey = "tvtest";
			discordPresence.smallImageText = "TvTest";
		}
		else {
			discordPresence.largeImageKey = "tvtest";
			discordPresence.smallImageKey = "";
			discordPresence.smallImageText = "";
		}
	}

	discordPresence.details = channelName.c_str();
	discordPresence.largeImageText = channelName.c_str();
	discordPresence.state = eventNamed.c_str();
	discordPresence.partyId = "";
	discordPresence.partySize = 0;
	discordPresence.partyMax = 0;
	discordPresence.matchSecret = "";
	discordPresence.joinSecret = "";
	discordPresence.spectateSecret = "";
	discordPresence.instance = 0;
	Discord_UpdatePresence(&discordPresence);

}


time_t CMyPlugin::SystemTime2Timet(const SYSTEMTIME& st)
{
	struct tm gm = { st.wSecond, st.wMinute, st.wHour, st.wDay, st.wMonth - 1, st.wYear - 1900, st.wDayOfWeek, 0, 0 };
	return mktime(&gm);
}

TVTest::CTVTestPlugin* CreatePluginClass()
{
	return new CMyPlugin;
}

bool CMyPlugin::ShowDialog(HWND hwndOwner) {

	TVTest::ShowDialogInfo Info;
	Info.Flags = 0;
	Info.hinst = g_hinstDLL;
	Info.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG1);
	Info.pMessageFunc = SettingsDlgProc;
	Info.pClientData = this;
	Info.hwndOwner = hwndOwner;

	return m_pApp->ShowDialog(&Info) == IDOK;
}

INT_PTR CALLBACK CMyPlugin::SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void* pClientData) {
	CMyPlugin* pThis = static_cast<CMyPlugin*>(pClientData);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		::CheckDlgButton(hDlg, IDC_CHECK1, pThis->conf_mode ? BST_CHECKED : BST_UNCHECKED);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			pThis->conf_mode = ::IsDlgButtonChecked(hDlg, IDC_CHECK1) == BST_CHECKED;
			if (pThis->conf_isFinalized) {
				struct IntString {
					IntString(int Value) { ::wsprintf(m_szBuffer, TEXT("%d"), Value); }
					operator LPCTSTR() const { return m_szBuffer; }
					TCHAR m_szBuffer[16];
				};

				::WritePrivateProfileString(TEXT("Settings"), TEXT("Mode"), IntString(pThis->conf_mode), pThis->m_szIniFileName);
			}
			pThis->UpdateState();
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
	default:
		break;
		return FALSE;
	}
	return FALSE;
}

LRESULT CALLBACK CMyPlugin::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void* pClientData)
{
	CMyPlugin* pThis = static_cast<CMyPlugin*>(pClientData);
	bool fEnable = lParam1 != 0;
	switch (Event)
	{
	case TVTest::EVENT_PLUGINENABLE:
		if (fEnable) {
			pThis->UpdateState();
			pThis->InitSettings();
		}
		else
		{
			pThis->pluginState = false;
			Discord_ClearPresence();
		}
		return TRUE;
	case TVTest::EVENT_SERVICECHANGE:
		pThis->UpdateState();
	case TVTest::EVENT_CHANNELCHANGE:
		pThis->UpdateState();
		return TRUE;
	case TVTest::EVENT_SERVICEUPDATE:
		pThis->UpdateState();
		return TRUE;
	case TVTest::EVENT_PLUGINSETTINGS:
		return pThis->ShowDialog(reinterpret_cast<HWND>(lParam1));
	}
	return 0;
}