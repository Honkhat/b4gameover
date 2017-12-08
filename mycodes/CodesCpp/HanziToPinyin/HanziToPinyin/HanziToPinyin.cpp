// HanziToPinyin.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <vector>

BOOL isIncludeChinese(const std::string& str)
{
	for (int n = 0; n < str.length()-1; ++n)
	{
		TCHAR ch[3] = { 0 };
		ch[0] = str[n];
		ch[1] = str[n+1];
		ch[2] = '\0';
		if ((ch[0] & 0x80) && (ch[1] & 0x80))
		{
			//++n;
			return TRUE;
		}
	}
	return FALSE;
}
std::string GetFirstLetter(const char* strChs)
{
	static int li_SecPosValue[] = {
		1601, 1637, 1833, 2078, 2274, 2302, 2433, 2594, 2787, 3106, 3212,
		3472, 3635, 3722, 3730, 3858, 4027, 4086, 4390, 4558, 4684, 4925, 5249
	};
	static char* lc_FirstLetter[] = {
		"a", "b", "c", "d", "e", "f", "g", "h", "j", "k", "l", "m", "n", "o",
		"p", "q", "r", "s", "t", "w", "x", "y", "z"
	};
	static char* ls_SecondSecTable =
		"CJWGNSPGCGNE[Y[BTYYZDXYKYGT[JNNJQMBSGZSCYJSYY[PGKBZGY[YWJKGKLJYWKPJQHY[W[DZLSGMRYPYWWCCKZNKYYGTTNJJNYKKZYTCJNMCYLQLYPYQFQRPZSLWBTGKJFYXJWZLTBNCXJJJJTXDTTSQZYCDXXHGCK[PHFFSS[YBGXLPPBYLL[HLXS[ZM[JHSOJNGHDZQYKLGJHSGQZHXQGKEZZWYSCSCJXYEYXADZPMDSSMZJZQJYZC[J[WQJBYZPXGZNZCPWHKXHQKMWFBPBYDTJZZKQHY"
		"LYGXFPTYJYYZPSZLFCHMQSHGMXXSXJ[[DCSBBQBEFSJYHXWGZKPYLQBGLDLCCTNMAYDDKSSNGYCSGXLYZAYBNPTSDKDYLHGYMYLCXPY[JNDQJWXQXFYYFJLEJPZRXCCQWQQSBNKYMGPLBMJRQCFLNYMYQMSQYRBCJTHZTQFRXQHXMJJCJLXQGJMSHZKBSWYEMYLTXFSYDSWLYCJQXSJNQBSCTYHBFTDCYZDJWYGHQFRXWCKQKXEBPTLPXJZSRMEBWHJLBJSLYYSMDXLCLQKXLHXJRZJMFQHXHWY"
		"WSBHTRXXGLHQHFNM[YKLDYXZPYLGG[MTCFPAJJZYLJTYANJGBJPLQGDZYQYAXBKYSECJSZNSLYZHSXLZCGHPXZHZNYTDSBCJKDLZAYFMYDLEBBGQYZKXGLDNDNYSKJSHDLYXBCGHXYPKDJMMZNGMMCLGWZSZXZJFZNMLZZTHCSYDBDLLSCDDNLKJYKJSYCJLKWHQASDKNHCSGANHDAASHTCPLCPQYBSDMPJLPZJOQLCDHJJYSPRCHN[NNLHLYYQYHWZPTCZGWWMZFFJQQQQYXACLBHKDJXDGMMY"
		"DJXZLLSYGXGKJRYWZWYCLZMSSJZLDBYD[FCXYHLXCHYZJQ[[QAGMNYXPFRKSSBJLYXYSYGLNSCMHZWWMNZJJLXXHCHSY[[TTXRYCYXBYHCSMXJSZNPWGPXXTAYBGAJCXLY[DCCWZOCWKCCSBNHCPDYZNFCYYTYCKXKYBSQKKYTQQXFCWCHCYKELZQBSQYJQCCLMTHSYWHMKTLKJLYCXWHEQQHTQH[PQ[QSCFYMNDMGBWHWLGSLLYSDLMLXPTHMJHWLJZYHZJXHTXJLHXRSWLWZJCBXMHZQXSDZP"
		"MGFCSGLSXYMJSHXPJXWMYQKSMYPLRTHBXFTPMHYXLCHLHLZYLXGSSSSTCLSLDCLRPBHZHXYYFHB[GDMYCNQQWLQHJJ[YWJZYEJJDHPBLQXTQKWHLCHQXAGTLXLJXMSL[HTZKZJECXJCJNMFBY[SFYWYBJZGNYSDZSQYRSLJPCLPWXSDWEJBJCBCNAYTWGMPAPCLYQPCLZXSBNMSGGFNZJJBZSFZYNDXHPLQKZCZWALSBCCJX[YZGWKYPSGXFZFCDKHJGXDLQFSGDSLQWZKXTMHSBGZMJZRGLYJB"
		"PMLMSXLZJQQHZYJCZYDJWBMYKLDDPMJEGXYHYLXHLQYQHKYCWCJMYYXNATJHYCCXZPCQLBZWWYTWBQCMLPMYRJCCCXFPZNZZLJPLXXYZTZLGDLDCKLYRZZGQTGJHHGJLJAXFGFJZSLCFDQZLCLGJDJCSNZLLJPJQDCCLCJXMYZFTSXGCGSBRZXJQQCTZHGYQTJQQLZXJYLYLBCYAMCSTYLPDJBYREGKLZYZHLYSZQLZNWCZCLLWJQJJJKDGJZOLBBZPPGLGHTGZXYGHZMYCNQSYCYHBHGXKAMTX"
		"YXNBSKYZZGJZLQJDFCJXDYGJQJJPMGWGJJJPKQSBGBMMCJSSCLPQPDXCDYYKY[CJDDYYGYWRHJRTGZNYQLDKLJSZZGZQZJGDYKSHPZMTLCPWNJAFYZDJCNMWESCYGLBTZCGMSSLLYXQSXSBSJSBBSGGHFJLYPMZJNLYYWDQSHZXTYYWHMZYHYWDBXBTLMSYYYFSXJC[DXXLHJHF[SXZQHFZMZCZTQCXZXRTTDJHNNYZQQMNQDMMG[YDXMJGDHCDYZBFFALLZTDLTFXMXQZDNGWQDBDCZJDXBZGS"
		"QQDDJCMBKZFFXMKDMDSYYSZCMLJDSYNSBRSKMKMPCKLGDBQTFZSWTFGGLYPLLJZHGJ[GYPZLTCSMCNBTJBQFKTHBYZGKPBBYMTDSSXTBNPDKLEYCJNYDDYKZDDHQHSDZSCTARLLTKZLGECLLKJLQJAQNBDKKGHPJTZQKSECSHALQFMMGJNLYJBBTMLYZXDCJPLDLPCQDHZYCBZSCZBZMSLJFLKRZJSNFRGJHXPDHYJYBZGDLQCSEZGXLBLGYXTWMABCHECMWYJYZLLJJYHLG[DJLSLYGKDZPZXJ"
		"YYZLWCXSZFGWYYDLYHCLJSCMBJHBLYZLYCBLYDPDQYSXQZBYTDKYXJY[CNRJMPDJGKLCLJBCTBJDDBBLBLCZQRPPXJCJLZCSHLTOLJNMDDDLNGKAQHQHJGYKHEZNMSHRP[QQJCHGMFPRXHJGDYCHGHLYRZQLCYQJNZSQTKQJYMSZSWLCFQQQXYFGGYPTQWLMCRNFKKFSYYLQBMQAMMMYXCTPSHCPTXXZZSMPHPSHMCLMLDQFYQXSZYYDYJZZHQPDSZGLSTJBCKBXYQZJSGPSXQZQZRQTBDKYXZK"
		"HHGFLBCSMDLDGDZDBLZYYCXNNCSYBZBFGLZZXSWMSCCMQNJQSBDQSJTXXMBLTXZCLZSHZCXRQJGJYLXZFJPHYMZQQYDFQJJLZZNZJCDGZYGCTXMZYSCTLKPHTXHTLBJXJLXSCDQXCBBTJFQZFSLTJBTKQBXXJJLJCHCZDBZJDCZJDCPRNPQCJPFCZLCLZXZDMXMPHJSGZGSZZQLYLWTJPFSYASMCJBTZKYCWMYTCSJJLJCQLWZMALBXYFBPNLSFHTGJWEJJXXGLLJSTGSHJQLZFKCGNNNSZFDEQ"
		"FHBSAQTGYLBXMMYGSZLDYDQMJJRGBJTKGDHGKBLQKBDMBYLXWCXYTTYBKMRTJZXQJBHLMHMJJZMQASLDCYXYQDLQCAFYWYXQHZ";
	std::string result;
	int H = 0;
	int L = 0;
	int W = 0;
	UINT stringlen = strlen(strChs);
	for (UINT i = 0; i < stringlen; i++) {
		H = (UCHAR)(strChs[i + 0]);
		L = (UCHAR)(strChs[i + 1]);
		if (H < 0xA1 || L < 0xA1) {
			result += strChs[i];
			continue;
		}
		else {
			W = (H - 160) * 100 + L - 160;
		}
		if (W > 1600 && W < 5590) {
			for (int j = 22; j >= 0; j--) {
				if (W >= li_SecPosValue[j]) {
					result += lc_FirstLetter[j];
					i++;
					break;
				}
			}
			continue;
		}
		else {
			i++;
			W = (H - 160 - 56) * 94 + L - 161;
			if (W >= 0 && W <= 3007)
				result += ls_SecondSecTable[W];
			else {
				result += (char)H;
				result += (char)L;
			}
		}
	}

	return result;
}


