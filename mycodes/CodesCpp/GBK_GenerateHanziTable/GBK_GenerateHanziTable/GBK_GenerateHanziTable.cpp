// GBK_GenerateHanziTable.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <assert.h>

using namespace std;


typedef std::vector<std::string> TStrArray;


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

int _tmain(int argc, _TCHAR* argv[])
{
	ifstream fIn;
	ofstream fOut;
	//从GBK_HanZi_Raw.DAT提取出来汉字, 写到GBK_HanZi.DAT中去;
	fIn.open("../data/GBK_HanZi_RAW.DAT");
	fOut.open("../data/GBK_HanZi.DAT");
	if (!fIn.fail() && !fOut.fail())
	{
		std::string sLine, sTmp;
		unsigned int u1 = 0;
		unsigned int u2 = 0;
		unsigned int uCount = 0;
		TStrArray arSplit;

		while (std::getline(fIn, sLine))
		{
			//以空格作为分隔符,分割字符串;
			if (SplitString((char*)sLine.c_str(), " ", "", arSplit) && (arSplit.size() > 2))//第1个字符串应该是序号;
			{
				//首先判断此行是序号行还是汉字行..
				sTmp = arSplit[1];
				if (sTmp.compare("０") >= 0 && sTmp.compare("Ｆ") <= 0) //如果此行汉字数字里的０-Ｆ(不同于英文里的数字);
					continue;

				for (int i = 1; i < arSplit.size(); ++i)
				{
					assert(arSplit[i].size() == 2);
					cout << arSplit[i] << " ";
					//u1 = arSplit[i][0];
					//u2 = arSplit[i][1];
					//cout << hex << u1 << u2 << " ";
					fOut << arSplit[i] << " ";
					++uCount;
				}
				cout << " <-- 此行" << dec << arSplit.size()-1 << "个汉字" << endl;
				fOut << endl;
			}
		}
		cout << "共 " << uCount << " 个汉字(包含有编码无字形的汉字)" << endl;
	}
	else
	{
		cout << "open file failed." << endl;
	}
		
	cout << "OVER." << endl;

	getchar();
	return 0;
}

