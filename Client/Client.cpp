// Client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "..\include\Veil\Veil.h"

constexpr const wchar_t* LPCPortName = L"\\LPCTest";
constexpr const size_t   MessageLen  = 0x1000;


LPVOID allocMessage(size_t msgsize)
{
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PORT_MESSAGE) + msgsize);
}


void client1() 
{
	SECURITY_QUALITY_OF_SERVICE sqos{ 0 };

	UNICODE_STRING portName{ 0 };
	NTSTATUS	   status = 0;
	HANDLE		   hPort = nullptr;
	ULONG		   MaxMessageLength = 0;
	LPVOID		   lpMem = nullptr;
	LPVOID		   lpMem2 = nullptr;

	RtlInitUnicodeString(&portName, LPCPortName);

	status = NtConnectPort(&hPort, &portName, &sqos, nullptr, nullptr, &MaxMessageLength, nullptr, nullptr);

	std::cout << "[i] NtConnectPort: 0x" << std::hex << status << std::endl;

	if (NT_SUCCESS(status))
	{
		auto messageLength = MaxMessageLength - sizeof(PORT_MESSAGE);

		lpMem = allocMessage(messageLength);
		lpMem2 = allocMessage(messageLength);

		if (lpMem)
		{
			auto* reqMsg = (PORT_MESSAGE*)lpMem;
			auto* reqBuf = (char*)lpMem + sizeof(PORT_MESSAGE);

			auto* replyMsg = (PORT_MESSAGE*)lpMem2;
			auto* replyBuf = (char*)lpMem2 + sizeof(PORT_MESSAGE);


			while (true)
			{
				std::cout << "[.] Request Message > ";

				fgets(reqBuf, messageLength, stdin);

				reqMsg->u1.s1.DataLength = strlen(reqBuf) + 1;
				reqMsg->u1.s1.TotalLength = reqMsg->u1.s1.DataLength + sizeof(PORT_MESSAGE);

				replyMsg->u1.s1.DataLength = messageLength;
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


void client2() 
{
	SECURITY_QUALITY_OF_SERVICE sqos  { 0 };
	LARGE_INTEGER		sectionLength { MessageLen };
	UNICODE_STRING		portName	  { 0 };
	PORT_VIEW			clientView	  { 0 };
	REMOTE_PORT_VIEW	serverView	  { 0 };
	CHAR				szInput[100]  { 0 };
	HANDLE				hSection = nullptr;
	HANDLE				hPort	 = nullptr;
	NTSTATUS			status	 = 0;

	status = NtCreateSection(&hSection, SECTION_MAP_READ | SECTION_MAP_WRITE, nullptr, &sectionLength, PAGE_READWRITE, SEC_COMMIT, nullptr);
	
	std::cout << "[i] NtCreateSection: 0x" << std::hex << status << std::endl;

	if (NT_SUCCESS(status))
	{
		RtlInitUnicodeString(&portName, LPCPortName);

		sqos.Length				 = sizeof(SECURITY_QUALITY_OF_SERVICE);
		sqos.ImpersonationLevel  = SecurityImpersonation;
		sqos.EffectiveOnly		 = FALSE;
		sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
		
		clientView.Length		 = sizeof(clientView);
		clientView.SectionHandle = hSection;
		clientView.SectionOffset = 0;
		clientView.ViewSize		 = MessageLen;
		serverView.Length		 = sizeof(serverView);

		status = NtConnectPort(&hPort, &portName, &sqos, &clientView, &serverView, nullptr, nullptr, nullptr);

		std::cout << "[i] NtConnectPort: 0x" << std::hex << status << std::endl;

		if (NT_SUCCESS(status))
		{
			std::cout << "[i] serverView.ViewBase: 0x" << std::hex << serverView.ViewBase << std::endl;

			PORT_MESSAGE reqMessage   { 0 };
			PORT_MESSAGE replyMessage { 0 };

			reqMessage.u1.s1.DataLength		= 0;
			reqMessage.u1.s1.TotalLength	= sizeof(PORT_MESSAGE);

			replyMessage.u1.s1.DataLength   = 0;
			replyMessage.u1.s1.TotalLength  = sizeof(PORT_MESSAGE);

			while (true)
			{
				std::cout << "[.] Request Message > ";
					
				fgets(szInput, sizeof(szInput), stdin);

				strcpy_s((char*)clientView.ViewBase, clientView.ViewSize, szInput);

				status = NtRequestWaitReplyPort(hPort, &reqMessage, &replyMessage);
				// status = NtRequestPort(hPort, &reqMessage);

				std::cout << "[i] NtRequestWaitReplyPort: 0x" << std::hex << status << std::endl;
				std::cout << "[i] Return Data: " << std::hex << (char*)clientView.ViewBase << std::endl;
			}

			NtClose(hPort);
		}

		NtClose(hSection);
	}


}


int main()
{
	// client1();
	client2();

	getchar();
}

