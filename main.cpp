#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

int GrabKey(Display* display, Window grabWindow, int keycode) {
	unsigned int modifiers = Mod2Mask;
	Bool ownerEvents = True;
	int pointerMode = GrabModeAsync;
	int keyboardMode = GrabModeAsync;

	XGrabKey(display, keycode, modifiers, grabWindow, ownerEvents, pointerMode, keyboardMode);
	return keycode;
}

void UnGrabKey(Display* display, Window grabWindow, int keycode) {
	unsigned int modifiers = Mod2Mask;
	XUngrabKey(display, keycode, modifiers, grabWindow);
}

struct ClickArgs {
	bool isActive;
	bool isClicking;
	int delayInMs;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	Display* display;
	Window root;
	long int timer;
};

void* ClickLoop(void* active) {
	ClickArgs* args = (ClickArgs*)active;
	cout << "click thread started with args isActive=" << args->isActive << ", isClicking=" << args->isClicking << ", delayInMs=" << args->delayInMs << endl;

	while (args->isActive) {
		if (!args->isClicking) {
			pthread_mutex_lock(&(args->mutex));

			pthread_cond_wait(&(args->condition), &(args->mutex));

			pthread_mutex_unlock(&(args->mutex));
		}

		if (args->isActive) {
			Display* display = XOpenDisplay(NULL);
			XTestFakeButtonEvent(display, Button1, True, 0);
			XTestFakeButtonEvent(display, Button1, False, 0);
			XFlush(display);
			XCloseDisplay(display);
			this_thread::sleep_for(chrono::milliseconds(args->delayInMs));
		}
	}

	pthread_exit(0);
}

XKeyEvent CreateKeyEvent(Display* display, Window& win,
	Window& winRoot, bool press,
	int keycode, int modifiers) {
	XKeyEvent event;

	event.display = display;
	event.window = win;
	event.root = winRoot;
	event.subwindow = None;
	event.time = CurrentTime;
	event.x = 1;
	event.y = 1;
	event.x_root = 1;
	event.y_root = 1;
	event.same_screen = True;
	event.keycode = XKeysymToKeycode(display, keycode);
	event.state = modifiers;

	if (press)
		event.type = KeyPress;
	else
		event.type = KeyRelease;

	return event;
}

void* TimerLoop(void* active) {
	ClickArgs* args = (ClickArgs*)active;
	cout << "timer thread started with arg timer=" << args->timer << endl;
	chrono::steady_clock::time_point startTime = chrono::steady_clock::now();
	chrono::steady_clock::time_point currentTime;
	long int timeElapsed;
	bool ended = false;
	XKeyEvent fakePressEvent = CreateKeyEvent(args->display, args->root, args->root, true, XK_F9, 0);
	XKeyEvent fakeReleaseEvent = CreateKeyEvent(args->display, args->root, args->root, false, XK_F9, 0);

	while (args->isActive && !ended) {
		currentTime = chrono::steady_clock::now();
		timeElapsed = chrono::duration_cast<chrono::seconds>(currentTime - startTime).count();
		if (timeElapsed > args->timer) {
			cout << "end (timeout)" << endl;
			XSendEvent(args->display, args->root, true, KeyPressMask, (XEvent*)&fakePressEvent);
			XSendEvent(args->display, args->root, true, KeyPressMask, (XEvent*)&fakeReleaseEvent);
			XFlush(args->display);
			ended = true;
		} else {
			this_thread::sleep_for(chrono::milliseconds(1000));
		}
	}
	pthread_exit(0);
}

void PrintInstructions() {}

int main(int argc, char* argv[]) {
	bool hasTimer = false;
	long int timerSeconds = 0;
	if (argc > 1) {
		string arg = argv[1];
		long int parsedValue;
		try {
			size_t pos;
			parsedValue = stol(arg, &pos);
			if (pos < arg.size()) {
				cerr << "Argument " << arg << " has invalid characters" << endl;
				PrintInstructions();
				return 1;
			}
		} catch (invalid_argument const& ex) {
			cerr << "Invalid number " << arg << endl;
			PrintInstructions();
			return 1;
		} catch (out_of_range const& ex) {
			cerr << "Number out of range " << arg << endl;
			PrintInstructions();
			return 1;
		}
		timerSeconds = parsedValue;
		cout << "Parsed timer value " << timerSeconds << " seconds from command line" << endl;
		hasTimer = true;
	}

	Display* display = XOpenDisplay(0);
	Window root = DefaultRootWindow(display);
	XEvent event;
	ClickArgs clickArgs;

	clickArgs.isActive = true;
	clickArgs.isClicking = false;
	clickArgs.delayInMs = 100;
	clickArgs.mutex = PTHREAD_MUTEX_INITIALIZER;
	clickArgs.condition = PTHREAD_COND_INITIALIZER;
	clickArgs.display = display;
	clickArgs.root = root;
	clickArgs.timer = timerSeconds;

	unsigned int keycodeF8 = XKeysymToKeycode(display, XK_F8);
	GrabKey(display, root, keycodeF8);
	unsigned int keycodeF9 = XKeysymToKeycode(display, XK_F9);
	GrabKey(display, root, keycodeF9);

	XSelectInput(display, root, KeyPressMask);

	pthread_t clickThreadId;
	pthread_create(&clickThreadId, NULL, ClickLoop, &clickArgs);
	pthread_t timerThreadId;
	if (hasTimer) {
		pthread_create(&timerThreadId, NULL, TimerLoop, &clickArgs);
	}

	while (clickArgs.isActive) {
		XNextEvent(display, &event);
		if (event.type == KeyPress) {
			if (event.xkey.keycode == keycodeF9) {
				cout << "end (keypress)" << endl;
				clickArgs.isActive = false;
				if (!clickArgs.isClicking) {
					pthread_cond_signal(&(clickArgs.condition));
				}
			} else if (event.xkey.keycode == keycodeF8) {
				clickArgs.isClicking = !clickArgs.isClicking;
				cout << "toggled " << (clickArgs.isClicking ? "on" : "off") << endl;
				if (clickArgs.isClicking) {
					pthread_cond_signal(&(clickArgs.condition));
				}
			}
		}
	}

	pthread_join(clickThreadId, NULL);
	if (hasTimer) {
		pthread_join(timerThreadId, NULL);
	}

	XCloseDisplay(display);

	return 0;
}
