using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.SQLite;

/*
 * @Purpose: 使用Microsoft的PinyinConverter获取汉字的拼音, 只支持20590个汉字,剩下418个未识别的汉字(有个位数的是占位的汉字--没有实际字形),
 *           手工完成拼音的翻译后, 利用此程序合并[GBK_Hanzi]+[GBK_Hanzi_MS_NOT_SUPPORT]-->[GBK_Hanzi_Publish].
 *           发布的table要尽可能的简洁,包含的字段有:GBKVal+Pinyins;
 * @Date: 2017/12/05 17:59;
 * @Author: jian.he;
 * @Mail: worksdata@163.com;
 * 
 */

namespace MergeMsUnsupportHanziToDB
{
    class Program
    {
        static void MergeMsUnsupportHanziToPublishTable()//其它两个表可以手动删除! 但是一定要做好备份!
        {
            const string kHanziMsSupportTb = "GBK_Hanzi";
            const string kHanziMsNotSupportFixedTb = "GBK_Hanzi_MS_NOT_SUPPORT_FIXED";
            const string kHanziPublishTb = "GBK_Hanzi_Publish";
            string sTmp;

            //=================================
            //创建DB TABLE [GBK_Hanzi_Publish];
            //=================================
            sTmp = @"..\..\..\data\hanzi.db";
            if (!System.IO.File.Exists(sTmp))
                return;

            SQLiteConnection connDB = new SQLiteConnection("data source=" + sTmp);
            if (connDB.State != System.Data.ConnectionState.Open)
            {
                connDB.Open();
                SQLiteCommand cmd = connDB.CreateCommand();
                cmd.Connection = connDB;
                cmd.CommandText = "DROP TABLE IF EXISTS " + kHanziPublishTb + ";";//";CREATE TABLE " + kHanziPublishTb + "(GBKVAL INTEGER PRIMARY KEY, PINYINS VARCHAR);";

                //合并MS支持和不支持的两个表中的数据到发布表(+排序);
                sTmp = "CREATE TABLE "+ kHanziPublishTb +" AS SELECT m.GBKVAL,m.PINYINS FROM (SELECT GBKVAL,PINYINS FROM " + kHanziMsSupportTb +
                " UNION ALL SELECT GBKVAL,PINYINS FROM " + kHanziMsNotSupportFixedTb + " WHERE PINYINS <> '') AS m ORDER BY m.GBKVAL ASC;";

                cmd.CommandText += sTmp;

                cmd.ExecuteNonQuery();
            }
                        

            connDB.Close();           
        }

        static void Main(string[] args)
        {
            MergeMsUnsupportHanziToPublishTable();

            Console.WriteLine("GAME OVER.");
            Console.ReadKey();
        }
    }
}
