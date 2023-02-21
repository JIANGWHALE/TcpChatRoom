#pragma once
#include <WinSock2.h>
#include <memory>
#define NAMELEN 32
#define MSGLEN 255

using namespace std;

/**
 * @brief 消息包类型
*/

enum Cmd
{
	C2S_LOGIN,					//客户端上线包
	S2C_LOGIN,					//发送给客户端

	C2S_LOGOUT,					//下线
	S2C_LOGOUT,					//下线

	C2S_PUBCHAT,				//公聊
	S2C_PUBCHAT,				//私聊

	C2S_PRICHAT,				//私聊
	S2C_PRICHAT,				//私聊
};

#pragma pack(push)
#pragma pack(1)
/**
 * @brief 协议包头
*/
struct ProtoHeader
{
	ProtoHeader() {};
	ProtoHeader(Cmd cmd,size_t nLen):m_cmd(cmd),m_nLen(nLen){};

	Cmd			m_cmd;					//消息类型
	size_t		m_nLen;					//包体长度
};

/**
 * @brief 客户端上线包
*/
struct C2SLogin :public ProtoHeader
{
	C2SLogin() {};
	C2SLogin(Cmd cmd, char* szName) :ProtoHeader(cmd, sizeof(C2SLogin) - sizeof(ProtoHeader))
	{
		strcpy(m_szName, szName);
	};
	char m_szName[NAMELEN];
};

/**
 * @brief 服务器转发上线包
*/
struct S2CLogin :public ProtoHeader
{
	S2CLogin() {};
	S2CLogin(Cmd cmd, char* szName,sockaddr_in siFrom) :ProtoHeader(cmd, sizeof(S2CLogin) - sizeof(ProtoHeader)),m_siFrom(siFrom)
	{
		strcpy(m_szName, szName);
	};
	char m_szName[NAMELEN];
	sockaddr_in m_siFrom;
};

/**
 * @brief 客户端下线包
*/
struct C2SLogout :public ProtoHeader
{
	C2SLogout() {};
	C2SLogout(Cmd cmd, char* szName) :ProtoHeader(cmd, sizeof(C2SLogout) - sizeof(ProtoHeader))
	{
		strcpy(m_szName, szName);
	};
	char m_szName[NAMELEN];
};

/**
 * @brief 服务器转发下线包
*/
struct S2CLogout :public ProtoHeader
{
	S2CLogout() {};
	S2CLogout(Cmd cmd, char* szName, sockaddr_in siFrom) :ProtoHeader(cmd, sizeof(S2CLogout) - sizeof(ProtoHeader)), m_siFrom(siFrom)
	{
		strcpy(m_szName, szName);
	};

	char m_szName[NAMELEN];
	sockaddr_in m_siFrom;
};

/**
 * @brief 客户端公聊包
*/
struct C2SPubChat :public ProtoHeader
{
	C2SPubChat() {};
	C2SPubChat(Cmd cmd, char* szName,char *szMsg) :ProtoHeader(cmd, sizeof(C2SPubChat) - sizeof(ProtoHeader))
	{
		strcpy(m_szName, szName);
		strcpy(m_szMsg, szMsg);
	};

	char m_szName[NAMELEN];
	char m_szMsg[MSGLEN];
};

/**
 * @brief 服务端转发公聊包
*/
struct S2CPubChat :public ProtoHeader
{
	S2CPubChat() {};
	S2CPubChat(Cmd cmd, char* szName, char* szMsg, sockaddr_in siFrom) :ProtoHeader(cmd, sizeof(S2CPubChat) - sizeof(ProtoHeader)), m_siFrom(siFrom)
	{
		strcpy(m_szName, szName);
		strcpy(m_szMsg, szMsg);
	};
	char m_szName[NAMELEN];
	char m_szMsg[MSGLEN];
	sockaddr_in m_siFrom;
};

/**
 * @brief 客户端私聊包
*/
struct C2SPriChat :public ProtoHeader
{
	C2SPriChat() {};
	C2SPriChat(Cmd cmd, char* szName, char* szMsg,sockaddr_in siTo) :ProtoHeader(cmd, sizeof(C2SPriChat) - sizeof(ProtoHeader)),m_siTo(siTo)
	{
		strcpy(m_szName, szName);
		strcpy(m_szMsg, szMsg);
	};
	char m_szName[NAMELEN];
	char m_szMsg[MSGLEN];
	sockaddr_in m_siTo;
};

/**
 * @brief 客户端私聊包
*/
struct S2CPriChat :public ProtoHeader
{
	S2CPriChat() {};
	S2CPriChat(Cmd cmd, char* szNameFrom, sockaddr_in siFrom, char* szNameTo, sockaddr_in siTo, char *szMsg) 
		:ProtoHeader(cmd, sizeof(S2CPriChat) - sizeof(ProtoHeader)), m_siTo(siTo),m_siFrom(siFrom)
	{
		strcpy(m_szNameFrom, szNameFrom);
		strcpy(m_szNameTo, szNameTo);
		strcpy(m_szMsg, szMsg);
	};
	char m_szNameFrom[NAMELEN];
	char m_szNameTo[NAMELEN];
	sockaddr_in m_siFrom;
	sockaddr_in m_siTo;
	char m_szMsg[MSGLEN];
};

#pragma pack(pop)

class CTcp
{
public:
	CTcp(SOCKET sock):m_sock(sock){};
	
	//从tcp流中读取 流量包
	shared_ptr<ProtoHeader> RecvPackage()
	{
		//先读包头
		ProtoHeader pkgHeader = {};
		int nRet = ReadBytes((char*)&pkgHeader, sizeof(pkgHeader));
		if (nRet == 0)
		{
			return nullptr;
		}

		//根据包头，在堆上创建对应大小的包
		shared_ptr<ProtoHeader> ptrPkg((ProtoHeader *) new char[sizeof(pkgHeader) + pkgHeader.m_nLen]);

		//拷贝包头
		memcpy(ptrPkg.get(),&pkgHeader,sizeof(pkgHeader));

		//继续从tcp流中读取，包体
		nRet = ReadBytes((char *)ptrPkg.get() + sizeof(pkgHeader), pkgHeader.m_nLen);
		if (nRet == 0)
		{
			return nullptr;
		}

		return ptrPkg;
	}

	//向tcp流中发送 流量包
	int SendPackage(shared_ptr<ProtoHeader> pkgHeader)
	{
		int nRet = send(m_sock, (char *)pkgHeader.get(),sizeof(pkgHeader) + pkgHeader->m_nLen, 0);
		return nRet;
	}

private:

	/**
	 * @brief 从tcp流中，读取nLen字节数据 到 pBuf所指位置
	 * @param pBuf 缓冲区
	 * @param nLen 指定大小
	 * @return 读取内容大小
	*/
	int ReadBytes(char *pBuf, int nLen)
	{
		int nBytesRecv = 0;
		while (nBytesRecv < nLen)
		{
			int nRet = recv(m_sock, pBuf + nBytesRecv, nLen - nBytesRecv, 0);
			if (nRet == 0 || (nRet == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET))
			{
				return 0;
			}
			else if (nRet == SOCKET_ERROR)
			{
				continue;
			}
			nBytesRecv += nRet;
		}
		return nBytesRecv;
	}
private:
	SOCKET m_sock;
};


bool operator==(sockaddr_in siL, sockaddr_in siR)
{
	return (siL.sin_addr.S_un.S_addr == siR.sin_addr.S_un.S_addr) && (siL.sin_port == siR.sin_port);
}