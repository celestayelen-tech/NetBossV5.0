/*
 * Copyright (C) 2025 Celeste Ayelen
 * Licensed under the GNU GPL v3.0.
 * You may not use this file except in compliance with the License.
 */
/*
 * NETBOSS v5.0 - GUI PROFESSIONAL EDITION
 * Arquitectura: Win32 API Nativa (Nada de frameworks pesados, solo C puro)
 * Capacidades: Escaneo Real + OSINT + Fingerprinting
 * Compilador: Requiere -lgdi32 -mwindows
 */

#include <winsock2.h> 
#include <windows.h>  
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include <process.h> 

// --- MARCA DE AGUA (BRANDING) ---
// Esto se compila dentro del .exe. Si alguien lo abre con un editor Hex, verá el nombre del autor del código.
volatile const char AUTHOR_SIGNATURE[] = 
    "\0\0\0[[ NETBOSS v5.0 - DEVELOPED BY CELESTE AYELEN - GPLv3 LICENSED ]]\0\0\0";

// Es un truco para que el compilador no elimine la firma por "optimización"
void _KeepSignature() {
    const char* p = AUTHOR_SIGNATURE; 
    (void)p;
}

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdi32.lib")

// --- CONFIGURACIÓN GUI ---
#define ID_BTN_SCAN 1
#define ID_EDIT_LOG 3

HWND hEditLog, hBtnScan;
HFONT hFont;

// --- FUNCIONES AUXILIARES (LOGGING) ---

void AppendLog(const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Magia de Win32 para inyectar texto al final sin borrar el historial
    int index = GetWindowTextLength(hEditLog);
    SendMessage(hEditLog, EM_SETSEL, (WPARAM)index, (LPARAM)index);
    SendMessage(hEditLog, EM_REPLACESEL, 0, (LPARAM)buffer);
    SendMessage(hEditLog, WM_VSCROLL, SB_BOTTOM, 0); // Auto-scroll para que baje solo
}

// Un parser JSON casero muy básico para extraer valores
void ExtractJsonValue(const char *json, const char *key, char *buffer, int bufSize) {
    char searchKey[64];
    sprintf(searchKey, "\"%s\":\"", key);
    char *start = strstr(json, searchKey);
    if (start) {
        start += strlen(searchKey);
        char *end = strchr(start, '"');
        if (end) {
            int len = end - start;
            if (len > bufSize - 1) len = bufSize - 1;
            strncpy(buffer, start, len);
            buffer[len] = '\0';
            return;
        }
    }
    strcpy(buffer, "N/A");
}

// --- LÓGICA DE RED (EL MOTOR) ---

// Se mide latencia real usando ICMP (Ping nativo de Windows)
DWORD GetLatency(IPAddr destIp) {
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) return 0;
    
    char SendData[32] = "NetBoss GUI Ping";
    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    LPVOID ReplyBuffer = malloc(ReplySize);
    
    // Se envía el paquete y esperamos hasta 1 seg
    DWORD dwRetVal = IcmpSendEcho(hIcmpFile, destIp, SendData, sizeof(SendData), NULL, ReplyBuffer, ReplySize, 1000);
    DWORD rtt = 0;
    if (dwRetVal != 0) {
        rtt = ((PICMP_ECHO_REPLY)ReplyBuffer)->RoundTripTime;
    }
    
    free(ReplyBuffer);
    IcmpCloseHandle(hIcmpFile);
    return rtt;
}

// Tocamos la puerta del Gateway para ver si es vulnerable
void FingerprintGatewayGUI(const char* gatewayIP) {
    SOCKET Sock;
    struct sockaddr_in destAddr;
    DWORD timeout = 1000;

    AppendLog("    [DEBUG] Escaneando Gateway %s...\r\n", gatewayIP);

    // ¿Está el puerto 80 abierto?
    Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Sock != INVALID_SOCKET) {
        setsockopt(Sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(Sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(80);
        inet_pton(AF_INET, gatewayIP, &destAddr.sin_addr);

        if (connect(Sock, (SOCKADDR*)&destAddr, sizeof(destAddr)) == 0) {
            AppendLog("    [!] PUERTO 80 (HTTP) ABIERTO - Inseguro\r\n");
        }
        closesocket(Sock);
    }
}

// OSINT: Le preguntamos a Internet quiénes somos realmente
void RunOSINTGUI(SOCKADDR *pLocalAddr) {
    SOCKET Sock;
    struct sockaddr_in destAddr;
    struct hostent *remoteHost;
    char recvBuf[4096];

    AppendLog("    [...] Consultando identidad en IP-API...\r\n");
    
    remoteHost = gethostbyname("ip-api.com");
    if (!remoteHost) return;

    Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Sock == INVALID_SOCKET) return;

    // Binding: Obligamos al socket a salir por ESTA interfaz específica
    if (bind(Sock, pLocalAddr, sizeof(struct sockaddr_in)) != 0) {
        closesocket(Sock); return;
    }

    destAddr.sin_family = AF_INET;
    destAddr.sin_addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];
    destAddr.sin_port = htons(80);

    if (connect(Sock, (SOCKADDR*)&destAddr, sizeof(destAddr)) == 0) {
        char request[] = "GET /json HTTP/1.1\r\nHost: ip-api.com\r\nUser-Agent: NetBoss_GUI/5.0\r\nConnection: close\r\n\r\n";
        send(Sock, request, strlen(request), 0);
        int iResult = recv(Sock, recvBuf, sizeof(recvBuf)-1, 0);
        
        if (iResult > 0) {
            recvBuf[iResult] = '\0';
            char ip[64], isp[64], country[64];
            ExtractJsonValue(recvBuf, "query", ip, sizeof(ip));
            ExtractJsonValue(recvBuf, "isp", isp, sizeof(isp));
            ExtractJsonValue(recvBuf, "country", country, sizeof(country));
            
            AppendLog("    [+] INFO PUBLICA:\r\n");
            AppendLog("        IP:  %s\r\n", ip);
            AppendLog("        ISP: %s\r\n", isp);
            AppendLog("        UBI: %s\r\n", country);
        }
    }
    closesocket(Sock);
}

