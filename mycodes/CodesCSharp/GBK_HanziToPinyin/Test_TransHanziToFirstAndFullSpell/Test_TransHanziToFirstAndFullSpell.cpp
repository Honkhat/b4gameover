// Test_TransHanziToFirstAndFullSpell.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "sqlite3.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <atlstr.h>//windows平台上最好用的字符串类CString,没有之一.
#include <assert.h>
//先包含跨平台的头文件,后包含特定平台的头文件.
#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

typedef std::vector<std::string> TStrArray;
typedef std::vector<TStrArray> TArrayOfStrArray;
typedef std::unordered_map<std::string, bool> TStrExistMap;

const std::string kGbkHanziPublishTable = "GBK_Hanzi_Publish";
std::string g_sPinyins;
std::string g_sFullSpells, g_sFirstSpells;
TArrayOfStrArray g_arPinyins;//每一个元素都是一个Pinyins; "arPinyin"则表示每一个元素是一个Pinyin;
std::string g_sTmp;

void print_row(int n_values, char** values)
{
	for (int i = 0; i < n_values; ++i)
		printf("%10s", values[i]);
	printf("\n");
}

int print_result(void* data, int n_columns, char** column_values, char** column_names)
{
	assert(n_columns == 1);//当前只有一种获取一个汉字对应的多个拼音, 一种需求;
	g_sPinyins = column_values[0];
	return 0;
}

int CalcHanziGBKValueFromDByte(unsigned int b1, unsigned b2)
{
	//严格用位运算, 不要用加减运算(因为会做符号计算)!
	//Byte是无符号的,所以扩展位数都是补0;
	
	return ((b1 << 8) | b2);
}

bool SplitString(char* sSrc, char* szDelim, char* szIgnore, TStrArray& arResult)
{
	arResult.clear();
	if (!sSrc || strlen(sSrc) < 1)
		return false;

	char* pRemain = nullptr;
	char* pDivided = strtok_s(sSrc, szDelim, &pRemain);
	while (pDivided)
	{
		if (!szIgnore || (strcmp(pDivided, szIgnore) != 0))
			arResult.push_back(pDivided);
		pDivided = strtok_s(NULL, szDelim, &pRemain);
	}

	return arResult.size() > 0;
}

//[IN]:以逗号分隔的字符串;
//[OUT]:去重后,仍以逗号分隔;
void FilterRepeatedString(std::string& sSrc)
{
	TStrArray arSplit;
	if (SplitString((char*)sSrc.c_str(), ",", nullptr, arSplit))
	{
		TStrExistMap mp1;
		for (int i = 0; i < arSplit.size(); ++i)
		{
			TStrExistMap::iterator it1 = mp1.find(arSplit[i]);
			if (it1 == mp1.end())
				mp1.insert(TStrExistMap::value_type(arSplit[i], true));
		}
		//重新拼接字符串;
		sSrc.clear();
		for (TStrExistMap::iterator it1 = mp1.begin(); it1 != mp1.end(); ++it1)
		{
			if (sSrc.length() > 0)
				sSrc += ',';
			sSrc += it1->first;
		}
	}
}

