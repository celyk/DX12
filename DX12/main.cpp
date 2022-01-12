#include"stdafx.h"

#include"Platform.h"

int main(int argc, char* argv[]){
	platform::Window window(500,500);
	window.show();

	platform::Window::PollEvent w_event = platform::Window::PollEvent::POLL_EVENT_NONE;
	while( w_event != platform::Window::PollEvent::POLL_EVENT_QUIT ){
		while( window.poll(w_event) ){
			switch( w_event ){
				case platform::Window::PollEvent::POLL_EVENT_RESIZED: 
					break;
			}
		}


		//renderer::

	}
	
	return 0;
}