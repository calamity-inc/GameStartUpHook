#include <filesystem>
#include <fstream>
#include <string>

#include <Windows.h>

using D3DReflect_t = HRESULT(__stdcall*)(LPCVOID pSrcData, SIZE_T SrcDataSize, const IID* const pInterface, void** ppReflector);

static HMODULE og_lib;
static D3DReflect_t og_D3DReflect;

extern "C" __declspec(dllexport) HRESULT __stdcall D3DReflect(LPCVOID pSrcData, SIZE_T SrcDataSize, const IID* const pInterface, void** ppReflector)
{
	return og_D3DReflect(pSrcData, SrcDataSize, pInterface, ppReflector);
}

static void onGameStart()
{
	auto path = std::move(std::string(getenv("appdata")).append(R"(\Stand\)"));
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directory(path);
	}
	path.append("Bin\\");
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directory(path);
	}
	std::string tmp_path(path);
	tmp_path.append("Temp\\");
	if (!std::filesystem::exists(tmp_path))
	{
		std::filesystem::create_directory(tmp_path);
	}
	tmp_path.append("AutoInject.dll");
	if (std::filesystem::exists(tmp_path))
	{
		std::filesystem::remove(tmp_path);
	}

	system(R"(powershell -Command "Invoke-WebRequest https://stand.gg/versions.txt -UseBasicParsing -OutFile %tmp%\versions.txt")");
	std::wstring versions_path(std::wstring(_wgetenv(L"tmp")) + L"\\versions.txt");
	std::ifstream versions_in(versions_path);
	std::string versions{};
	std::getline(versions_in, versions);
	versions_in.close();
	if (versions.empty())
	{
		MessageBoxA(nullptr, "Failed to fetch latest Stand version", "Stand GameStartUpHook", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	std::filesystem::remove(versions_path);
	auto sep = versions.find(':');
	auto stand_version = versions.substr(sep + 1);

	path.append("Stand ").append(stand_version).append(".dll");
	if (!std::filesystem::exists(path))
	{
		std::string cmd(R"(powershell -Command "Invoke-WebRequest https://stand.gg/Stand%20")");
		cmd.append(stand_version);
		cmd.append(".dll -UseBasicParsing -OutFile ");
		cmd.append(tmp_path);
		cmd.push_back('"');
		system(cmd.c_str());
		std::filesystem::copy(tmp_path, path);
	}
	else
	{
		std::filesystem::copy(path, tmp_path);
	}
	LoadLibraryA(tmp_path.c_str());
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hInstance);
			std::wstring path(_wgetenv(L"windir"));
			path.append(LR"(\System32\D3DCompiler_43.dll)");
			og_lib = LoadLibraryW(path.c_str());
			og_D3DReflect = (D3DReflect_t)GetProcAddress(og_lib, "D3DReflect");
			CreateThread(0, 0, [](PVOID) -> DWORD
			{
				try
				{
					onGameStart();
				}
				catch (const std::exception& e)
				{
					MessageBoxA(nullptr, e.what(), "Stand GameStartUpHook", MB_OK | MB_ICONEXCLAMATION);
				}
				return 0;
			}, 0, 0, 0);
		}
		break;

	case DLL_PROCESS_DETACH:
		FreeLibrary(og_lib);
		break;
	}
	return TRUE;
}
