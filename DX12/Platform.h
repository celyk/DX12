#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace platform{ class Window; }
namespace gfx{
	bool createSwapChain(IDXGIFactory4* a, platform::Window* b);
	class Renderer;
}

namespace platform{

	class Window{
	public:
		Window(const int w, const int h);
		void show();

		enum class PollEvent{
			POLL_EVENT_QUIT,
			POLL_EVENT_NONE,
			POLL_EVENT_RESIZED
		};
		bool poll(Window::PollEvent& event);

		void setTitle(const char* cstr);
		double getTime();

		bool should_close = false;

		int width;
		int height;
	private:
		friend bool gfx::createSwapChain(IDXGIFactory4* a, platform::Window* b);
		friend class gfx::Renderer;

		friend class Input;

		double inverse_count_freq = 0.;
		LARGE_INTEGER start_count;

		HWND hwnd;
		MSG msg;
	};



	class Input{
	public:
		struct Button{
			bool down = false;
			bool changed = false;
		};

		Input(Window* w);
		void getButton(int code,Button& b);
	private:
		Window* window;
	};

}