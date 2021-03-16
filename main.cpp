#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

int GrabKey(Display *display, Window grabWindow, int keycode){
	unsigned int modifiers = Mod2Mask;
	Bool ownerEvents = True;
	int pointerMode = GrabModeAsync;
	int keyboardMode = GrabModeAsync;

	XGrabKey(display, keycode, modifiers, grabWindow, ownerEvents, pointerMode, keyboardMode);
	return keycode;
}

void UnGrabKey(Display *display, Window grabWindow, int keycode){
	unsigned int modifiers = Mod2Mask;
	XUngrabKey(display, keycode, modifiers, grabWindow);
}

struct ClickArgs{
	bool isActive;
	bool isClicking;
	int delayInMs;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	Display *display;
};

void *ClickLoop(void *active){
	ClickArgs *args = (ClickArgs*)active;
	cout << "thread started with args" << args->isActive << ", " << args->isClicking << ", " << args->delayInMs << "\n";

	while(args->isActive){
		if(!args->isClicking){
			pthread_mutex_lock(&(args->mutex));

			pthread_cond_wait(&(args->condition), &(args->mutex));

			pthread_mutex_unlock(&(args->mutex));
		}

		if(args->isActive){
			// cout << "clicked\n";
			Display *display = XOpenDisplay(NULL);
			XTestFakeButtonEvent(display, Button1, True, 0);
			XTestFakeButtonEvent(display, Button1, False, 0);
			XFlush(display);
			XCloseDisplay(display);
			this_thread::sleep_for(chrono::milliseconds(args->delayInMs));
		}
	}

	pthread_exit(0);
}

int main(int argc, char *argv[]){
	Display *display = XOpenDisplay(0);
	Window root = DefaultRootWindow(display);
	XEvent event;
	ClickArgs clickArgs;

	clickArgs.isActive = true;
	clickArgs.isClicking = false;
	clickArgs.delayInMs = 500;
	clickArgs.mutex = PTHREAD_MUTEX_INITIALIZER;
	clickArgs.condition = PTHREAD_COND_INITIALIZER;
	clickArgs.display = display;

	unsigned int keycodeF8 = XKeysymToKeycode(display, XK_F8);
	GrabKey(display, root, keycodeF8);

	unsigned int keycodeF9 = XKeysymToKeycode(display, XK_F9);
	GrabKey(display, root, keycodeF9);

	pthread_t clickThreadId;
	pthread_create(&clickThreadId, NULL, ClickLoop, &clickArgs);

	XSelectInput(display, root, KeyPressMask);

	while(clickArgs.isActive){
		XNextEvent(display, &event);
		if(event.type == KeyPress){
			if(event.xkey.keycode == keycodeF9){
				cout << "end\n";
				clickArgs.isActive = false;
				if(!clickArgs.isClicking){
					pthread_cond_signal(&(clickArgs.condition));
				}
			}else if(event.xkey.keycode == keycodeF8){
				cout << "toggle\n";
				clickArgs.isClicking = !clickArgs.isClicking;
				if(clickArgs.isClicking){
					pthread_cond_signal(&(clickArgs.condition));
				}
			}
		}
	}

	pthread_join(clickThreadId, NULL);

	XCloseDisplay(display);

	return 0;
}