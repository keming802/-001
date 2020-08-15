from selenium import webdriver
from time import sleep
browser = webdriver.Chrome()
browser.get('http://www.baidu.com')
sleep(2)  #等待网页加载
browser.find_element_by_css_selector('#u1 .lb').click()  #通过class来获取DOM元素
#browser.find_element_by_css_selector("#u1 > a[name='tj_login']").click() #通过标签来获取取DOM元素
sleep(2)  #等待网页加载
browser.find_element_by_id('TANGRAM__PSP_10__footerULoginBtn').click()
browser.find_element_by_id('TANGRAM__PSP_10__userName').send_keys('18817673505')
browser.find_element_by_id('TANGRAM__PSP_10__password').send_keys('158cheng')
browser.find_element_by_id('TANGRAM__PSP_10__submit').click(