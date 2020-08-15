import re

pat=re.compile('AA')
#m=pat.search("ABCAA")
#m=pat.search("AABCAADDCCAA") #search方法进行对比

m=re.search('asd','Aasd')
print(m)
print(re.findall('[A-Z]+','ASDaDFGAa'))
print(re.sub('a','A','abcdcasd'))
