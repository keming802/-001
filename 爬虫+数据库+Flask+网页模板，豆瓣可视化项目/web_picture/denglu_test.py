1 from etc import jboxenv
 2 from selenium import webdriver
 3 from selenium.webdriver.common.by import By
 4 from selenium.webdriver.support import expected_conditions as EC
 5 from selenium.webdriver.support.wait import WebDriverWait
 6
 7 logger = jboxenv.JBOX_LOGGER
 8 __jbox_cookies = {}
12 __header = {
13     'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3112.78 Safari/537.36'}
14
15
16 def get_user_cookies(username, password):
17     '''
18     获取JBOX的登录cookies
19     模块第一次使用时通过账号密码获取cookie
20     登录成功以后再申请cookie，读取模块中的缓存
21     :param username: 用户名
22     :param password: 密码
23     :return: 返回一个dict,保存了cookies字典
24     '''
25     # 打开首页获取cookies
26     global __jbox_cookies
27     if __jbox_cookies.keys():
28         return __jbox_cookies
29     else:
30         driver = webdriver.Chrome(r'C:\Program Files (x86)\Google\Chrome\Application\chromedriver.exe')
31         try:
32             driver.get("http://pan.jd.com/")
33             name = driver.find_element_by_id('account')
34             name.send_keys(username)
35             psd = driver.find_element_by_id('password')
36             psd.send_keys(password)
37             btn = driver.find_element_by_class_name('btnsubmint')
38             btn.click()
39             # 等待新页面出现的某个元素出现
40             wait = WebDriverWait(driver, 10)
41             wait.until(EC.presence_of_element_located((By.ID, 'showMine')))
42             if len(driver.get_cookies())>0:
43                 for c in driver.get_cookies():
44                     __jbox_cookies[c.get('name')] = c.get('value')
45         finally:
46             driver.close()
47         #如果没有登录成功，就提醒需要重新登录
48         if not __jbox_cookies.keys():
            logger.error('通过登录JBOX申请cookies,登录结果:失败')
     print(__jbox_cookies)
     return __jbox_cookies


if __name__ == '__main__':
    print(get_user_cookies('shwujiang', 'Fig@2016092404'))