string HZ2FirstPY(IN std::string szHZ)
{
	std::string str = GetFirstLetter(szHZ.c_str());

	return str;
}


char* ConvertChineseUnicodeToPyt(const char * chrstr)
{
	const  int pyvalue[] = { -20319, -20317, -20304, -20295, -20292, -20283, -20265, -20257, -20242, -20230, -20051, -20036, -20032, -20026,
		-20002, -19990, -19986, -19982, -19976, -19805, -19784, -19775, -19774, -19763, -19756, -19751, -19746, -19741, -19739, -19728,
		-19725, -19715, -19540, -19531, -19525, -19515, -19500, -19484, -19479, -19467, -19289, -19288, -19281, -19275, -19270, -19263,
		-19261, -19249, -19243, -19242, -19238, -19235, -19227, -19224, -19218, -19212, -19038, -19023, -19018, -19006, -19003, -18996,
		-18977, -18961, -18952, -18783, -18774, -18773, -18763, -18756, -18741, -18735, -18731, -18722, -18710, -18697, -18696, -18526,
		-18518, -18501, -18490, -18478, -18463, -18448, -18447, -18446, -18239, -18237, -18231, -18220, -18211, -18201, -18184, -18183,
		-18181, -18012, -17997, -17988, -17970, -17964, -17961, -17950, -17947, -17931, -17928, -17922, -17759, -17752, -17733, -17730,
		-17721, -17703, -17701, -17697, -17692, -17683, -17676, -17496, -17487, -17482, -17468, -17454, -17433, -17427, -17417, -17202,
		-17185, -16983, -16970, -16942, -16915, -16733, -16708, -16706, -16689, -16664, -16657, -16647, -16474, -16470, -16465, -16459,
		-16452, -16448, -16433, -16429, -16427, -16423, -16419, -16412, -16407, -16403, -16401, -16393, -16220, -16216, -16212, -16205,
		-16202, -16187, -16180, -16171, -16169, -16158, -16155, -15959, -15958, -15944, -15933, -15920, -15915, -15903, -15889, -15878,
		-15707, -15701, -15681, -15667, -15661, -15659, -15652, -15640, -15631, -15625, -15454, -15448, -15436, -15435, -15419, -15416,
		-15408, -15394, -15385, -15377, -15375, -15369, -15363, -15362, -15183, -15180, -15165, -15158, -15153, -15150, -15149, -15144,
		-15143, -15141, -15140, -15139, -15128, -15121, -15119, -15117, -15110, -15109, -14941, -14937, -14933, -14930, -14929, -14928,
		-14926, -14922, -14921, -14914, -14908, -14902, -14894, -14889, -14882, -14873, -14871, -14857, -14678, -14674, -14670, -14668,
		-14663, -14654, -14645, -14630, -14594, -14429, -14407, -14399, -14384, -14379, -14368, -14355, -14353, -14345, -14170, -14159,
		-14151, -14149, -14145, -14140, -14137, -14135, -14125, -14123, -14122, -14112, -14109, -14099, -14097, -14094, -14092, -14090,
		-14087, -14083, -13917, -13914, -13910, -13907, -13906, -13905, -13896, -13894, -13878, -13870, -13859, -13847, -13831, -13658,
		-13611, -13601, -13406, -13404, -13400, -13398, -13395, -13391, -13387, -13383, -13367, -13359, -13356, -13343, -13340, -13329,
		-13326, -13318, -13147, -13138, -13120, -13107, -13096, -13095, -13091, -13076, -13068, -13063, -13060, -12888, -12875, -12871,
		-12860, -12858, -12852, -12849, -12838, -12831, -12829, -12812, -12802, -12607, -12597, -12594, -12585, -12556, -12359, -12346,
		-12320, -12300, -12120, -12099, -12089, -12074, -12067, -12058, -12039, -11867, -11861, -11847, -11831, -11798, -11781, -11604,
		-11589, -11536, -11358, -11340, -11339, -11324, -11303, -11097, -11077, -11067, -11055, -11052, -11045, -11041, -11038, -11024,
		-11020, -11019, -11018, -11014, -10838, -10832, -10815, -10800, -10790, -10780, -10764, -10587, -10544, -10533, -10519, -10331,
		-10329, -10328, -10322, -10315, -10309, -10307, -10296, -10281, -10274, -10270, -10262, -10260, -10256, -10254 };
	const char pystr[396][7] = { "a", "ai", "an", "ang", "ao", "ba", "bai", "ban", "bang", "bao", "bei", "ben", "beng", "bi", "bian", "biao",
		"bie", "bin", "bing", "bo", "bu", "ca", "cai", "can", "cang", "cao", "ce", "ceng", "cha", "chai", "chan", "chang", "chao", "che", "chen",
		"cheng", "chi", "chong", "chou", "chu", "chuai", "chuan", "chuang", "chui", "chun", "chuo", "ci", "cong", "cou", "cu", "cuan", "cui",
		"cun", "cuo", "da", "dai", "dan", "dang", "dao", "de", "deng", "di", "dian", "diao", "die", "ding", "diu", "dong", "dou", "du", "duan",
		"dui", "dun", "duo", "e", "en", "er", "fa", "fan", "fang", "fei", "fen", "feng", "fo", "fou", "fu", "ga", "gai", "gan", "gang", "gao",
		"ge", "gei", "gen", "geng", "gong", "gou", "gu", "gua", "guai", "guan", "guang", "gui", "gun", "guo", "ha", "hai", "han", "hang",
		"hao", "he", "hei", "hen", "heng", "hong", "hou", "hu", "hua", "huai", "huan", "huang", "hui", "hun", "huo", "ji", "jia", "jian",
		"jiang", "jiao", "jie", "jin", "jing", "jiong", "jiu", "ju", "juan", "jue", "jun", "ka", "kai", "kan", "kang", "kao", "ke", "ken",
		"keng", "kong", "kou", "ku", "kua", "kuai", "kuan", "kuang", "kui", "kun", "kuo", "la", "lai", "lan", "lang", "lao", "le", "lei",
		"leng", "li", "lia", "lian", "liang", "liao", "lie", "lin", "ling", "liu", "long", "lou", "lu", "lv", "luan", "lue", "lun", "luo",
		"ma", "mai", "man", "mang", "mao", "me", "mei", "men", "meng", "mi", "mian", "miao", "mie", "min", "ming", "miu", "mo", "mou", "mu",
		"na", "nai", "nan", "nang", "nao", "ne", "nei", "nen", "neng", "ni", "nian", "niang", "niao", "nie", "nin", "ning", "niu", "nong",
		"nu", "nv", "nuan", "nue", "nuo", "o", "ou", "pa", "pai", "pan", "pang", "pao", "pei", "pen", "peng", "pi", "pian", "piao", "pie",
		"pin", "ping", "po", "pu", "qi", "qia", "qian", "qiang", "qiao", "qie", "qin", "qing", "qiong", "qiu", "qu", "quan", "que", "qun",
		"ran", "rang", "rao", "re", "ren", "reng", "ri", "rong", "rou", "ru", "ruan", "rui", "run", "ruo", "sa", "sai", "san", "sang",
		"sao", "se", "sen", "seng", "sha", "shai", "shan", "shang", "shao", "she", "shen", "sheng", "shi", "shou", "shu", "shua",
		"shuai", "shuan", "shuang", "shui", "shun", "shuo", "si", "song", "sou", "su", "suan", "sui", "sun", "suo", "ta", "tai",
		"tan", "tang", "tao", "te", "teng", "ti", "tian", "tiao", "tie", "ting", "tong", "tou", "tu", "tuan", "tui", "tun", "tuo",
		"wa", "wai", "wan", "wang", "wei", "wen", "weng", "wo", "wu", "xi", "xia", "xian", "xiang", "xiao", "xie", "xin", "xing",
		"xiong", "xiu", "xu", "xuan", "xue", "xun", "ya", "yan", "yang", "yao", "ye", "yi", "yin", "ying", "yo", "yong", "you",
		"yu", "yuan", "yue", "yun", "za", "zai", "zan", "zang", "zao", "ze", "zei", "zen", "zeng", "zha", "zhai", "zhan", "zhang",
		"zhao", "zhe", "zhen", "zheng", "zhi", "zhong", "zhou", "zhu", "zhua", "zhuai", "zhuan", "zhuang", "zhui", "zhun", "zhuo",
		"zi", "zong", "zou", "zu", "zuan", "zui", "zun", "zuo" };

	int chrasc = 0;

	char* pcReturnString = NULL;


	int length = strlen(chrstr);
	const char* nowchar = chrstr;

	//转换ANSI,字符部分不变，汉字转换成相应的拼音
	char *returnstr = new char[6 * length + 1];
	memset(returnstr, 0, 6 * length + 1);

	int offset = 0;
	for (int j = 0; j < length;) // 循环处理字节数组
	{
		if (nowchar[j] >= 0 && nowchar[j] < 128) // 非汉字处理
		{ 
			returnstr[offset] = nowchar[j];
			offset++;
			j++;
			continue;
		}

		// 汉字处理
		chrasc = nowchar[j] * 256 + nowchar[j + 1] + 256;

		if (chrasc > 0 && chrasc < 160)
		{
			returnstr[offset] = nowchar[j];
			offset++;
			j++;
		}
		else
		{
			for (int i = (sizeof(pyvalue) / sizeof(pyvalue[0]) - 1); i >= 0; i--)
			{
				if (pyvalue[i] <= chrasc)
				{
					strcpy(returnstr + offset, pystr[i]);

					offset += strlen(pystr[i]);
					break;
				}
			}
			j += 2;
		}
	}
	if (strlen(returnstr) > 0)
	{
		pcReturnString = new char[strlen(returnstr) + 1];
		memset(pcReturnString, 0, strlen(returnstr) + 1);
		strcpy(pcReturnString, returnstr);
	}
	delete[]returnstr;
	//delete[]nowchar;

	return pcReturnString;

}