// --- TRABAJO PESADO EN SEGUNDO PLANO (THREADS) ---

void ScanThread(void* pParams) {
    EnableWindow(hBtnScan, FALSE);
    SetWindowText(hEditLog, ""); 

    AppendLog("--- INICIANDO ESCANEO DE RED ---\r\n\r\n");

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurr = NULL;
    ULONG outBufLen = 15000;
    
    // Asigno memoria
    int i = 0;
    while (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW && i < 3) {
        pAddresses = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, outBufLen);
        i++;
    }

    // Comienza el análisis real
    pAddresses = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, outBufLen);
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAddresses, &outBufLen) == NO_ERROR) {
        pCurr = pAddresses;
        while (pCurr) {
           
            if (pCurr->IfType != IF_TYPE_SOFTWARE_LOOPBACK && pCurr->OperStatus == IfOperStatusUp) {
                AppendLog("=========================================\r\n");
                AppendLog("INTERFAZ: %S\r\n", pCurr->FriendlyName);
                
                char gwIP[64] = {0};
                if (pCurr->FirstGatewayAddress && pCurr->FirstGatewayAddress->Address.lpSockaddr->sa_family == AF_INET) {
                    inet_ntop(AF_INET, &((struct sockaddr_in *)pCurr->FirstGatewayAddress->Address.lpSockaddr)->sin_addr, gwIP, sizeof(gwIP));
                    AppendLog("GATEWAY:  %s\r\n", gwIP);
                } else {
                    AppendLog("GATEWAY:  No detectado\r\n");
                }

                PIP_ADAPTER_UNICAST_ADDRESS pUni = pCurr->FirstUnicastAddress;
                while (pUni) {
                    if (pUni->Address.lpSockaddr->sa_family == AF_INET) {
                        char localIP[64];
                        inet_ntop(AF_INET, &((struct sockaddr_in *)pUni->Address.lpSockaddr)->sin_addr, localIP, sizeof(localIP));
                        AppendLog(" -> IP LOCAL: %s\r\n", localIP);

                        // Check de conectividad (Bind a puerto 53)
                        SOCKET testSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        struct sockaddr_in dnsAddr;
                        dnsAddr.sin_family = AF_INET;
                        dnsAddr.sin_port = htons(53);
                        inet_pton(AF_INET, "8.8.8.8", &dnsAddr.sin_addr);

                        DWORD t = 2000;
                        setsockopt(testSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof(t));

                        if (bind(testSock, pUni->Address.lpSockaddr, sizeof(struct sockaddr_in)) == 0) {
                            if (connect(testSock, (SOCKADDR*)&dnsAddr, sizeof(dnsAddr)) == 0) {
                                AppendLog("    [OK] CONEXION VERIFICADA\r\n");
                                
                                DWORD lat = GetLatency(inet_addr("8.8.8.8"));
                                AppendLog("    [stats] Latencia Google: %lu ms\r\n", lat);
                                
                                RunOSINTGUI(pUni->Address.lpSockaddr);

                                if (strlen(gwIP) > 0) FingerprintGatewayGUI(gwIP);

                            } else {
                                AppendLog("    [X] SIN SALIDA A INTERNET\r\n");
                            }
                        }
                        closesocket(testSock);
                    }
                    pUni = pUni->Next;
                }
            }
            pCurr = pCurr->Next;
        }
    }

    if (pAddresses) HeapFree(GetProcessHeap(), 0, pAddresses);
    WSACleanup();
    AppendLog("\r\n--- FIN DEL ESCANEO ---\r\n");
    EnableWindow(hBtnScan, TRUE); // Reactivamos el botón
    _endthread();
}

// --- MAIN GUI (La ventana) ---

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE:
            // Botón de Escaneo
            hBtnScan = CreateWindow("BUTTON", "INICIAR AUDITORIA DE RED", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 
                10, 10, 560, 40, hwnd, (HMENU)ID_BTN_SCAN, NULL, NULL);
            
            // Caja de logs
            hEditLog = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 
                10, 60, 560, 300, hwnd, (HMENU)ID_EDIT_LOG, NULL, NULL);
            
            // Es una fuente monoespaciada para que parezca terminal
            hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, "Consolas");
            SendMessage(hEditLog, WM_SETFONT, (WPARAM)hFont, TRUE);
            break;
        
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_SCAN) _beginthread(ScanThread, 0, NULL);
            break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
        
        // --- EASTER EGG (F12) ---
        case WM_KEYUP:
            if (wParam == VK_F12) {
                MessageBox(hwnd, 
                    "NetBoss v5.0 Professional\n\n"
                    "Desarrollado por: Celeste Ayelen\n"
                    "Licencia: GNU GPL v3.0\n\n"
                    "\"La paranoia es solo un estado superior de conciencia.\"", 
                    "Autoría Verificada", 
                    MB_OK | MB_ICONINFORMATION);
            }
            break;
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR args, int nShow) {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), 0, WndProc, 0, 0, hInst, LoadIcon(NULL, IDI_SHIELD), LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1), NULL, "NetBossGUI", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow("NetBossGUI", "NetBoss v5.0 Pro", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 600, 420, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, nShow);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}