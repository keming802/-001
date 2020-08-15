from bs4 import BeautifulSoup
import xlwt
import re
import urllib.request,urllib.error
import sqlite3

def main():
    baseurl='https://movie.douban.com/top250?start='
    #1.爬取网页
    datalist = getData(baseurl)
    savepath = '.\\豆瓣电影Top250.xls'
    #2.解析数据
    dbpath='movie.db'
    #3.保存数据
    #saveData(datalist,savepath)
    saveData2DB(datalist,dbpath)

    askURL('https://movie.douban.com/top250?start=0')

findlink=re.compile(r'<a href="(.*?)">')       #创建正则表达式对象，表示规则（字符串模式）
#影片图片
findImgSrc=re.compile(r'<img.*src="(.*?)"',re.S)  #re.S让换行符包含在字符中
#影片的片名
findTitle=re.compile(r'<span class="title">(.*)</span>')
#影片评分
findRating=re.compile(r'<span class="rating_num" property="v:average">(.*)</span>')
#找到评价人数
findJudge=re.compile(r'<span>(\d*)+人评价</span>')
#找到概况
findInq=re.compile('<span class="inq">(.*)</span>')
#找到影片的相关内容
findBd=re.compile(r'<p class="">(.*?)</p>',re.S)

#爬取网页
def getData(baseurl):
    datalist=[]
    for i in range(0,10):   #调用获取页面信息的函数，10次
        url=baseurl+str(i*25)
        html=askURL(url)    #保存获取网页   #
        #2.逐一解析数据
        soup=BeautifulSoup(html,"html.parser")
        for item in soup.find_all('div',class_="item"):
            #print(item)  #测试查看item全部信息
            data=[]
            item=str(item)
            # print(item)
            # break

            #影片详情的超链接
            link=re.findall(findlink,item)[0]    #re库用来查找正则表达式
            data.append(link)
            imgSrc=re.findall(findImgSrc,item)[0]   #添加图片
            data.append(imgSrc)
            titles=re.findall(findTitle,item)     #添加名字
            if len(titles) == 2:
                ctitle=titles[0]
                data.append(ctitle)               #添加中文名
                otitle=titles[1].replace('/','')  #去掉无关的符号
                data.append(otitle)             #添加外国名
            else:
                data.append(titles[0])
                data.append(' ')             #外国名字留空

            rating=re.findall(findRating,item)[0]
            data.append(rating)             #添加评分

            judgeNum=re.findall(findJudge,item)[0]
            data.append(str(judgeNum)+"人评价")           #添加评价

            inq=re.findall(findInq,item)
            if len(inq)!=0:
                inq=inq[0].replace('。','')    #去掉句号
                data.append(inq)  # 添加概述
            else:
                data.append(" ")


            bd=re.findall(findBd,item)[0]
            bd=re.sub('<br(\s+)?/>(\s+)?',' ',bd) #去掉br
            bd=re.sub('/',' ',bd)     #替换/
            data.append(bd.strip())  #去掉前后的空格

            datalist.append(data)
    # print(datalist)

    return datalist

#得到一个指定URL网页的内容
# 得到一个指定URL网页的内容
def askURL(url):
    head={      #模拟流浪器头部信息，向豆瓣浏览器发送信息
        "User-Agent": "Mozilla / 5.0(Windows NT 10.0;Win64;x64) AppleWebKit / 537.36(KHTML, like Gecko) Chrome / 83.0.4103.97Safari / 537.36"
    }
             #用户代理，表示告诉浏览器我们是什么类型的机器浏览器
    request=urllib.request.Request(url,headers=head)
    html=''
    try:
        response=urllib.request.urlopen(request)
        html=response.read().decode('utf-8')
        #print(html)
    except urllib.error.URLError as e:
        if hasattr(e,"code"):
            print(e.code)
        if hasattr(e,"reason"):
            print(e.reason)
    return html

def saveData(datalist,savepath):
    book=xlwt.Workbook(encoding='utf-8')
    sheet=book.add_sheet('douan_Top250',cell_overwrite_ok=True)
    col=('电影详情链接','图片链接','影片中文名','影片外国名','评分','评价数','概括','相关信息')
    for i in range(0,8):
        sheet.write(0,i,col[i])#列名
    for i in range(0,250):
        print('第%d条'%i)
        data=datalist[i]
        for j in range(0,8):
            sheet.write(i+1,j,data[j])
    book.save('豆瓣.xls')

    #print("save.....")

def saveData2DB(datalist,dbpath):
    init_db(dbpath)
    conn=sqlite3.connect(dbpath)
    cur=conn.cursor()

    for data in datalist:
        for index in range(len(data)):
            if index==4 :
                continue

            data[index]='"'+data[index]+'"'
        sql="""
            insert into movie250(
            info_link,pic_link,cname,ename,score,rated,instroduction,info)
            values(%s)""" %",".join(data)
        print(sql)
        cur.execute(sql)
        conn.commit()
    cur.close()
    conn.close()
def init_db(dbpath):
    sql="""
        create table movie250
        (
        id integer primary key autoincrement,
        info_link text,
        pic_link text,
        cname varchar,
        ename varchar,
        score numeric,
        rated text,
        instroduction text,
        info text
        )
    """ #创建数据表
    conn=sqlite3.connect(dbpath)
    cursor=conn.cursor()
    cursor.execute(sql)
    conn.commit()
    conn.close()

if __name__=='__main__':
    main()
    #init_db('movietest.db')
    print('爬取完毕')