string HZ2AllPY(IN string szHZ)
{
	char* psz = ConvertChineseUnicodeToPyt(szHZ.c_str());

	return psz;
}

Int32 splitString(__in std::wstring src, __in std::vector<std::wstring> _vecSpliter,
	__out std::vector<std::wstring> &_splitList)
{
	if (src.empty() || _vecSpliter.empty())
	{
		return 0;
	}

	int iSrcLen = wcslen(src.c_str());
	int iSplitLen = _vecSpliter.size();

	int iPrevOccurIdx = 0;
	for (int i = 0; i < iSrcLen; i++)
	{
		for (int m = 0; m < iSplitLen; m++)
		{
			std::wstring _spliter = _vecSpliter[m];
			if (_wcsnicmp(_spliter.c_str(), src.c_str() + i, _spliter.length()))
			{
				continue;
			}

			int iCopyLen = i - iPrevOccurIdx + 1;
			if (iCopyLen > 1)
			{
				wchar_t *strMid = new wchar_t[i - iPrevOccurIdx + 1];
				memset(strMid, 0, iCopyLen * sizeof(wchar_t));
				wcsncpy(strMid, src.c_str() + iPrevOccurIdx, iCopyLen - 1);

				iPrevOccurIdx = i + _spliter.length();

				_splitList.push_back(strMid);

				delete[]strMid;
			}

			break;
		}

		if (iPrevOccurIdx >= iSrcLen)
		{
			break;
		}
	}

	if (iPrevOccurIdx < iSrcLen)
	{
		_splitList.push_back(src.c_str() + iPrevOccurIdx);
	}

	return _splitList.size();
}

