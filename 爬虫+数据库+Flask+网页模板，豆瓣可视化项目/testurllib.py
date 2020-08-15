import urllib.request
import urllib.parse
# #获取一个get请求
# response=urllib.request.urlopen('https://www.baidu.com/')
# print(response.read().decode('utf-8'))
#
# #获取一个post
# import urllib.parse
# data=bytes(urllib.parse.urlencode({'hello':'world'}),encoding='utf-8')
# response=urllib.request.urlopen('http://httpbin.org/post',data=data)
# print(response.read())
#
# #超时处理
# try:
#    response=urllib.request.urlopen('http://httpbin.org/get',timeout=1)
#    print(response.read().decode('utf-8'))
# except urllib.error.URLError as e:
#     print('time out!')
url="https://www.douban.com"
headers={
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.61 Safari/537.36'
}

req=urllib.request.Request(url=url,headers=headers)
response=urllib.request.urlopen(req)
print(response.read().decode("utf-8"))