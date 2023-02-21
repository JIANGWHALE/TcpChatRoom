#pragma once
#include <WinSock2.h>
#include <memory>
#define NAMELEN 32
#define MSGLEN 255

using namespace std;

/**
 * @brief ��Ϣ������
*/

enum Cmd
{
	C2S_LOGIN,					//�ͻ������߰�
	S2C_LOGIN,					//���͸��ͻ���

	C2S_LOGOUT,					//����
	S2C_LOGOUT,					//����

	C2S_PUBCHAT,				//����
	S2C_PUBCHAT,				//˽��

	C2S_PRICHAT,				//˽��
	S2C_PRICHAT,				//˽��
};

#pragma pack(push)
#pragma pack(1)
/**
 * @brief Э���ͷ
*/
struct ProtoHeader
{
	ProtoHeader() {};
	ProtoHeader(Cmd cmd,size_t nLen):m_cmd(cmd),m_nLen(nLen){};

	Cmd			m_cmd;					//��Ϣ����
	size_t		m_nLen;					//���峤��
};

/**
 * @brief �ͻ������߰�
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
 * @brief ������ת�����߰�
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
 * @brief �ͻ������߰�
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
 * @brief ������ת�����߰�
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
 * @brief �ͻ��˹��İ�
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
 * @brief �����ת�����İ�
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
 * @brief �ͻ���˽�İ�
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
 * @brief �ͻ���˽�İ�
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
	
	//��tcp���ж�ȡ ������
	shared_ptr<ProtoHeader> RecvPackage()
	{
		//�ȶ���ͷ
		ProtoHeader pkgHeader = {};
		int nRet = ReadBytes((char*)&pkgHeader, sizeof(pkgHeader));
		if (nRet == 0)
		{
			return nullptr;
		}

		//���ݰ�ͷ���ڶ��ϴ�����Ӧ��С�İ�
		shared_ptr<ProtoHeader> ptrPkg((ProtoHeader *) new char[sizeof(pkgHeader) + pkgHeader.m_nLen]);

		//������ͷ
		memcpy(ptrPkg.get(),&pkgHeader,sizeof(pkgHeader));

		//������tcp���ж�ȡ������
		nRet = ReadBytes((char *)ptrPkg.get() + sizeof(pkgHeader), pkgHeader.m_nLen);
		if (nRet == 0)
		{
			return nullptr;
		}

		return ptrPkg;
	}

	//��tcp���з��� ������
	int SendPackage(shared_ptr<ProtoHeader> pkgHeader)
	{
		int nRet = send(m_sock, (char *)pkgHeader.get(),sizeof(pkgHeader) + pkgHeader->m_nLen, 0);
		return nRet;
	}

private:

	/**
	 * @brief ��tcp���У���ȡnLen�ֽ����� �� pBuf��ָλ��
	 * @param pBuf ������
	 * @param nLen ָ����С
	 * @return ��ȡ���ݴ�С
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