Int32 splitString(__in std::string src, __in std::vector<std::string> _vecSpliter,
	__out std::vector<std::string> &_splitList)
{
	if (src.empty() || _vecSpliter.empty())
	{
		return 0;
	}

	int iSrcLen = strlen(src.c_str());
	int iSplitLen = _vecSpliter.size();

	int iPrevOccurIdx = 0;
	for (int i = 0; i < iSrcLen; i++)
	{
		for (int m = 0; m < iSplitLen; m++)
		{
			std::string _spliter = _vecSpliter[m];
			if (strncmp(_spliter.c_str(), src.c_str() + i, _spliter.length()))
			{
				continue;
			}

			int iCopyLen = i - iPrevOccurIdx + 1;
			if (iCopyLen > 1)
			{
				char *strMid = new char[i - iPrevOccurIdx + 1];
				memset(strMid, 0, iCopyLen * sizeof(char));
				strncpy(strMid, src.c_str() + iPrevOccurIdx, iCopyLen - 1);

				iPrevOccurIdx = i + _spliter.length();

				_splitList.push_back(strMid);

				delete[] strMid;
			}

			break;
		}

		if (iPrevOccurIdx >= iSrcLen)
		{
			break;
		}
	}

	if (iPrevOccurIdx < iSrcLen)
	{
		_splitList.push_back(src.c_str() + iPrevOccurIdx);
	}

	return _splitList.size();
}

