# WashFlow 同机测试

## 文件说明

- `test_wash_flow.py`：同机集成测试脚本，通过 HTTP 模拟冲洗抓拍机、AI 摄像头、串口光电/水泵。
- 被测程序需要以 **测试模式** 编译，才会注册 `/test/serial` 和 `/test/status` 路由。

## 编译测试版本

```bash
cd /path/to/CarClean-ai6_1106_carsimple
mkdir build && cd build
cmake .. -DWASH_TEST_MODE=ON && make -j
```

## 准备运行

1. 修改 `src/net_cfg.json`，让上报指向本机 mock 服务器：

```json
{
    "local_server": "127.0.0.1",
    "local_port": 8080,
    "remote_server": "127.0.0.1",
    "remote_port": 9999
}
```

2. 启动被测程序：

```bash
./CarClean
```

## 运行测试脚本

### 交互式菜单

```bash
python3 tests/test_wash_flow.py
```

运行后会显示菜单，按数字选择场景：

```
========== WashFlow 测试菜单 ==========
  [1] 正常干净车
  [2] 脏车
  [3] 水泵未工作
  [4] 抓拍机无车牌
  [5] B点先触发后拍车牌
  [6] 查询传感器状态
  [0] 退出
======================================
```

### 命令行直接执行

```bash
python3 tests/test_wash_flow.py 1   # 正常干净车
python3 tests/test_wash_flow.py 2   # 脏车
python3 tests/test_wash_flow.py 3   # 水泵未工作
python3 tests/test_wash_flow.py 4   # 抓拍机无车牌
python3 tests/test_wash_flow.py 5   # B点先触发后拍车牌
python3 tests/test_wash_flow.py 6   # 查询传感器状态
```

## 测试场景说明

| 编号 | 场景 | 预期结果 |
|---|---|---|
| 1 | 正常干净车 | 收到 `dataType=1` 上报，`cleanRes=2` |
| 2 | 脏车 | 收到上报，`cleanRes=3` |
| 3 | 水泵未工作 | 收到上报，`alarmType=3` |
| 4 | 抓拍机无车牌 | 不应收到 `dataType=1` 上报 |
| 5 | B点先触发后拍车牌 | 仍能正常收到上报 |
| 6 | 查询传感器状态 | 打印当前 point_b / water_pump / ipc / ai_ipc 状态 |
