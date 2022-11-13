// Server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include "..\include\Veil\Veil.h"

constexpr const wchar_t* LPCPortName = L"\\LPCTest";
constexpr const size_t   MessageLen  = 100;


LPVOID allocMessage(size_t msgsize) 
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PORT_MESSAGE) + msgsize);
}


void serverThread() 
{
    UNICODE_STRING      portName { 0 };
    OBJECT_ATTRIBUTES   objAttr  { 0 };
    NTSTATUS            status  = 0;
    HANDLE              hPort   = nullptr;
    HANDLE              hClient = nullptr;
    LPVOID              lpMem   = nullptr;

    RtlInitUnicodeString(&portName, LPCPortName);
    InitializeObjectAttributes(&objAttr, &portName, 0, nullptr, nullptr);

    status = NtCreatePort(&hPort, &objAttr, 0, sizeof(PORT_MESSAGE) + MessageLen, 0);

	std::cout << "[i] NtCreatePort: 0x" << std::hex << status << std::endl;

    if (NT_SUCCESS(status))
    {
        lpMem = allocMessage(MessageLen);
        if (lpMem)
        {
            auto* message = (PORT_MESSAGE*)lpMem;
            auto* buffer  = (char*)lpMem + sizeof(PORT_MESSAGE);

            while (true)
            {
				status = NtReplyWaitReceivePort(hPort, nullptr, nullptr, message);
                if (!NT_SUCCESS(status))
                {
                    break;
                }

                switch (message->u2.s2.Type)
                {
				    case LPC_CONNECTION_REQUEST:

                        std::cout << "[i] NtReplyWaitReceivePort: LPC_CONNECTION_REQUEST" << std::endl;

                        if (hClient == nullptr)
                        {
							status = NtAcceptConnectPort(&hClient, nullptr, message, true, nullptr, nullptr);
							if (NT_SUCCESS(status))
							{
								status = NtCompleteConnectPort(hClient);

								if (!NT_SUCCESS(status))
								{
                                    NtClose(hClient);
                                    hClient = nullptr;
								}
							}
                        }

					    break;


                    case LPC_REQUEST:

                        std::cout << "[i] NtReplyWaitReceivePort: LPC_REQUEST" << std::endl;
                        std::cout << "[i] Received Data: " << buffer << std::endl;

                        NtReplyPort(hPort, message);

                        break;

                    case LPC_PORT_CLOSED:
                    case LPC_CLIENT_DIED:

						std::cout << "[i] NtReplyWaitReceivePort: LPC_PORT_CLOSED or LPC_CLIENT_DIED" << std::endl;
                        
                        if (hClient)
                        {
                            NtClose(hClient);
                            hClient = nullptr;
                        }

                        break;
                }
            }


            HeapFree(GetProcessHeap(), 0, lpMem);
        }

        NtClose(hPort);
    }
}


int main()
{
    std::thread t (serverThread);

    t.join();

    return 0;
}

