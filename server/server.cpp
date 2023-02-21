// server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <winsock2.h>
#include <windows.h>
#include "../include/protocol.h"
#pragma comment(lib,"ws2_32.lib")

using namespace std;

void InitWs2_32();


struct tagInfo
{
	tagInfo(char* szName, sockaddr_in si,SOCKET sock):m_si(si),m_sock(sock)
	{
		strcpy(m_szName, szName);
	}

	char		m_szName[NAMELEN];				//姓名
	sockaddr_in m_si;							//ip和port
	SOCKET		m_sock;							//cs之间的sock
};


int main()
{
	InitWs2_32();

	SOCKET sockAccept = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockAccept == INVALID_SOCKET)
	{
		cout << "sock 创建失败" << endl;
	}

	sockaddr_in si = {};
	si.sin_family = AF_INET;
	si.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	si.sin_port = htons(5555);
	int nRet = bind(sockAccept, (sockaddr*)&si, sizeof(si));
	if (nRet == SOCKET_ERROR)
	{
		cout << "端口绑定失败" << endl;
	}

	nRet = listen(sockAccept, SOMAXCONN);
	if (nRet != SOCKET_ERROR)
	{
		cout << "开始监听：" << endl;
	}

	list<tagInfo> lst;//保存在线客户端的链表

	while (true)
	{
		sockaddr_in siClient = {};
		int nLen = sizeof(siClient);
		SOCKET sock = accept(sockAccept, (sockaddr*)&siClient, &nLen);

		if (sock != INVALID_SOCKET)
		{
			printf("ip:%s port:%d 连接上服务器了\r\n", inet_ntoa(siClient.sin_addr), ntohs(siClient.sin_port));

			//开启线程，收发数据
			thread([siClient,sock,&lst]() 
			{
				while (true)
				{
					shared_ptr<ProtoHeader> ptrPkgFromClient =  CTcp(sock).RecvPackage();
					if (ptrPkgFromClient == nullptr)
					{
						printf("ip:%s port:%d 掉线了\r\n", inet_ntoa(siClient.sin_addr), ntohs(siClient.sin_port));

						char szName[NAMELEN] = {};
						//从在线列表中剔除
						for (auto it = lst.begin(); it != lst.end(); it++)
						{
							if (it->m_si == siClient)
							{
								strcpy(szName, it->m_szName);
								lst.erase(it);
								break;
							}
						}

						//告诉在线客户端，这个客户端下线
						for (auto info : lst)
						{
							//下线包
							shared_ptr<ProtoHeader> ptrPkgOffline(new S2CLogout(S2C_LOGOUT, szName,siClient));
							CTcp(info.m_sock).SendPackage(ptrPkgOffline);
						}

						//当前在线列表
						cout << "\r\n===========当前在线列表=============" << endl;
						for (auto info : lst)
						{
							printf("ip:%s port:%d name:%s \r\n", inet_ntoa(info.m_si.sin_addr), ntohs(info.m_si.sin_port), info.m_szName);
						}
						cout << "===========当前在线列表=============\r\n" << endl;

						break;
					}
					switch (ptrPkgFromClient->m_cmd)
					{
						case C2S_LOGIN:
						{
							//解包
							C2SLogin* pPkg = (C2SLogin*)ptrPkgFromClient.get();

							printf("ip:%s port:%d name:%s 上线了\r\n", inet_ntoa(siClient.sin_addr), ntohs(siClient.sin_port),pPkg->m_szName);
							
							//告诉在线客户端，有新客户端上线了
							for (auto info : lst)
							{
								//构造转发包
								shared_ptr<ProtoHeader> ptrToClient(new S2CLogin(S2C_LOGIN,pPkg->m_szName,siClient));
								CTcp(info.m_sock).SendPackage(ptrToClient);
							}

							//告诉当前客户端，在线的客户端列表
							for (auto info : lst)
							{
								shared_ptr<ProtoHeader> ptrToClient(new S2CLogin(S2C_LOGIN, info.m_szName, info.m_si));
								CTcp(sock).SendPackage(ptrToClient);
							}
							
							//将当前客户端加入到在线客户端列表
							lst.push_back({ pPkg->m_szName,siClient,sock });

							//当前在线列表
							cout << "\r\n===========当前在线列表=============" << endl;
							for (auto info : lst)
							{
								printf("ip:%s port:%d name:%s \r\n", inet_ntoa(info.m_si.sin_addr), ntohs(info.m_si.sin_port), info.m_szName);
							}
							cout << "===========当前在线列表=============\r\n" << endl;

							break;
						}

						case C2S_LOGOUT:
						{
							C2SLogout* pPkg = (C2SLogout*)ptrPkgFromClient.get();
							printf("ip:%s port:%d name:%s 下线了\r\n", inet_ntoa(siClient.sin_addr), ntohs(siClient.sin_port), pPkg->m_szName);

							//从在线列表中剔除
							for (auto it = lst.begin(); it != lst.end(); it++)
							{
								if (it->m_si == siClient)
								{
									lst.erase(it);
									break;
								}
							}

							//告诉在线客户端，这个客户端下线
							for (auto info : lst)
							{
								//下线包
								shared_ptr<ProtoHeader> ptrPkgOffline(new S2CLogout(S2C_LOGOUT, pPkg->m_szName, siClient));
								CTcp(info.m_sock).SendPackage(ptrPkgOffline);
							}

							//当前在线列表
							cout << "\r\n===========当前在线列表=============" << endl;
							for (auto info : lst)
							{
								printf("ip:%s port:%d name:%s \r\n", inet_ntoa(info.m_si.sin_addr), ntohs(info.m_si.sin_port), info.m_szName);
							}
							cout << "===========当前在线列表=============\r\n" << endl;

							break;
						}

						case C2S_PUBCHAT:
						{
							C2SPubChat* pPkg = (C2SPubChat*)ptrPkgFromClient.get();

							//日志
							printf("ip:%s port:%d name:%s 说:%s\r\n",
								inet_ntoa(siClient.sin_addr), ntohs(siClient.sin_port),
								pPkg->m_szName,
								pPkg->m_szMsg
							);

							for (auto info : lst)
							{
								shared_ptr<ProtoHeader> ptrPkg(new S2CPubChat(S2C_PUBCHAT,pPkg->m_szName,pPkg->m_szMsg,siClient));
								int nRet = CTcp(info.m_sock).SendPackage(ptrPkg);
							}

							break;
						}

						case C2S_PRICHAT:
						{
							C2SPriChat* pPkg = (C2SPriChat*)ptrPkgFromClient.get();

							//日志
							printf("ip:%s port:%d name:%s 对 ip:%s port:%d 私聊说:%s\r\n",
								inet_ntoa(siClient.sin_addr), ntohs(siClient.sin_port),
								pPkg->m_szName,
								inet_ntoa(pPkg->m_siTo.sin_addr), ntohs(pPkg->m_siTo.sin_port),
								pPkg->m_szMsg
							);

							char szNameTo[NAMELEN] = {};
							sockaddr_in siTo = {};

							//把消息发送给私聊对象
							for (auto info:lst)
							{
								if (info.m_si == pPkg->m_siTo)
								{
									//printf("私聊对象是：ip:%s port:%d name:%s\r\n", inet_ntoa(info.m_si.sin_addr), ntohs(info.m_si.sin_port), info.m_szName);
									shared_ptr<ProtoHeader> ptrPkg(new S2CPriChat(S2C_PRICHAT, pPkg->m_szName,siClient,info.m_szName,info.m_si,pPkg->m_szMsg));
									CTcp(info.m_sock).SendPackage(ptrPkg);

									strcpy(szNameTo, info.m_szName);
									siTo = info.m_si;
									break;
								}
							}


							//把消息转发给自己
							shared_ptr<ProtoHeader> ptrPkg(new S2CPriChat(S2C_PRICHAT, pPkg->m_szName, siClient, szNameTo, siTo, pPkg->m_szMsg));
							CTcp(sock).SendPackage(ptrPkg);
							break;
						}
					}
				}
			}).detach();
		}
	}
	closesocket(sockAccept);
}




void InitWs2_32()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		WSACleanup();
		return;
	}

	/* The WinSock DLL is acceptable. Proceed. */

}