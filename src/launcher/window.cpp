#include <std_include.hpp>
#include "window.hpp"

window::window(const std::string& title, int width, int height)
{
	const auto handle = GetModuleHandle(nullptr);

	ZeroMemory(&this->wc_, sizeof(this->wc_));

	this->wc_.cbSize = sizeof(this->wc_);
	this->wc_.style = CS_HREDRAW | CS_VREDRAW;
	this->wc_.lpfnWndProc = window::static_processor;
	this->wc_.hInstance = handle;
	this->wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
	this->wc_.hIcon = LoadIcon(handle, MAKEINTRESOURCE(102));
	this->wc_.hIconSm = this->wc_.hIcon;
	this->wc_.hbrBackground = CreateSolidBrush(RGB(35, 35, 35));//HBRUSH(COLOR_WINDOW);
	this->wc_.lpszClassName = L"omw3_window";
	RegisterClassEx(&this->wc_);

	const auto x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	const auto y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

	this->handle_ = CreateWindowExA(NULL, "omw3_window", title.data(), (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), x, y, width,
	                                height, nullptr, nullptr, handle, nullptr);

	SetWindowLongPtrA(*this, GWLP_USERDATA, LONG_PTR(this));
}

window::~window()
{
	this->close();
	UnregisterClass(this->wc_.lpszClassName, this->wc_.hInstance);
	DeleteObject(this->wc_.hbrBackground);
}

void window::close()
{
	if (!this->handle_) return;

	SendMessageA(this->handle_, WM_KILL_WINDOW, NULL, NULL);
	this->handle_ = nullptr;
}

void window::run() const
{
	MSG msg;
	while (this->handle_ && IsWindow(*this))
	{
		if (PeekMessageA(&msg, nullptr, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		else
		{
			std::this_thread::sleep_for(1ms);
		}
	}
}

LRESULT CALLBACK window::processor(UINT message, WPARAM w_param, LPARAM l_param) const
{
	if (message == WM_KILL_WINDOW)
	{
		DestroyWindow(*this);
		return 0;
	}

	return DefWindowProc(*this, message, w_param, l_param);
}

LRESULT CALLBACK window::static_processor(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
	window* self = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (self) return self->processor(message, w_param, l_param);
	return DefWindowProc(hwnd, message, w_param, l_param);
}


window::operator HWND() const
{
	return this->handle_;
}
