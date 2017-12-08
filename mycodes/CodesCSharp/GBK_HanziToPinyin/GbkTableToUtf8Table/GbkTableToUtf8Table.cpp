// GbkTableToUtf8Table.cpp : 定义控制台应用程序的入口点。
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
	@Pupose: 表GBK_Hanzi_Pub的主键是GBK VALUE, 现在改成UTF8类型的一个汉字字符.
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
		wchar_t * pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴
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
		wchar_t * pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴
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
	//GBK汉字占两个byte的空间;
	unsigned int u = uGbkVal;
	char ch1 = (u >> 8) & 0x00ff;
	char ch2 = uGbkVal & 0x00ff;//可能直接ch2 = uGbkVal截断也能达到效果;
	sGbk.clear();
	sGbk += ch1;
	sGbk += ch2;
}

void GbkIntToUtf8Str(unsigned int uGbkVal, std::string& sUtf8)
{
	Gbk_IntToStr(uGbkVal, sUtf8);//借用下sUtf8变量;
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
	//实现思路:
	//从hanzi.db迁移GBK_Hanzi_Pub表到hanzi_utf8.db;
	//每次取5000条记录,批量转换成UTF8后,批量保存到表UTF8_Hanzi中.
	//(DB中可能有几万条数据,一下子全取出来,可能会内存耗尽)

	std::string sTmp;
	CStringA sSql;


	char szPath[MAX_PATH] = { 0 };
	::GetModuleFileNameA(NULL, szPath, MAX_PATH);
	::PathAppendA(szPath, "..\\..\\data\\hanzi_utf8.db");

	char szPathSrc[MAX_PATH] = { 0 };
	strcpy_s(szPathSrc, szPath);
	::PathAppendA(szPathSrc, "..\\hanzi.db");

	//如果hanzi_utf8.db已经存在, delete it!
	::DeleteFileA(szPath);

	//创建hanzi_utf8.db文件;
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

	//从hanzi_gbk.db迁移数据到hanzi_utf8.db;
	sSql.Format("ATTACH DATABASE '%s' as 'hzgbk';CREATE TABLE %s AS SELECT * FROM hzgbk.GBK_Hanzi_Publish;DETACH DATABASE 'hzgbk';\
								CREATE TABLE %s (ZI VARCHAR PRIMARY KEY,PINYINS VARCHAR);", szPathSrc, g_kHanziGbkTable.c_str(), g_kHanziUtf8Table.c_str());
	rc = sqlite3_exec(db, sSql.GetString(), nullptr, nullptr, &errMsg);
	if (SQLITE_OK != rc)
	{
		std::cout << sqlite3_errmsg(db) << std::endl;
		sqlite3_close(db);
		//简单起见,这里不做其它清理工作;
		return -1;
	}

	//每5000条记录作为一个batch转化为UTF8后,插入新表;
	//sqlite3_prepare_v2();sqlite3_step()等的用法;
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

		//一行一行获取数据, 转化成UTF8, 放入内存中;
		arHanziInfo.clear();
		while (SQLITE_ROW == sqlite3_step(pStmt))
		{
			THanziInfo infoZi;
			infoZi.iGbkVal = sqlite3_column_int(pStmt, 0);
			infoZi.sPinyins = (char*)sqlite3_column_text(pStmt, 1);//UTF8 FORMAT;
			GbkIntToUtf8Str((unsigned int)infoZi.iGbkVal, infoZi.sZiUtf8);

			arHanziInfo.push_back(infoZi);
		}
		sqlite3_finalize(pStmt);//一段sql字节码执行完毕, 释放pStmt;

		//检查是否全部检索完毕;
		if (arHanziInfo.size() < 1)//不再有数据;
			break;

		//=========================================
		//内存中的数据批量保存到HANZI_UTF8表中;
		//=========================================

		//执行"BEGIN TRANSACTION;";
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
		
		//执行批量插入..
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
			//注意在绑定时,最左面的变量索引值是1;
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

		//执行"COMMIT;";
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

		//更新iLastMaxID;
		iLastMaxID = arHanziInfo[arHanziInfo.size() - 1].iGbkVal;

		//清空获取的数据;
		arHanziInfo.clear();
	}


	//验证是否21003条记录;
	sSql.Format("SELECT COUNT(*) FROM %s;", g_kHanziUtf8Table.c_str());
	rc = sqlite3_prepare_v2(db, sSql.GetString(), sSql.GetLength(), &pStmt, nullptr);
	if (SQLITE_ROW == sqlite3_step(pStmt))
	{
		int iTotalRecordCount = sqlite3_column_int(pStmt, 0);
		std::cout << g_kHanziUtf8Table << " has " << iTotalRecordCount << " records." << std::endl;
	}
	if (pStmt)
		sqlite3_finalize(pStmt);
	
	//删除HANZI_GBK表;
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

	//压缩数据库占用的空间;
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

