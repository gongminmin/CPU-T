#include <CPU-T/CPU.hpp>

#include <windows.h>
#include <tchar.h>

#include <vector>
#include <string>

#include "resource.h"

DWORD const IDT_UPDATE_FREQ_TIMER = 1;

HINSTANCE g_instance;
bool g_in_chs;
CPUT::CPUInfo g_CpuInfo;

INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (uMsg) 
	{
		case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
		case IDCANCEL: 
			::EndDialog(hwndDlg, wParam);
			return TRUE;

		case IDC_HOMEPAGE:
			::ShellExecute(NULL, TEXT("open"), TEXT("http://www.klayge.org"), NULL, NULL, SW_SHOWNORMAL);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

void CacheDesc(char* buf, CPUT::CPUInfo::CacheInfo const & cache_info)
{
	char size_buf[16];
	if (cache_info.size > 1024)
	{
		sprintf(size_buf, "%dMB", cache_info.size / 1024);
	}
	else
	{
		sprintf(size_buf, "%dKB", cache_info.size);
	}

	char way_buf[64];
	if (cache_info.way != 0xFF)
	{
		sprintf(way_buf, "%d-way set associative", cache_info.way);
	}
	else
	{
		sprintf(way_buf, "fully associative");
	}

	sprintf(buf, "%s, %s, %dB lines", size_buf, way_buf, cache_info.line);
}

INT_PTR CALLBACK CPUInfoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
	case WM_INITDIALOG:
		::SetTimer(hwndDlg, IDT_UPDATE_FREQ_TIMER, 1000, nullptr);
		return TRUE;

	case WM_SHOWWINDOW:
		{
			if (g_CpuInfo.CPUName() != NULL)
			{
				::SetDlgItemTextA(hwndDlg, IDC_CPU, g_CpuInfo.CPUName());
			}
	
			if (g_CpuInfo.BrandString()[0] != '\0')
			{
				::SetDlgItemTextA(hwndDlg, IDC_SPEC, g_CpuInfo.BrandString());
			}

			::SetDlgItemTextA(hwndDlg, IDC_CODENAME, g_CpuInfo.CodeName());
			::SetDlgItemTextA(hwndDlg, IDC_PACKAGE, g_CpuInfo.Package());
			::SetDlgItemTextA(hwndDlg, IDC_TECHNOLOGY, g_CpuInfo.Technology());
			::SetDlgItemTextA(hwndDlg, IDC_TRANSISTORS, g_CpuInfo.Transistors());

			TCHAR buf[256];
			_stprintf(buf, TEXT("%d MHz"), g_CpuInfo.Frequency());
			::SetDlgItemText(hwndDlg, IDC_FREQUENCY, buf);

			if (g_CpuInfo.Ratio() > 0)
			{
				_stprintf(buf, TEXT("%d"), g_CpuInfo.Ratio());
				::SetDlgItemText(hwndDlg, IDC_RATIO, buf);

				_stprintf(buf, TEXT("%f"), g_CpuInfo.Frequency() / g_CpuInfo.Ratio());
				::SetDlgItemText(hwndDlg, IDC_MAINBOARD, buf);
			}

			_stprintf(buf, TEXT("%X"), g_CpuInfo.Type());
			::SetDlgItemText(hwndDlg, IDC_TYPE, buf);

			_stprintf(buf, TEXT("%X"), g_CpuInfo.Family());
			::SetDlgItemText(hwndDlg, IDC_FAMILY, buf);

			_stprintf(buf, TEXT("%X"), g_CpuInfo.Model());
			::SetDlgItemText(hwndDlg, IDC_MODEL, buf);

			_stprintf(buf, TEXT("%X"), g_CpuInfo.Stepping());
			::SetDlgItemText(hwndDlg, IDC_STEPPING, buf);

			_stprintf(buf, TEXT("%d"), g_CpuInfo.NumCores());
			::SetDlgItemText(hwndDlg, IDC_NUM_CORES, buf);

			_stprintf(buf, TEXT("%d"), g_CpuInfo.NumHWThreads());
			::SetDlgItemText(hwndDlg, IDC_NUM_THREADS, buf);

			char abuf[MAX_PATH];
			CacheDesc(abuf, g_CpuInfo.L1DataCache());
			::SetDlgItemTextA(hwndDlg, IDC_L1_DATA_CACHE, abuf);
			CacheDesc(abuf, g_CpuInfo.L1CodeCache());
			::SetDlgItemTextA(hwndDlg, IDC_L1_CODE_CACHE, abuf);
			CacheDesc(abuf, g_CpuInfo.L2Cache());
			::SetDlgItemTextA(hwndDlg, IDC_L2_CACHE, abuf);
			CacheDesc(abuf, g_CpuInfo.L3Cache());
			::SetDlgItemTextA(hwndDlg, IDC_L3_CACHE, abuf);

			using namespace CPUT;

			std::vector<std::string> instructions;
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_CMPXCHG8B))
			{
				instructions.push_back("CX8");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_CMPXCHG16B))
			{
				instructions.push_back("CX16");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_SYSENTER))
			{
				instructions.push_back("SYSENTER");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_CMOV))
			{
				instructions.push_back("CMOV");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_MMX))
			{
				instructions.push_back("MMX");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_3DNow))
			{
				instructions.push_back("3DNow!");
			}			
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_3DNowExt))
			{
				instructions.push_back("Extended 3DNow!");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_SSE))
			{
				instructions.push_back("SSE");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_SSE2))
			{
				instructions.push_back("SSE2");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_SSE3))
			{
				instructions.push_back("SSE3");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_SSSE3))
			{
				instructions.push_back("SSSE3");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_SSE41))
			{
				instructions.push_back("SSE4.1");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_SSE42))
			{
				instructions.push_back("SSE4.2");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_SSE4A))
			{
				instructions.push_back("SSE4.A");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_MisalignedSSE))
			{
				instructions.push_back("MisalignedSSE");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_X64))
			{
				instructions.push_back("X64");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_FMA3))
			{
				instructions.push_back("FMA3");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_AVX))
			{
				instructions.push_back("AVX");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_AVX2))
			{
				instructions.push_back("AVX2");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_AES))
			{
				instructions.push_back("AES");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_FMA4))
			{
				instructions.push_back("FMA4");
			}
			if (g_CpuInfo.IsFeatureSupport(CPUInfo::CF_F16C))
			{
				instructions.push_back("F16C");
			}

			std::string instructions_str;
			for (size_t i = 0; i < instructions.size() - 1; ++ i)
			{
				instructions_str += instructions[i] + ", ";
			}
			instructions_str += instructions.back();
			::SetDlgItemTextA(hwndDlg, IDC_INSTRUCTION_SET, instructions_str.c_str());
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		{
			HWND ctrl = reinterpret_cast<HWND>(lParam);
			HDC hdc = reinterpret_cast<HDC>(wParam);
			if ((::GetDlgItem(hwndDlg, IDC_CPU) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_SPEC) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_CODENAME) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_PACKAGE) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_TECHNOLOGY) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_TRANSISTORS) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_L1_DATA_CACHE) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_L1_CODE_CACHE) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_L2_CACHE) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_L3_CACHE) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_FREQUENCY) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_RATIO) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_MAINBOARD) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_TYPE) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_FAMILY) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_MODEL) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_STEPPING) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_NUM_CORES) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_NUM_THREADS) == ctrl)
				|| (::GetDlgItem(hwndDlg, IDC_INSTRUCTION_SET) == ctrl))
			{
				::SetTextColor(hdc, RGB(14, 99, 156));
			}
			::SetBkColor(hdc, ::GetSysColor(COLOR_BTNFACE));
			return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_BTNFACE));
		}

	case WM_TIMER:
		if (IDT_UPDATE_FREQ_TIMER == wParam)
		{
			g_CpuInfo.UpdateFrequency();

			TCHAR buf[256];
			_stprintf(buf, TEXT("%d MHz"), g_CpuInfo.Frequency());
			::SetDlgItemText(hwndDlg, IDC_FREQUENCY, buf);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
		case IDCANCEL:
			::KillTimer(hwndDlg, IDT_UPDATE_FREQ_TIMER);
			::EndDialog(hwndDlg, wParam);
			return TRUE;

		case IDC_ABOUT:
			::DialogBox(g_instance, g_in_chs ? MAKEINTRESOURCE(IDD_ABOUT_CHS) : MAKEINTRESOURCE(IDD_ABOUT_EN),
				nullptr, AboutDlgProc);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

#if defined(CPUT_COMPILER_MSVC) && (CPUT_COMPILER_VERSION >= 100)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpszCmdLine*/, _In_ int /*nCmdShow*/)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpszCmdLine*/, int /*nCmdShow*/)
#endif
{
	g_instance = hInstance;

	LANGID lang_id = ::GetUserDefaultUILanguage();
	if ((LANG_CHINESE == PRIMARYLANGID(lang_id)) && (SUBLANG_CHINESE_SIMPLIFIED == SUBLANGID(lang_id)))
	{
		g_in_chs = true;
	}
	else
	{
		g_in_chs = false;
	}

	::DialogBox(hInstance, g_in_chs ? MAKEINTRESOURCE(IDD_CPUINFO_CHS) : MAKEINTRESOURCE(IDD_CPUINFO_EN),
		nullptr, CPUInfoDlgProc);

	return FALSE;
}
