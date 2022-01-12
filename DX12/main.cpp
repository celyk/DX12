#include"stdafx.h"

#include"Platform.h"
#include"Renderer.h"

int main(int argc, char* argv[]){
	platform::Window window(500,500);
	gfx::Renderer renderer;
	if( !renderer.init(&window) ) renderer.shutdown();

	window.show();

	platform::Window::PollEvent w_event = platform::Window::PollEvent::POLL_EVENT_NONE;
	while( w_event != platform::Window::PollEvent::POLL_EVENT_QUIT ){
		while( window.poll(w_event) ){
			switch( w_event ){
				case platform::Window::PollEvent::POLL_EVENT_RESIZED: 
					break;
			}
		}

		renderer.render();
	}
	
	renderer.shutdown();

	return 0;
}