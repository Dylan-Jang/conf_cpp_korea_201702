// 출처: http://eternalwindows.jp/network/winsock/winsock07s.html

#include <stdio.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")


HWND g_hwndListBox = NULL;
BOOL g_bExitThread = FALSE;

SOCKET InitializeWinsock(LPSTR lpszPort);
bool AcceptSocket(int maxSocketCount, WSAPOLLFD* pfd, SOCKET listenSocket);
void ReceiveSocket(WSAPOLLFD* pfd, int fdIndex);
void ClosedSocket(WSAPOLLFD* pfd, int fdIndex);

DWORD WINAPI ThreadProc(LPVOID lpParamater);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lpszCmdLine, int nCmdShow)
{
	TCHAR      szAppName[] = TEXT("sample-server");
	HWND       hwnd;
	MSG        msg;
	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinst;
	wc.hIcon = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);
	wc.hCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szAppName;
	wc.hIconSm = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);

	if (RegisterClassEx(&wc) == 0) {
		return 0;
	}

	hwnd = CreateWindowEx(0, szAppName, szAppName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinst, NULL);
	if (hwnd == NULL) {
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HANDLE hThread = NULL;
	static SOCKET socListen = INVALID_SOCKET;

	switch (uMsg) {

	case WM_CREATE: {
		DWORD dwThreadId;

		g_hwndListBox = CreateWindowEx(0, TEXT("LISTBOX"), NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, 0, 0, 0, hwnd, (HMENU)1, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

		socListen = InitializeWinsock("32452");
		if (socListen == INVALID_SOCKET)
			return -1;

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, &socListen, 0, &dwThreadId);

		return 0;
	}

	case WM_SIZE:
		MoveWindow(g_hwndListBox, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;

	case WM_DESTROY:
		if (hThread != NULL) {
			g_bExitThread = TRUE;
			WaitForSingleObject(hThread, 1000);
			CloseHandle(hThread);
		}

		if (socListen != INVALID_SOCKET) {
			closesocket(socListen);
			WSACleanup();
		}

		PostQuitMessage(0);
		return 0;

	default:
		break;

	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI ThreadProc(LPVOID lpParamater)
{
	SOCKET    listenSocket = *((SOCKET *)lpParamater);
	int       i;
	int       nMaxSocketCount = 11;
	WSAPOLLFD fdArray[11];

	for (int i = 0; i < nMaxSocketCount; i++) {
		fdArray[i].fd = INVALID_SOCKET;
		fdArray[i].events = 0;
	}

	fdArray[0].fd = listenSocket;
	fdArray[0].events = POLLRDNORM;

	while (!g_bExitThread) {
		int nResult = WSAPoll(fdArray, nMaxSocketCount, 500);
		if (nResult < 0) {
			SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)TEXT("WSAPOLLFD 실행이 실패하였다"));
			break;
		}
		else if (nResult == 0) {
			continue;
		}

		for (i = 0; i < nMaxSocketCount; i++)
		{
			/*	if (fdArray[i].revents & POLLRDNORM)
					break;
				else if (fdArray[i].revents & POLLHUP)
					break;
			}*/

			if (fdArray[i].revents & POLLRDNORM) {
				if (fdArray[i].fd == listenSocket) {
					AcceptSocket(nMaxSocketCount, fdArray, listenSocket);
					/*int              nAddrLen;
					char             szBuf[256];
					char             szHostName[256];
					SOCKADDR_STORAGE sockAddr;

					for (i = 0; i < nMaxSocketCount; i++) {
						if (fdArray[i].fd == INVALID_SOCKET)
							break;
					}

					if (i == nMaxSocketCount)
						continue;

					nAddrLen = sizeof(SOCKADDR_STORAGE);
					fdArray[i].fd = accept(listenSocket, (LPSOCKADDR)&sockAddr, &nAddrLen);
					fdArray[i].events = POLLRDNORM;

					getnameinfo((LPSOCKADDR)&sockAddr, nAddrLen, szHostName, sizeof(szHostName), NULL, 0, 0);
					wsprintfA(szBuf, "No%d(%s) 접속", i, szHostName);
					SendMessageA(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);*/
				}
				else {
					ReceiveSocket(&fdArray[i], i);
					/*int   nResult;
					wchar_t szBuf[256];
					char szData[256] = { 0, };

					nResult = recv(fdArray[i].fd, (char *)szData, sizeof(szData), 0);

					char szSendMessage[256] = { 0, };
					sprintf_s(szSendMessage, 256 - 1, "Re:%s", szData);
					int nMsgLen = strnlen_s(szSendMessage, 256 - 1);

					wsprintf(szBuf, L"No%d %S", i, szSendMessage);
					SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);

					nResult = send(fdArray[i].fd, (char *)szSendMessage, nMsgLen, 0);*/
				}
			}
			else if (fdArray[i].revents & POLLHUP) {
				ClosedSocket(&fdArray[i], i);
				/*wchar_t szBuf[256];

				wsprintf(szBuf, L"No%d 접속 종료", i);
				SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);

				shutdown(fdArray[i].fd, SD_BOTH);
				closesocket(fdArray[i].fd);
				fdArray[i].fd = INVALID_SOCKET;
				fdArray[i].events = 0;*/
			}
		}
	}

	return 0;
}

SOCKET InitializeWinsock(LPSTR lpszPort)
{
	WSADATA    wsaData;
	ADDRINFO   addrHints;
	LPADDRINFO lpAddrList;
	SOCKET     socListen;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	ZeroMemory(&addrHints, sizeof(addrinfo));
	addrHints.ai_family = AF_INET;
	addrHints.ai_socktype = SOCK_STREAM;
	addrHints.ai_protocol = IPPROTO_TCP;
	addrHints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, lpszPort, &addrHints, &lpAddrList) != 0) {
		MessageBox(NULL, TEXT("호스트 정보에서 어드레스 취득이 실패하였다"), NULL, MB_ICONWARNING);
		WSACleanup();
		return INVALID_SOCKET;
	}

	socListen = socket(lpAddrList->ai_family, lpAddrList->ai_socktype, lpAddrList->ai_protocol);

	if (bind(socListen, lpAddrList->ai_addr, (int)lpAddrList->ai_addrlen) == SOCKET_ERROR) {
		MessageBox(NULL, TEXT("로컬 어드레스와 소켓 연결에 실패하였다"), NULL, MB_ICONWARNING);
		closesocket(socListen);
		freeaddrinfo(lpAddrList);
		WSACleanup();
		return INVALID_SOCKET;
	}

	if (listen(socListen, 1) == SOCKET_ERROR) {
		closesocket(socListen);
		freeaddrinfo(lpAddrList);
		WSACleanup();
		return INVALID_SOCKET;
	}

	freeaddrinfo(lpAddrList);

	return socListen;
}

