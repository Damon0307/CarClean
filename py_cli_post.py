import http.client
import json

# 创建一个 HTTP 连接
conn = http.client.HTTPConnection("localhost", 8080)

# 构建要发送的 JSON 数据
data = {
    "hello": "wjc"
}
json_data = json.dumps(data)

# 设置请求头
headers = {
    "Content-Type": "application/json"
}

# 发送 POST 请求
conn.request("POST", "/ipc", body=json_data, headers=headers)

# 获取响应
response = conn.getresponse()

# 打印响应状态码和响应内容
print("Response Status:", response.status)
print("Response Content:")
print(response.read().decode())

# 关闭连接
conn.close()
