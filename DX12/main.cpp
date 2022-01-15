#include"stdafx.h"

#include"Platform.h"
#include"Renderer.h"

int main(int argc, char* argv[]){
	platform::Window window(500, 500);
	window.show();

	gfx::Renderer renderer;
	if( !renderer.init(&window) ){
		printf("error");
		return -1;
	}

	platform::Window::PollEvent w_event = platform::Window::PollEvent::POLL_EVENT_NONE;
	while( w_event != platform::Window::PollEvent::POLL_EVENT_QUIT ){
		while( window.poll(w_event) ){
			switch( w_event ){
				case platform::Window::PollEvent::POLL_EVENT_RESIZED: 
					break;
			}
		}

		//printf("render");
		if( !renderer.render() ){
			printf("error");
			return -1;
		}
	}

	renderer.shutdown();

	return 0;
}