bool AcceptSocket(int maxSocketCount, WSAPOLLFD* pfd, SOCKET listenSocket)
{
	int              nAddrLen;
	char             szBuf[256];
	char             szHostName[256];
	SOCKADDR_STORAGE sockAddr;

	int fdIndex = -1;

	for (int i = 0; i < maxSocketCount; i++) 
	{
		if (pfd[i].fd == INVALID_SOCKET) 
		{
			fdIndex = i;
			break;
		}
	}

	if (fdIndex == -1) {
		return false;
	}

	nAddrLen = sizeof(SOCKADDR_STORAGE);
	pfd[fdIndex].fd = accept(listenSocket, (LPSOCKADDR)&sockAddr, &nAddrLen);
	pfd[fdIndex].events = POLLRDNORM;

	getnameinfo((LPSOCKADDR)&sockAddr, nAddrLen, szHostName, sizeof(szHostName), NULL, 0, 0);
	wsprintfA(szBuf, "No%d(%s) 접속", fdIndex, szHostName);
	SendMessageA(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);

	return true;
}

void ReceiveSocket(WSAPOLLFD* pfd, int fdIndex)
{
	int   nResult;
	wchar_t szBuf[256];
	char szData[256] = { 0, };

	nResult = recv(pfd->fd, (char *)szData, sizeof(szData), 0);

	char szSendMessage[256] = { 0, };
	sprintf_s(szSendMessage, 256 - 1, "Re:%s", szData);
	int nMsgLen = (int)strnlen_s(szSendMessage, 256 - 1);

	wsprintf(szBuf, L"No%d %S", fdIndex, szSendMessage);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);

	nResult = send(pfd->fd, (char *)szSendMessage, nMsgLen, 0);
}

void ClosedSocket(WSAPOLLFD* pfd, int fdIndex)
{
	wchar_t szBuf[256];

	wsprintf(szBuf, L"No%d 접속 종료", fdIndex);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);

	shutdown(pfd->fd, SD_BOTH);
	closesocket(pfd->fd);
	pfd->fd = INVALID_SOCKET;
	pfd->events = 0;
}