// GbkTableToUtf8Table.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Shlwapi.h>
#include "sqlite3.h"
#include <iostream>
#include <string>
#include <vector>
#include <Stringapiset.h>
#include <atlstr.h>

#pragma comment(lib, "Shlwapi.lib")

/*
	@Pupose: ��GBK_Hanzi_Pub��������GBK VALUE, ���ڸĳ�UTF8���͵�һ�������ַ�.
	@Date: 2017/12/07;
	@Author: jian.he;
	@Email: hejian@anymacro.com;
*/

const std::string g_kHanziGbkTable = "HANZI_GBK";
const std::string g_kHanziUtf8Table = "HANZI_UTF8";
const int g_kBatchCount = 5000;

std::string GB2312ToUTF8(const std::string& str)
{
	std::string retStr;
	if (str.length() > 0)
	{
		int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
		wchar_t * pwBuf = new wchar_t[nwLen + 1];//һ��Ҫ��1����Ȼ�����β��
		wmemset(pwBuf, 0, nwLen + 1);
		//GB2312-->UNICODE;
		::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pwBuf, nwLen + 1);

		int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
		char * pBuf = new char[nLen + 1];
		memset(pBuf, 0, nLen+1);
		//UNICODE-->UTF8;
		::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, pBuf, nLen + 1, NULL, NULL);
		
		retStr = pBuf;

		delete[]pwBuf;
		delete[]pBuf;
	}

	return retStr;
}

std::string UTF8ToGB2312(const std::string& str)
{
	std::string retStr;
	if (str.length() > 0)
	{
		int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
		wchar_t * pwBuf = new wchar_t[nwLen + 1];//һ��Ҫ��1����Ȼ�����β��
		wmemset(pwBuf, 0, nwLen+1);
		//UTF8-->UNICODE;
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, pwBuf, nwLen);

		int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
		char * pBuf = new char[nLen + 1];
		memset(pBuf, 0, nLen+1);
		//UNICODE-->GB2312;
		WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, pBuf, nLen+1, NULL, NULL);

		retStr = pBuf;

		delete[]pBuf;
		delete[]pwBuf;
	}

	return retStr;
}

void Gbk_IntToStr(unsigned int uGbkVal, std::string& sGbk)
{
	//GBK����ռ����byte�Ŀռ�;
	unsigned int u = uGbkVal;
	char ch1 = (u >> 8) & 0x00ff;
	char ch2 = uGbkVal & 0x00ff;//����ֱ��ch2 = uGbkVal�ض�Ҳ�ܴﵽЧ��;
	sGbk.clear();
	sGbk += ch1;
	sGbk += ch2;
}

void GbkIntToUtf8Str(unsigned int uGbkVal, std::string& sUtf8)
{
	Gbk_IntToStr(uGbkVal, sUtf8);//������sUtf8����;
	sUtf8 = GB2312ToUTF8(sUtf8);
}

struct THanziInfo
{
	int iGbkVal;
	std::string sZiUtf8;
	std::string sPinyins;
};

typedef std::vector<THanziInfo> THanziInfoArray;

