// GBK_GenerateHanziTable.cpp : �������̨Ӧ�ó������ڵ㡣
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
	//��GBK_HanZi_Raw.DAT��ȡ��������, д��GBK_HanZi.DAT��ȥ;
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
			//�Կո���Ϊ�ָ���,�ָ��ַ���;
			if (SplitString((char*)sLine.c_str(), " ", "", arSplit) && (arSplit.size() > 2))//��1���ַ���Ӧ�������;
			{
				//�����жϴ���������л��Ǻ�����..
				sTmp = arSplit[1];
				if (sTmp.compare("��") >= 0 && sTmp.compare("��") <= 0) //������к���������ģ�-��(��ͬ��Ӣ���������);
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
				cout << " <-- ����" << dec << arSplit.size()-1 << "������" << endl;
				fOut << endl;
			}
		}
		cout << "�� " << uCount << " ������(�����б��������εĺ���)" << endl;
	}
	else
	{
		cout << "open file failed." << endl;
	}
		
	cout << "OVER." << endl;

	getchar();
	return 0;
}

