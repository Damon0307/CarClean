import http.client

# 创建一个 HTTP 连接
conn = http.client.HTTPConnection("localhost", 8080)

# 发送 GET 请求
conn.request("GET", "/hi")

# 获取响应
response = conn.getresponse()

# 打印响应状态码和响应内容
print("Response Status:", response.status)
print("Response Content:")
print(response.read().decode())

# 关闭连接
conn.close()