bool StringContains(std::string& sLong, std::string& sShort, bool bCaseSensitive)
{
	bool bRet = false;
	// 	 if( sLong.length() < sShort.length() ) //"魏润平" < "weirunping";
	// 		 return bRet;
	if( sLong.length() == 0 )
	{
		if( sShort.length() == 0 )
			return true;
		else
			return false;
	}

	std::string sLong1 = sLong;
	std::string sShort1 = sShort;
	if( !bCaseSensitive )
	{
		transform(sLong1.begin(), sLong1.end(), sLong1.begin(), ::tolower);
		transform(sShort1.begin(), sShort1.end(), sShort1.begin(), ::tolower);
	}

	if( isIncludeChinese(sShort1) )//搜索字符串中含有中文;
	{
		if( sLong1.find(sShort1) !=  string::npos )
			bRet = true;
	}
	else
	{
		if( utils::isIncludeChinese(sLong1) )//被搜索字符串中含有中文;
		{
			string firstPY = utils::HZ2FirstPY(sLong1);//汉字转化成拼音都是小写;
			if(firstPY.find(sShort1) !=  string::npos)//先检索简拼
			{
				bRet = true;
			}
			else
			{
				string allPY = utils::HZ2AllPY(sLong1);//再检索全拼
				if(allPY.find(sShort1) !=  string::npos)
					bRet = true;
			}
		}
		else
		{
			if( sLong1.find(sShort1) != string::npos )
				bRet = true;
		}		 
	}
	return bRet;
}

int _tmain(int argc, _TCHAR* argv[])
{
	return 0;
}

