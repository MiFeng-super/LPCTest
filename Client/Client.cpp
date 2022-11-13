// Client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "..\include\Veil\Veil.h"

constexpr const wchar_t* LPCPortName = L"\\LPCTest";
constexpr const size_t   MessageLen = 100;


LPVOID allocMessage(size_t msgsize)
{
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PORT_MESSAGE) + msgsize);
}


int main()
{
	SECURITY_QUALITY_OF_SERVICE sqos { 0 };

	UNICODE_STRING portName			{ 0 };
	NTSTATUS	   status			= 0;
	HANDLE		   hPort			= nullptr;
	ULONG		   MaxMessageLength = 0;
	LPVOID		   lpMem			= nullptr;
	LPVOID		   lpMem2			= nullptr;

	RtlInitUnicodeString(&portName, LPCPortName);

	status = NtConnectPort(&hPort, &portName, &sqos, nullptr, nullptr, &MaxMessageLength, nullptr, nullptr);

	std::cout << "[i] NtConnectPort: 0x" << std::hex << status << std::endl;

	if (NT_SUCCESS(status))
	{
		auto messageLength = MaxMessageLength - sizeof(PORT_MESSAGE);

		lpMem  = allocMessage(messageLength);
		lpMem2 = allocMessage(messageLength);

		if (lpMem)
		{
			auto* reqMsg  = (PORT_MESSAGE*)lpMem;
			auto* reqBuf   = (char*)lpMem + sizeof(PORT_MESSAGE);

			auto* replyMsg = (PORT_MESSAGE*)lpMem2;
			auto* replyBuf = (char*)lpMem2 + sizeof(PORT_MESSAGE);


			while (true)
			{
				std::cout << "[.] Request Message > ";

				fgets(reqBuf, messageLength, stdin);

				reqMsg->u1.s1.DataLength  = strlen(reqBuf) + 1;
				reqMsg->u1.s1.TotalLength = reqMsg->u1.s1.DataLength + sizeof(PORT_MESSAGE);

				replyMsg->u1.s1.DataLength  = messageLength;
				replyMsg->u1.s1.TotalLength = replyMsg->u1.s1.DataLength + sizeof(PORT_MESSAGE);

				status = NtRequestWaitReplyPort(hPort, reqMsg, replyMsg);

				if (!NT_SUCCESS(status))
				{
					break;
				}

				std::cout << "[i] Reply Message > " << replyBuf << std::endl;

			}

			HeapFree(GetProcessHeap(), 0, lpMem);
			HeapFree(GetProcessHeap(), 0, lpMem2);
		}

		NtClose(hPort);
	}
}