int _tmain(int argc, _TCHAR* argv[])
{
	//ʵ��˼·:
	//��hanzi.dbǨ��GBK_Hanzi_Pub��hanzi_utf8.db;
	//ÿ��ȡ5000����¼,����ת����UTF8��,�������浽��UTF8_Hanzi��.
	//(DB�п����м���������,һ����ȫȡ����,���ܻ��ڴ�ľ�)

	std::string sTmp;
	CStringA sSql;


	char szPath[MAX_PATH] = { 0 };
	::GetModuleFileNameA(NULL, szPath, MAX_PATH);
	::PathAppendA(szPath, "..\\..\\data\\hanzi_utf8.db");

	char szPathSrc[MAX_PATH] = { 0 };
	strcpy_s(szPathSrc, szPath);
	::PathAppendA(szPathSrc, "..\\hanzi.db");

	//���hanzi_utf8.db�Ѿ�����, delete it!
	::DeleteFileA(szPath);

	//����hanzi_utf8.db�ļ�;
	sqlite3 *db = NULL;
	char *errMsg = NULL;
	int rc = sqlite3_open(szPath, &db);
	if (rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		std::cout << sqlite3_errmsg(db) << std::endl;
		getchar();
		return 0;
	}
	else
		printf("open gbk hanzi_utf8.db successfully!\n");

	//��hanzi_gbk.dbǨ�����ݵ�hanzi_utf8.db;
	sSql.Format("ATTACH DATABASE '%s' as 'hzgbk';CREATE TABLE %s AS SELECT * FROM hzgbk.GBK_Hanzi_Publish;DETACH DATABASE 'hzgbk';\
								CREATE TABLE %s (ZI VARCHAR PRIMARY KEY,PINYINS VARCHAR);", szPathSrc, g_kHanziGbkTable.c_str(), g_kHanziUtf8Table.c_str());
	rc = sqlite3_exec(db, sSql.GetString(), nullptr, nullptr, &errMsg);
	if (SQLITE_OK != rc)
	{
		std::cout << sqlite3_errmsg(db) << std::endl;
		sqlite3_close(db);
		//�����,���ﲻ������������;
		return -1;
	}

	//ÿ5000����¼��Ϊһ��batchת��ΪUTF8��,�����±�;
	//sqlite3_prepare_v2();sqlite3_step()�ȵ��÷�;
	sqlite3_stmt* pStmt = nullptr;
	int iLastMaxID = 0;
	THanziInfoArray arHanziInfo;
	
	while (1)
	{
		sSql.Format("SELECT GBKVAL,PINYINS FROM %s WHERE GBKVAL>%d ORDER BY GBKVAL ASC LIMIT %d", g_kHanziGbkTable.c_str(), iLastMaxID, g_kBatchCount);
		if (SQLITE_OK != sqlite3_prepare_v2(db, sSql.GetString(), sSql.GetLength(), &pStmt, nullptr))
		{
			if (pStmt)
				sqlite3_finalize(pStmt);
			std::cout << sqlite3_errmsg(db) << std::endl;
			sqlite3_close(db);
			return -1;
		}

		//һ��һ�л�ȡ����, ת����UTF8, �����ڴ���;
		arHanziInfo.clear();
		while (SQLITE_ROW == sqlite3_step(pStmt))
		{
			THanziInfo infoZi;
			infoZi.iGbkVal = sqlite3_column_int(pStmt, 0);
			infoZi.sPinyins = (char*)sqlite3_column_text(pStmt, 1);//UTF8 FORMAT;
			GbkIntToUtf8Str((unsigned int)infoZi.iGbkVal, infoZi.sZiUtf8);

			arHanziInfo.push_back(infoZi);
		}
		sqlite3_finalize(pStmt);//һ��sql�ֽ���ִ�����, �ͷ�pStmt;

		//����Ƿ�ȫ���������;
		if (arHanziInfo.size() < 1)//����������;
			break;

		//=========================================
		//�ڴ��е������������浽HANZI_UTF8����;
		//=========================================

		//ִ��"BEGIN TRANSACTION;";
		pStmt = nullptr;
		sSql = "BEGIN TRANSACTION;";//sqlite3_exec = sqlite3_prepare + [sqlite3_bind] + sqlite3_step + sqlite3_finalize;
		if (SQLITE_OK != sqlite3_prepare_v2(db, sSql.GetString(), sSql.GetLength(), &pStmt, nullptr))
		{
			if (pStmt)
				sqlite3_finalize(pStmt);
			std::cout << sqlite3_errmsg(db) << std::endl;
			sqlite3_close(db);
			return -1;
		}
		sqlite3_step(pStmt);//should return SQLITE_DONE;
		sqlite3_finalize(pStmt);
		
		//ִ����������..
		sSql.Format("INSERT INTO %s (ZI,PINYINS) VALUES (?,?);", g_kHanziUtf8Table.c_str());
		pStmt = nullptr;
		if (SQLITE_OK != sqlite3_prepare_v2(db, sSql.GetString(), sSql.GetLength(), &pStmt, nullptr))
		{
			if (pStmt)
				sqlite3_finalize(pStmt);
			std::cout << sqlite3_errmsg(db) << std::endl;
			sqlite3_close(db);
			return -1;
		}
		for (int j = 0; j < arHanziInfo.size(); ++j)
		{
			//ע���ڰ�ʱ,������ı�������ֵ��1;
			sTmp = arHanziInfo[j].sZiUtf8;
			sqlite3_bind_text(pStmt, 1, sTmp.c_str(), sTmp.size(), SQLITE_TRANSIENT);
			sTmp = arHanziInfo[j].sPinyins;
			sqlite3_bind_text(pStmt, 2, sTmp.c_str(), sTmp.size(), SQLITE_TRANSIENT);

			if (SQLITE_DONE != sqlite3_step(pStmt))
			{
				if (pStmt)
					sqlite3_finalize(pStmt);
				std::cout << sqlite3_errmsg(db) << std::endl;
				sqlite3_close(db);
				return -1;
			}
			sqlite3_reset(pStmt);
		}
		if (pStmt)
			sqlite3_finalize(pStmt);

		//ִ��"COMMIT;";
		sSql = "COMMIT;";
		pStmt = nullptr;
		if (SQLITE_OK != sqlite3_prepare_v2(db, sSql.GetString(), sSql.GetLength(), &pStmt, nullptr))
		{
			if (pStmt)
				sqlite3_finalize(pStmt);
			std::cout << sqlite3_errmsg(db) << std::endl;
			sqlite3_close(db);
			return -1;
		}
		sqlite3_step(pStmt);
		if (pStmt)
			sqlite3_finalize(pStmt);

		//����iLastMaxID;
		iLastMaxID = arHanziInfo[arHanziInfo.size() - 1].iGbkVal;

		//��ջ�ȡ������;
		arHanziInfo.clear();
	}


	//��֤�Ƿ�21003����¼;
	sSql.Format("SELECT COUNT(*) FROM %s;", g_kHanziUtf8Table.c_str());
	rc = sqlite3_prepare_v2(db, sSql.GetString(), sSql.GetLength(), &pStmt, nullptr);
	if (SQLITE_ROW == sqlite3_step(pStmt))
	{
		int iTotalRecordCount = sqlite3_column_int(pStmt, 0);
		std::cout << g_kHanziUtf8Table << " has " << iTotalRecordCount << " records." << std::endl;
	}
	if (pStmt)
		sqlite3_finalize(pStmt);
	
	//ɾ��HANZI_GBK��;
	sSql.Format("DROP TABLE %s;", g_kHanziGbkTable.c_str());
	pStmt = nullptr;
	if (SQLITE_OK != sqlite3_prepare_v2(db, sSql.GetString(), sSql.GetLength(), &pStmt, nullptr))
	{
		if (pStmt)
			sqlite3_finalize(pStmt);
		std::cout << sqlite3_errmsg(db) << std::endl;
		sqlite3_close(db);
		return -1;
	}
	sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);

	//ѹ�����ݿ�ռ�õĿռ�;
	sSql ="VACUUM;";
	pStmt = nullptr;
	rc = sqlite3_exec(db, sSql.GetString(), nullptr, nullptr, &errMsg);

	printf("error code: %d\n", rc);
	printf("error message: %s\n", errMsg);

	sqlite3_close(db);
	std::cout << "OVER." << std::endl;
	getchar();
	return 0;
}

