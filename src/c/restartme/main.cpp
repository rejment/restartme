#define UNICODE
#include <windows.h>
#include <memory.h>
#include <jni.h>

#pragma intrinsic(strlen, memcpy)

extern "C" {
JNIEXPORT jintArray JNICALL Java_com_rejment_restartme_ProcessEnvironment_getProcesses(JNIEnv *, jobject, jstring);
JNIEXPORT jint JNICALL Java_com_rejment_restartme_ProcessEnvironment_sendMessage(JNIEnv *, jobject, jint, jstring);
JNIEXPORT jint JNICALL Java_com_rejment_restartme_ProcessEnvironment_register(JNIEnv *, jobject, jstring);
}

/* Window class name for all message windows in this IPC library */
#define CLASSNAME L"rejment/restartme"
/* We only react to messages with this message id */
#define MESSAGE_ID 666
/* Maximum number of processes that we will return */
#define MAX_PROCESSES 256

/**
 * Structure to pass JVM information to the message pump thread.
 */
struct ThreadParams
{
	JNIEnv *env;
	JavaVM *vm;
	jobject obj;
	wchar_t * name;
	jmethodID callback;
	HANDLE event;
	jint proc;
};

/**
 * Message window event handler. Only WM_COPYDATA is handled.
 */
LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COPYDATA)
	{
		COPYDATASTRUCT *cds = (COPYDATASTRUCT*) lParam;
		if (MESSAGE_ID == cds->dwData) {
			ThreadParams *tp = (ThreadParams *) GetWindowLongPtrA(hwnd, GWLP_USERDATA);
			jstring message = tp->env->NewString((jchar *) cds->lpData, lstrlen((wchar_t *) cds->lpData));
			jint result = tp->env->CallIntMethod(tp->obj, tp->callback, message);
			return result;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * The message pump thread for the message window. The start parameter must be a pointer to a ThreadParams structure.
 * The thread will signal the event in ThreadParams.event when the message is created and the window ID is in ThreadParams.proc.
 */
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    DragAcceptFiles(
	ThreadParams *tp = (ThreadParams *) lpParameter;
	tp->vm->AttachCurrentThreadAsDaemon((void**) &tp->env, 0);
	jclass cls = tp->env->GetObjectClass(tp->obj);
	tp->callback = tp->env->GetMethodID(cls, "messageReceived", "(Ljava/lang/String;)I");
	WNDCLASS wndcls = {0, (WNDPROC) WndProc, 0, 0, 0, 0, 0, 0, 0, CLASSNAME};
	RegisterClass(&wndcls);
	HWND msgwnd = CreateWindow(CLASSNAME, tp->name, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, 0, 0);
	SetWindowLongPtrA(msgwnd, GWLP_USERDATA, (LONG) tp);
	tp->proc = (jint) msgwnd;
	SetEvent(tp->event);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) DispatchMessage(&msg);
	return 0;
}

/**
 * Finds all message windows with the specified window name. Returns a Java int array.
 * No more than 256 window ids will be returned.
 */
JNIEXPORT jintArray JNICALL Java_com_rejment_restartme_ProcessEnvironment_getProcesses(JNIEnv *env, jobject obj, jstring name)
{
	jboolean l_IsCopy;
	const jchar *l_name = env->GetStringChars(name, &l_IsCopy);

	HWND others[MAX_PROCESSES];
	int count = 0;

	HWND found = FindWindowEx(HWND_MESSAGE, NULL, CLASSNAME, l_name);
	while (found != 0) {
		others[count++] = found;
		if (count == MAX_PROCESSES) break;
		found = FindWindowEx(HWND_MESSAGE, found, CLASSNAME, l_name);
	}

    env->ReleaseStringChars(name, l_name);

	jintArray result = env->NewIntArray(count);
	env->SetIntArrayRegion(result, 0, count, (jint*)others);
	return result;
}

/**
 * Creates a message window with the specified name for the calling JVM. 
 * Will also spawn a thread for the message window pump.
 */
JNIEXPORT jint JNICALL Java_com_rejment_restartme_ProcessEnvironment_register(JNIEnv *env, jobject obj, jstring name)
{
	DWORD thread;

	jboolean l_IsCopy;
	const jchar *l_name = env->GetStringChars(name, &l_IsCopy);

	ThreadParams *tp = (ThreadParams *) malloc(sizeof(ThreadParams));
	tp->env = env;
	env->GetJavaVM(&tp->vm);
	int len = (int) lstrlen(l_name)+1;
	tp->name = (wchar_t*) malloc(len * sizeof(wchar_t));
	lstrcpy(tp->name, l_name);
	tp->obj = env->NewGlobalRef(obj);
	tp->event = CreateEvent(NULL, FALSE, FALSE, NULL);
	env->ReleaseStringChars(name, l_name);

	CreateThread(NULL, 0, ThreadProc, tp, 0, &thread);

	WaitForSingleObject(tp->event, INFINITE);
	CloseHandle(tp->event);

	return tp->proc;
}

/**
 * Sends a message to the specified message window. May timeout and fail.
 * Returns -1 on success.
 */
JNIEXPORT jint JNICALL Java_com_rejment_restartme_ProcessEnvironment_sendMessage(JNIEnv *env, jobject obj, jint wnd, jstring msg)
{
	jboolean l_IsCopy;
	const jchar *l_msg = env->GetStringChars(msg, &l_IsCopy);

	COPYDATASTRUCT cds;
	cds.cbData = (DWORD) lstrlen(l_msg)+1;
	cds.dwData = MESSAGE_ID;
	cds.lpData = (void*)l_msg;

	DWORD dwResult;
	LRESULT result = SendMessageTimeout((HWND)wnd, WM_COPYDATA, 0, (LPARAM) &cds, SMTO_ABORTIFHUNG, 5000, &dwResult); 
	if (result == 0 && GetLastError() == 0)
		dwResult = -1;
	env->ReleaseStringChars(msg, l_msg);
	return dwResult;
}


extern "C" BOOL WINAPI _DllMainCRTStartup(HANDLE  hDllHandle, DWORD   dwReason, LPVOID  lpreserved)
{
	return TRUE;
}
