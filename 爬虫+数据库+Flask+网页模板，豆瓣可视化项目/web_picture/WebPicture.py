from selenium import webdriver
from time import sleep

driver=webdriver.Chrome()
driver.get("http://www.baidu.com")
sleep(2)
#网页截图并保存
driver.get_screenshot_as_file(r'D:\PYTHON书籍\爬虫\web_picture\1.png')
print("over!")
