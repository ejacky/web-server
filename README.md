# tiny-server
a web server  , written by c language

# 问题
- [ ] response 超过 150 ，则 无法访问
- 	  已经解决 
- [ ] printf 无法在命令行显示。但在 read_requesthdrs 函数中却可以。 
- [ ] 输出信息乱码
- [ ] 测试并发量， 目前太小 
-	  初步解决， 通过调大 listen 队列 
- [ ] 用 php 执行动态服务 
- [ ] fork 函数 
- [ ] 将 头文件， 通用文件， 从主文件分离 

# 使用
- 静态访问： http://host/home.html
- 动态访问:  http://host/cgi-bin/addr?30&90
  得到结果 120 
  
  
# 压力测试
- ab -n10000 -c 1000 http://localhost/demo 
