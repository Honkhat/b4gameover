// Test_TransHanziToFirstAndFullSpell.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "sqlite3.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <atlstr.h>//windowsƽ̨������õ��ַ�����CString,û��֮һ.
#include <assert.h>
//�Ȱ�����ƽ̨��ͷ�ļ�,������ض�ƽ̨��ͷ�ļ�.
#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

typedef std::vector<std::string> TStrArray;
typedef std::vector<TStrArray> TArrayOfStrArray;
typedef std::unordered_map<std::string, bool> TStrExistMap;

const std::string kGbkHanziPublishTable = "GBK_Hanzi_Publish";
std::string g_sPinyins;
std::string g_sFullSpells, g_sFirstSpells;
TArrayOfStrArray g_arPinyins;//ÿһ��Ԫ�ض���һ��Pinyins; "arPinyin"���ʾÿһ��Ԫ����һ��Pinyin;
std::string g_sTmp;

void print_row(int n_values, char** values)
{
	for (int i = 0; i < n_values; ++i)
		printf("%10s", values[i]);
	printf("\n");
}

int print_result(void* data, int n_columns, char** column_values, char** column_names)
{
	assert(n_columns == 1);//��ǰֻ��һ�ֻ�ȡһ�����ֶ�Ӧ�Ķ��ƴ��, һ������;
	g_sPinyins = column_values[0];
	return 0;
}

int CalcHanziGBKValueFromDByte(unsigned int b1, unsigned b2)
{
	//�ϸ���λ����, ��Ҫ�üӼ�����(��Ϊ�������ż���)!
	//Byte���޷��ŵ�,������չλ�����ǲ�0;
	
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

//[IN]:�Զ��ŷָ����ַ���;
//[OUT]:ȥ�غ�,���Զ��ŷָ�;
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
		//����ƴ���ַ���;
		sSrc.clear();
		for (TStrExistMap::iterator it1 = mp1.begin(); it1 != mp1.end(); ++it1)
		{
			if (sSrc.length() > 0)
				sSrc += ',';
			sSrc += it1->first;
		}
	}
}

//����һ���������ֵ��ַ���������ƴ�����(������);
void _GenPinyinCombinationRecursive(int iZiIdx, std::string& sFullSpell, std::string& sFirstSpell)
{
	if (iZiIdx > g_arPinyins.size()-1)//����
	{
		//�Ž���Ϸ�������;
		if (g_sFullSpells.length() > 0)
			g_sFullSpells += ',';
		g_sFullSpells += sFullSpell;
		if (g_sFirstSpells.length() > 0)
			g_sFirstSpells += ',';
		g_sFirstSpells += sFirstSpell;
	}
	else
	{
		//��ǰ�ֵ�ÿ�ַ�����Ӧһ�����;
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
	//���·��ת����·���ķ���:
	//1. Windowsƽ̨�ض��ķ���: GetModuleFileName()+PathAppend();
	//2. C Runtime�ķ���: getcwd();//���ǵ��Ե�ʱ���ǹ���Ŀ¼����exeĿ¼;
	//3. ȡ�ɷ�: argv[0];
	//4. C++���Է���: BOOST;


	//���Ժ���FilterRepeatedString();
	//sstd::string sTest88 = "abcdef��,abcdef��,123,123,�ν�3,�ν�3,��33����ǰ,��33";
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

	//�����ַ���	
	TStrArray arTestHanzis;
	
	arTestHanzis.push_back("����123��������a");
	arTestHanzis.push_back("DAMN! �������ҵĳ��ˣ�");
	arTestHanzis.push_back("�������α������F���ϳ�--");
	arTestHanzis.push_back("You are the best!");
	std::string sZis, sTmp, sFullSpell, sFirstSpell;
	CStringA sSql;
	
	//ʵ��˼·: ȡһ���ַ���, ����ÿһ�����ֵ�ƴ��, ����g_arPinyins, �ݹ����(δ֪���)�������յ�ȫƴ�ַ���sFullSpells+��ƴ�ַ���sFirstSpells;
	for (int i = 0; i < arTestHanzis.size(); ++i)
	{
		sZis = arTestHanzis[i];
		std::cout << "ԭ�ַ���: [" << sZis << "]" << std::endl;
		
		g_arPinyins.clear();
		int iZisSize = sZis.size();
		for (int j = 0; j < iZisSize;)//��ǰ����ʹ�ö��ֽڱ���,����һ�����ֻᱻ�ֳ�2���ֽ����洢;
		{
			//GBK�ַ����к��ֵı��뷶Χ��: [0x81][0x40] --> [0xFE][0xA0];
			//char ch1 = sZis[j];
			//char ch2 = sZis[j + 1];//ֻȡ���8λ��ֵ,�����Ƿ���λ,��0.
			unsigned int b1 = sZis[j];
			unsigned int b2 = 0;
			if (j < iZisSize - 1)
				b2 = sZis[j + 1];
			b1 &= 0x00ff;
			b2 &= 0x00ff;
			unsigned int uGbkVal = CalcHanziGBKValueFromDByte(b1, b2);//DB Table�б������GBKֵ��Type���޷�������;
			if ((j + 1 < iZisSize) && (uGbkVal >= 0x8140) && (uGbkVal <= 0xFEA0))
			{
				g_sPinyins = "";//sqlite3_exec()ִ�гɹ��Ż���ûص�print_result;���򲻻�!
				sSql.Format("SELECT PINYINS FROM %s WHERE GBKVAL=%d", kGbkHanziPublishTable.c_str(), uGbkVal);
				rc = sqlite3_exec(db, sSql.GetString(), print_result, NULL, &errMsg);

				TStrArray arPinyin;
				if ((g_sPinyins.length() > 0) && SplitString((char*)g_sPinyins.c_str(), ",", nullptr, arPinyin))
				{
					g_arPinyins.push_back(arPinyin);					
				}
				else//û���ҵ���Ӧ���ֵ�ƴ��,ԭ���Ż�;
				{
					arPinyin.clear();
					sTmp = sZis[j];
					sTmp += sZis[j + 1];
					arPinyin.push_back(sTmp);
					g_arPinyins.push_back(arPinyin);
				}
				j += 2;
			}
			else//�Ǻ��ִ���
			{
				TStrArray arPinyin;
				sTmp = sZis[j];
				arPinyin.push_back(sTmp);
				g_arPinyins.push_back(arPinyin);
				++j;
			}			
		}

		//�ݹ���������������ֵ��ַ��������п���ƴ�������;
		g_sFullSpells = g_sFirstSpells = sFullSpell = sFirstSpell = "";
		_GenPinyinCombinationRecursive(0, sFullSpell, sFirstSpell);


		std::cout << "FullSpell: " << g_sFullSpells << std::endl;
		FilterRepeatedString(g_sFirstSpells);//��ƴ���ܴ����ظ������!
		std::cout << "FirstSpell: " << g_sFirstSpells << std::endl;
		std::cout << std::endl;
	}

	printf("error code: %d\n", rc);
	printf("error message: %s\n", errMsg);

	sqlite3_close(db);

	getchar();
	return 0;
}