//生成一个包含汉字的字符串的所有拼音组合(多音字);
void _GenPinyinCombinationRecursive(int iZiIdx, std::string& sFullSpell, std::string& sFirstSpell)
{
	if (iZiIdx > g_arPinyins.size()-1)//出口
	{
		//放进组合方案里面;
		if (g_sFullSpells.length() > 0)
			g_sFullSpells += ',';
		g_sFullSpells += sFullSpell;
		if (g_sFirstSpells.length() > 0)
			g_sFirstSpells += ',';
		g_sFirstSpells += sFirstSpell;
	}
	else
	{
		//当前字的每种发音对应一种组合;
		std::string sFullCopy, sFirstCopy;
		for (int i = 0; i < g_arPinyins[iZiIdx].size(); ++i)
		{
			g_sTmp = g_arPinyins[iZiIdx][i];
			sFullCopy = sFullSpell + g_sTmp;
			char ch1 = g_sTmp[0];
			if ( (ch1 > 0 && ch1 < 128) || g_sTmp.size() < 2)
				sFirstCopy = sFirstSpell + g_sTmp[0];
			else
				sFirstCopy = sFirstSpell + g_sTmp[0] + g_sTmp[1];
			_GenPinyinCombinationRecursive(iZiIdx+1, sFullCopy, sFirstCopy);
		}			
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	//相对路径转绝对路径的方法:
	//1. Windows平台特定的方法: GetModuleFileName()+PathAppend();
	//2. C Runtime的方法: getcwd();//但是调试的时候是工程目录不是exe目录;
	//3. 取巧法: argv[0];
	//4. C++语言方法: BOOST;


	//测试函数FilterRepeatedString();
	//sstd::string sTest88 = "abcdef！,abcdef！,123,123,何健3,何健3,王33，，前,王33";
	//FilterRepeatedString(sTest88);

	char szPath[MAX_PATH] = { 0 };
	::GetModuleFileNameA(NULL, szPath, MAX_PATH);
	::PathAppendA(szPath, "..\\..\\data\\hanzi.db");
	
	sqlite3 *db = NULL;
	char *errMsg = NULL;
	int rc = sqlite3_open(szPath, &db);
	if (rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		getchar();
		return 0;
	}
	else
		printf("open test.db successfully!\n");

	//测试字符串	
	TStrArray arTestHanzis;
	
	arTestHanzis.push_back("！王123重阳很行a");
	arTestHanzis.push_back("DAMN! 银行是我的仇人！");
	arTestHanzis.push_back("重量级嘉宾：朱镕基上场--");
	arTestHanzis.push_back("You are the best!");
	std::string sZis, sTmp, sFullSpell, sFirstSpell;
	CStringA sSql;
	
	//实现思路: 取一个字符串, 遍历每一个汉字的拼音, 放入g_arPinyins, 递归遍历(未知广度)生成最终的全拼字符串sFullSpells+首拼字符串sFirstSpells;
	for (int i = 0; i < arTestHanzis.size(); ++i)
	{
		sZis = arTestHanzis[i];
		std::cout << "原字符串: [" << sZis << "]" << std::endl;
		
		g_arPinyins.clear();
		int iZisSize = sZis.size();
		for (int j = 0; j < iZisSize;)//当前工程使用多字节编码,所以一个汉字会被分成2个字节来存储;
		{
			//GBK字符集中汉字的编码范围是: [0x81][0x40] --> [0xFE][0xA0];
			//char ch1 = sZis[j];
			//char ch2 = sZis[j + 1];//只取最低8位的值,其余是符号位,置0.
			unsigned int b1 = sZis[j];
			unsigned int b2 = 0;
			if (j < iZisSize - 1)
				b2 = sZis[j + 1];
			b1 &= 0x00ff;
			b2 &= 0x00ff;
			unsigned int uGbkVal = CalcHanziGBKValueFromDByte(b1, b2);//DB Table中保存的是GBK值的Type是无符号整型;
			if ((j + 1 < iZisSize) && (uGbkVal >= 0x8140) && (uGbkVal <= 0xFEA0))
			{
				g_sPinyins = "";//sqlite3_exec()执行成功才会调用回调print_result;否则不会!
				sSql.Format("SELECT PINYINS FROM %s WHERE GBKVAL=%d", kGbkHanziPublishTable.c_str(), uGbkVal);
				rc = sqlite3_exec(db, sSql.GetString(), print_result, NULL, &errMsg);

				TStrArray arPinyin;
				if ((g_sPinyins.length() > 0) && SplitString((char*)g_sPinyins.c_str(), ",", nullptr, arPinyin))
				{
					g_arPinyins.push_back(arPinyin);					
				}
				else//没有找到对应汉字的拼音,原样放回;
				{
					arPinyin.clear();
					sTmp = sZis[j];
					sTmp += sZis[j + 1];
					arPinyin.push_back(sTmp);
					g_arPinyins.push_back(arPinyin);
				}
				j += 2;
			}
			else//非汉字处理
			{
				TStrArray arPinyin;
				sTmp = sZis[j];
				arPinyin.push_back(sTmp);
				g_arPinyins.push_back(arPinyin);
				++j;
			}			
		}

		//递归生成这个包含汉字的字符串的所有可能拼音的组合;
		g_sFullSpells = g_sFirstSpells = sFullSpell = sFirstSpell = "";
		_GenPinyinCombinationRecursive(0, sFullSpell, sFirstSpell);


		std::cout << "FullSpell: " << g_sFullSpells << std::endl;
		FilterRepeatedString(g_sFirstSpells);//首拼可能存在重复的情况!
		std::cout << "FirstSpell: " << g_sFirstSpells << std::endl;
		std::cout << std::endl;
	}

	printf("error code: %d\n", rc);
	printf("error message: %s\n", errMsg);

	sqlite3_close(db);

	getchar();
	return 0;
}