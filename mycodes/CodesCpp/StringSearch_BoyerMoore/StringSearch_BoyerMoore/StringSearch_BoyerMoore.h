// StringSearch_BoyerMoore.cpp : �������̨Ӧ�ó������ڵ㡣
//
/*
	@Date: 2016/03/21 18:31;
	@Where: Tongzhou District of Beijing;
	@Author: Alex Hong;
	@Mail: worksdata@163.com;
	@Reference: 
		http://www.ruanyifeng.com/blog/2013/05/boyer-moore_string_search_algorithm.html BM�㷨-��һ���������־;
*/

#include <sstream>
#include <string>
#include <vector>
#include <map>

typedef std::vector<int> TArrayInt;
TArrayInt g_arSearchResult;//store the index of searched result;
typedef std::map<std::wstring, int> TMapWszToInt; //E.g. "��,4":7;
typedef std::map<int, int> TMapIntToInt;


void StringSearch_BM(wchar_t* wszStrMain, int iLenMain, wchar_t* wszSearch, int iLenSearch, bool bOnlyFirst)
{
	//reset variables;
	g_arSearchResult.clear();

	//handle exceptions..
	if( NULL == wszStrMain || NULL == wszSearch)
		return;
	if(iLenSearch > iLenMain)
		return;
	/*
		δ֪���ַ��кܶ�, ������֪�������ַ������޵�. ����ֻ��ʼ����֪���ַ�����Ծֵ(Skip), �����в����ڵľ���-1;
		Shift����ĳ�ʼ��ͬ�����������뷨.
		��ʼ�������������ֵ��, ���ڳ�����/��ʱ��������������;
	*/

	int i = 0, j = 0;

	//Initialize the Skip array data for Bad-Character-Rules..
	TMapWszToInt mapSkip;
	mapSkip.clear();
	TMapWszToInt::iterator it1;
	std::wostringstream oStream;
	for(i = iLenSearch - 1; i > 0; --i)//0-->1;
	{
		for(j = i - 1; j >= 0; --j)
		{
			if(wszSearch[j] != wszSearch[i]) //Skip is for bad character which is different with wszSearch;
			{
				oStream.clear();//clear bad bits;
				oStream.str(L"");//clear data;
				oStream<<wszSearch[j]<<','<<i;//format to key string;
				//wcout<<oStream.str().c_str()<<endl;//test..
				it1 = mapSkip.find(oStream.str());
				if(mapSkip.end() == it1) //only set the pos of nearest same bad character;
					mapSkip.insert(TMapWszToInt::value_type(oStream.str(), i-j));
			}
		}
	}

	//�ú�׺��ǰ����: ������һ���ַ�����ͬ��!!!!!!
	//Initialize the Shift array data for Good-Suffix-Rules..
	//Shift����ļ�ֵ�Ǻú�׺�������ַ����±�;
	//1. �����ж�ģʽ�����Ƿ�����ȫһ�µĺú�׺;�о�ֱ�Ӽ���λ��, EXIT;
	//  ��ʵ�Ȿ�����һ���ַ��������Ĺ���,Ϊ�˼򻯼���, ����ֻʹ��Bad-Character-Rule;
	//2. ���û��, ���ڴ���Ѱ�Һú�׺���Ӵ�;
	TMapIntToInt mapShift;	
	mapShift.clear();
	mapShift.insert(TMapIntToInt::value_type(0, iLenSearch));//0==i�Ļ���ƥ����, Shift[]=iLenSearch;
	//1. try to find the same suffix..
	for(int iSuffixPosL = iLenSearch - 1; iSuffixPosL > 0; --iSuffixPosL)//iSuffixPosL: left position of good suffix;
	{
		int iSuffixLen = iLenSearch - iSuffixPosL;
		wchar_t* wszSuffix = wszSearch + iSuffixPosL;
		
		for(int iPosMainL = iSuffixPosL - 1; iPosMainL >= 0;)//���������ƶ�suffix���Ƚ�;
		{
			//compare..
			for(i = 0; i < iSuffixLen; ++i)
			{
				if(wszSearch[iPosMainL+i] != wszSuffix[i])//bad character
					break;
			}
			//check result..
			if(iSuffixLen == i) //find it!
			{				
				mapShift[iLenSearch-i] = iSuffixPosL - iPosMainL; 
				break; //����Ϊ��ǰsuffix��Shiftֵ;
			}
			else //calc the skip distance;
			{
				for(j = i + 1; j < iSuffixLen; ++j)//��suffix��Ѱ������Ļ��ַ�
				{
					if(wszSuffix[j] == wszSearch[iPosMainL+i])
						break;
				}
				iPosMainL = iPosMainL - (j - i);//iSuffixLen==j�����Ҳ����;	
			}
		}
	}
	//2. try to find the child suffix at head..
	TMapIntToInt::iterator it2;
	for(int iSuffixPosL = iLenSearch - 1; iSuffixPosL > 0; --iSuffixPosL)//iSuffixPosL: left position of good suffix;
	{
		it2 = mapShift.find(iSuffixPosL);
		if(mapShift.end() == it2)
		{
			wchar_t* wszSuffix = NULL;
			int iSuffixLen  = 0;
			for(i = 0; i < iLenSearch - iSuffixPosL; ++i)//form sub suffix big to small.
			{
				wszSuffix = wszSearch + iSuffixPosL + i;
				iSuffixLen = iLenSearch - iSuffixPosL - i;
				//compare if match..				
				for(j = 0; j < iSuffixLen; ++j)
				{
					if(wszSearch[j] != wszSuffix[j])
						break;
				}
				if(iSuffixLen == j) //find it!
				{
					mapShift[iSuffixPosL] = iLenSearch - iSuffixLen; //�Ժú�׺�����һ���ַ�Ϊ׼;
					break;
				}
			}

			//if no sub suffix at all..
			it2 = mapShift.find(iSuffixPosL);
			if(mapShift.end() == it2)
				mapShift[iSuffixPosL] = iLenSearch;
		}
	}

	//================
	//start to search!
	//================
	int iPosMainL = 0;//left iterator of main string;
	//int iPosSearch = 0;//iterator of search string; no use!
	while( iPosMainL <= iLenMain - iLenSearch + 1)
	{
		//compare with aligned sub-string..
		int i = iLenSearch - 1;
		for(; i >= 0; --i)//right to left comparison;
		{
			if(wszSearch[i] != wszStrMain[iPosMainL+i])//bad character!
				break;
		}
		//----calc how many steps to move rightwards..----
		int iSkip = 0;
		if(i < 0)
		{
			g_arSearchResult.push_back(iPosMainL);
			if(bOnlyFirst)
			{
				mapSkip.clear();
				mapShift.clear();
				return;
			}
			iSkip = iLenSearch;
		}
		else
		{
			//1. calc max skip steps for bad characters..
			oStream.clear();//clear bits;
			oStream.str(L"");//clear data;
			oStream<<wszStrMain[iPosMainL+i]<<','<<i;
			it1 = mapSkip.find((wchar_t*)oStream.str().c_str());
			if(mapSkip.end() == it1)
				iSkip = i + 1;
			else
				iSkip = it1->second;
		}
		
		//--2. calc max shift steps for good suffix..--
		int iShift = 0;
		if(i < iLenSearch - 1)//������һ����ͬ���ַ��ſ���ʹ��"�ú�׺"����!
		{
			it2 = mapShift.find(i-1);
			if(mapShift.end() != it2)
				iShift = it2->second;
		}
		iPosMainL += ((iSkip>iShift) ? iSkip : iShift);
	}

	//clear..
	mapSkip.clear();
	mapShift.clear();
}
