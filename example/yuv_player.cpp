#include "yuv_player.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>
#include <memory>
#include <shlobj_core.h>
#include <io.h>
#include <direct.h>

#include "UIlib.h"
#include "my_window.h"
#include "d3d9_player.h"
#include "json.hpp"
#include "resource.h"
#include "sdl_player.h"

using Json = nlohmann::json;

class YUVPlayer::YUVPlayerImpl : public DuiLib::CWindowWnd,
                                 public DuiLib::INotifyUI,
                                 public DuiLib::IDialogBuilderCallback
{
public:
    YUVPlayerImpl()
    {
        _formatMap[" I420"] = I420;
        _formatMap[" NV12"] = NV12;
        _formatMap[" NV21"] = NV21;
		CreateConfigFile();
    }
    ~YUVPlayerImpl()
    {
        StopPlay();
		if (_fin) {
			_fin.close();
		}
		if (_work.joinable()) {
			_work.join();
		}
    }
    YUVPlayerImpl(const YUVPlayerImpl&) = delete;
    YUVPlayerImpl& operator =(const YUVPlayerImpl&) = delete;

	void CreateConfigFile() 
	{
		TCHAR szPath[MAX_PATH + 1] = { 0 };
		SHGetSpecialFolderPathA(NULL, szPath, CSIDL_APPDATA, FALSE);
		std::string directory = szPath;
		directory = directory + "\\YUVPlayer";

		if (_access(directory.c_str(), 0) == -1) {
			_mkdir(directory.c_str());
		}
		_configFilename = directory + "\\player_config.json";
		std::cout << "_configFilename:" << _configFilename << std::endl;
	}

    void Init()
    {
        SetProcessDPIAware();
        _hInstance = GetModuleHandle(0);
        DuiLib::CPaintManagerUI::SetInstance(_hInstance);
        DuiLib::CPaintManagerUI::SetResourcePath(DuiLib::CPaintManagerUI::GetInstancePath() + _T("resources"));
    }
    bool CreateDUIWindow()
    {
        _ownerWnd = Create(NULL, _T("YUVPlayer"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
        if (!_ownerWnd)
        {
            std::cout << "create dui window failed" << std::endl;
            return false;
        }
        return true;
    }
    void ShowWindow()
    {
        ShowModal();
    }

    LPCTSTR GetWindowClassName() const
    {
        return _T("DUIYUVPlayerFrame");
    }
    void Notify(DuiLib::TNotifyUI& msg)
    {
        if (msg.sType == _T("click"))
        {
            OnClick(msg);
        }
        else if (msg.sType == _T("valuechanged"))
        {
            OnValueChange(msg);
        }
		else if (msg.sType == _T("setfocus")) {
			OnSetFocus(msg);
		}
    }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT lRes = 0;
        switch (uMsg) {
        case WM_CREATE:
            lRes = OnCreate(uMsg, wParam, lParam);
            break;
        case WM_CLOSE:
            lRes = OnClose(uMsg, wParam, lParam);
            break;
        default:
            break;
        }
        if (_paintManager.MessageHandler(uMsg, wParam, lParam, lRes))
        {
            return lRes;
        }

        return __super::HandleMessage(uMsg, wParam, lParam);
    }

    DuiLib::CControlUI* CreateControl(LPCTSTR pstrClass)
    {
        if (_tcscmp(pstrClass, _T("MyWindow")) == 0) {
            CMyWindowUI* wndui = new CMyWindowUI();
            HWND wnd = CreateWindow(_T("STATIC"), _T(""),
                WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED,
                0, 0, 0, 0, _paintManager.GetPaintWindow(), (HMENU)0, NULL, NULL);
            wndui->Attach(wnd);
			_paintManager.SetFocus(wndui, false);
            return wndui;
        }
        return nullptr;
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        _paintManager.Init(m_hWnd);
        DuiLib::CDialogBuilder builder;
        DuiLib::CControlUI* pRoot = builder.Create(_T("player.xml"), (UINT)0, this, &_paintManager);
        _paintManager.AttachDialog(pRoot);
        _paintManager.AddNotifier(this);
        InitWindow();
        return S_OK;
    }
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
		StopPlay();
		// json
		Json config;
		auto fileEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("fileEdit")));
		auto widthEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("widthEdit")));
		auto heightEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("heightEdit")));
		auto formatType = static_cast<DuiLib::CComboUI*>(_paintManager.FindControl(_T("formatType")));
		std::string filename = fileEdit->GetText();
		std::string width = widthEdit->GetText();
		std::string height = heightEdit->GetText();
		std::string format = formatType->GetText();
		config["filename"] = filename;
		config["width"] = width;
		config["height"] = height;
		config["format"] = format;
		std::string data = config.dump();
		
		std::ofstream fout(_configFilename, std::ios::binary | std::ios::out);
		if (!fout)
		{
			std::cout << "open file failed" << std::endl;
			return S_OK;
		}
		fout.write(data.c_str(), data.size());
		fout.close();
		std::cout << "write file" << std::endl;
        return S_OK;
    }

    LRESULT OnSizeChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return S_OK;
    }

    std::string ChooseYUVFile() {
		auto fileEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("fileEdit")));
		std::string fileText = fileEdit->GetText();
        OPENFILENAME ofn;
        TCHAR szFile[MAX_PATH] = _T("");

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = *this;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = TEXT("YUV File(*YUV)\0*.yuv\0All Files(*.*)\0*.*\0\0");
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn))
        {
            std::cout << "szFile:" << szFile << std::endl;
			std::string name = szFile;
			if (name.empty()) {
				return fileText;
			}
            return szFile;
        }
        return fileText;
    }

    void OnClick(DuiLib::TNotifyUI& msg)
    {
        auto name = msg.pSender->GetName();
        if (name == "btnFile") {
            std::string filename = ChooseYUVFile();
            auto fileEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("fileEdit")));
            fileEdit->SetText(filename.c_str());
        }
        else if (name == "btnStart") {
            auto fileEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("fileEdit")));
            auto szFile = fileEdit->GetText();
            _filename = szFile;
            if (_filename.empty()) {
                MessageBox(NULL, "文件为空，请重新输入或者选择文件", "小提示", MB_OK);
                return;
            }
            // 检查宽高，是否正确
            auto widthEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("widthEdit")));
            auto heightEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("heightEdit")));
            std::string width = widthEdit->GetText();
            std::string height = heightEdit->GetText();
            if (width.empty() || height.empty()) {
                MessageBox(NULL, "宽或者高为空，请重新输入", "小提示", MB_OK);
                return;
            }
            _width = std::stoi(width);
            _height = std::stoi(height);
            std::cout << "width:" << _width << std::endl;
            std::cout << "height:" << _height << std::endl;

            auto formatType = static_cast<DuiLib::CComboUI*>(_paintManager.FindControl(_T("formatType")));
            std::string format = formatType->GetText();
            _format = _formatMap[format];
            auto btnPlay = static_cast<DuiLib::CButtonUI*>(_paintManager.FindControl(_T("btnPlay")));
            btnPlay->SetVisible();
			btnPlay->SetEnabled();
            auto btnPause = static_cast<DuiLib::CButtonUI*>(_paintManager.FindControl(_T("btnPause")));
            btnPause->SetVisible();
			btnPause->SetEnabled();
            if (_fin.is_open()) {
                std::cout << "Close File" << std::endl;
                _fin.close();
            }
            std::cout << "_filename:" << _filename << std::endl;
            _fin.open(_filename, std::ios::binary | std::ios::in);
            if (!_fin) {
                MessageBox(NULL, "文件打开失败，请重新输入或选择文件", "小提示", MB_OK);
                return;
            }
            StartPlay();
        }
        else if (name == "btnPlay") {
            PlayYUV();
        }
        else if (name == "btnPause") {
            PausePlay();
        }
    }
    void OnValueChange(DuiLib::TNotifyUI& msg)
    {

    }

	void OnSetFocus(DuiLib::TNotifyUI& msg)
	{
		auto name = msg.pSender->GetName();
		if (name == "videoDisplay") {
			return;
		}
	}

    void InitWindow()
    {
		SetIcon(IDI_ICON1);
		std::ifstream fin(_configFilename, std::ios::binary | std::ios::in);
		if (!fin)
		{
			std::cout << "file not fount" << std::endl;
			return;
		}
		fin.seekg(0, std::ios_base::end);
		int size = (int)fin.tellg();
		fin.seekg(0);
		char* data = new char[size + 1];
		data[size] = '\0';
		fin.read(data, size);
		fin.close();
		std::string strData(data);
		Json config = Json::parse(strData);
		auto fileEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("fileEdit")));
		auto widthEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("widthEdit")));
		auto heightEdit = static_cast<DuiLib::CEditUI*>(_paintManager.FindControl(_T("heightEdit")));
		auto formatType = static_cast<DuiLib::CComboUI*>(_paintManager.FindControl(_T("formatType")));
		std::string filename = config["filename"];
		std::string width = config["width"];
		std::string height = config["height"];
		std::string format = config["format"];
		fileEdit->SetText(filename.c_str());
		widthEdit->SetText(width.c_str());
		heightEdit->SetText(height.c_str());
		int count = formatType->GetCount();
		for (int i = 0; i < count; ++i) {
			auto item = formatType->GetItemAt(i);
			std::string text = item->GetText();
			if (text == format) {
				formatType->SelectItem(i);
				break;
			}
		}
    }

    void StartPlay()
    {
        if (_running) {
            // 如果当前有文件在播放，那么需要先停止，再重新开始
            std::cout << "StopPlay" << std::endl;
            StopPlay();
        }
        // 创建窗口
        CreatePlayer();
        _yuv_player->SetPixFormat(_format);
        // 读取文件进行播放
        PlayYUV();
    }

    void CreatePlayer() {
        if (!_renderWnd) {
            _renderWnd = static_cast<CMyWindowUI*>(_paintManager.FindControl(_T("videoDisplay")));
            HWND hwnd = _renderWnd->GetHwnd();
			RECT maxRect = _renderWnd->GetPos();
			//RECT rect;
			//// 设置窗口的最大分辨率
			//uint32_t window_width = maxRect.right - maxRect.left;
			//uint32_t window_height = maxRect.bottom - maxRect.top;
			//int rate = _width * window_height - _height * window_width;
			//if (rate < 0)
			//{
			//	uint32_t render_width = _width * window_height / _height;
			//	rect.left = maxRect.left + (window_width - render_width) / 2;
			//	rect.top = maxRect.top;
			//	rect.right = rect.left + render_width;
			//	rect.bottom = rect.top + window_height;
			//}
			//else if (rate > 0)
			//{
			//	uint32_t render_height = _height * window_width / _width;
			//	rect.left = maxRect.left;
			//	rect.top = maxRect.top + (window_height - render_height) / 2;
			//	rect.right = rect.left + window_width;
			//	rect.bottom = rect.top + render_height;
			//}
			//else
			//{
			//	rect.left = maxRect.left;
			//	rect.top = maxRect.top;
			//	rect.right = maxRect.right;
			//	rect.bottom = maxRect.bottom;
			//}
			
            _renderWnd->SetPos(maxRect);
			_renderWnd->SetEnabled(false);
			EnableWindow(hwnd, false);
            ::ShowWindow(hwnd, true);
            // _yuv_player = std::make_shared<D3D9Player>();
			_yuv_player = std::make_shared<SDLPlayer>();
            _yuv_player->SetWindow(hwnd);
        }
    }

    void PlayYUV() {
		if (_running) {
			return;
		}
		if (_work.joinable()) {
			_work.join();
		}
        _running = true;
        _work = std::thread([&]() {
            uint8_t* data = new uint8_t[_width * _height * 3 / 2];
            while (_running) {
				if (_fin.eof()) {
					_running = false;
					auto btnPlay = static_cast<DuiLib::CButtonUI*>(_paintManager.FindControl(_T("btnPlay")));
					btnPlay->SetEnabled(false);
					auto btnPause = static_cast<DuiLib::CButtonUI*>(_paintManager.FindControl(_T("btnPause")));
					btnPause->SetEnabled(false);
					break;
				}
                _fin.read((char*)data, _width * _height * 3 / 2);
                _yuv_player->Render(data, _width, _height);
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            }
        });
    }

    void StopPlay()
    {
		if (!_running) {
			return;
		}
		std::cout << "Stop" << std::endl;
        PausePlay();
        if (_data) {
            delete[] _data;
            _data = nullptr;
        }
    }

    void PausePlay() {
        if (!_running) {
            return;
        }
        _running = false;
        if (_work.joinable()) {
            _work.join();
        }
    }

public:
    HINSTANCE _hInstance;
    HICON _hIcon{};
    DuiLib::CPaintManagerUI _paintManager{};
    HWND _ownerWnd{};

    bool _running{ false };
    std::thread _work{};
    std::ifstream _fin{};
    std::string _filename{};
    uint32_t _width{};
    uint32_t _height{};
    CMyWindowUI* _renderWnd{};
    // std::shared_ptr<D3D9Player> _yuv_player{};
	std::shared_ptr<SDLPlayer> _yuv_player{};
    uint8_t* _data{};
    std::unordered_map<std::string, PixFormat> _formatMap;
    PixFormat _format;
	std::string _configFilename{};
};


YUVPlayer::YUVPlayer() : _pimpl(std::make_unique<YUVPlayerImpl>())
{

}

YUVPlayer::~YUVPlayer()
{

}

void YUVPlayer::Init()
{
    _pimpl->Init();
}

bool YUVPlayer::CreateDUIWindow()
{
    return _pimpl->CreateDUIWindow();
}

void YUVPlayer::ShowWindow()
{
    _pimpl->ShowWindow();
}