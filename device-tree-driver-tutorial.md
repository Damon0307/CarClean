# Linux 设备树与驱动开发入门教程

> 面向对象:有 Linux 应用开发经验、没接触过底层驱动的开发者
> 实战环境:Luckfox Pico Ultra(RV1106)+ SDK 目录 `/home/wjc/luckfox-pico` + 主线案例(本文所有路径/命令都是你机器上的真实路径)

---

## 0. 为什么要学设备树(从今天的真实故障说起)

今天你遇到的故障是学习设备树最好的教材:

- **应用层一切正常**:能打开 `/dev/ttyS3`、能通过它发数据
- **但收不到数据**:板子 TX 正常、RX 完全没反应
- **根因在设备树**:UART3 同时挂了 `uart3m0_xfer` 和 `uart3m1_xfer` 两组引脚,两个 RX 引脚同时连到控制器输入,空接的那根被上拉电阻钳住,和接了线的打架 → RX 死掉

结论:**应用开发者看到的"外设坏了",很多时候是设备树配置错了**。学会设备树,你就能独立排查这类问题,并且能自己配置外设(串口、LED、按键、传感器),甚至写出第一个自己的驱动。

---

## 1. 设备树是什么(大白话版)

### 1.1 一句话理解

**设备树是一份描述"板子上有什么硬件、接在哪个引脚、用哪个驱动、什么参数"的清单文件**,内核启动时读取它,据此加载驱动、配置引脚。

### 1.2 为什么 ARM 需要它

x86 平台靠 BIOS/ACPI 自动发现硬件;ARM 芯片没有这种统一机制,早期每个板子都在内核里硬编码一份"板级文件",导致内核里塞满了垃圾代码。于是引入设备树:硬件描述从内核代码里剥离出来,变成独立文件。

### 1.3 数据流

```
.dts / .dtsi 源码 ──dtc 编译器──> .dtb 二进制 ──内核启动解析──> /proc/device-tree (运行时可见)
```

- `.dts` :板级设备树源文件(入口)
- `.dtsi` :可被 include 的设备树片段(SoC 级、公共部分)
- `.dtb` :编译产物,烧进 boot 分区
- `/proc/device-tree` :板子跑起来后,当前生效的设备树(调试时看它最准!)

---

## 2. 核心概念(每个概念 5 分钟)

### 2.1 节点与属性

```dts
/dts-v1/;

/ {                                  /* 根节点 */
    my-led {                         /* 一个设备节点 */
        compatible = "my,led";       /* 驱动匹配钥匙(最重要) */
        gpios = <&gpio1 0 0>;        /* 接在哪个 GPIO */
        status = "okay";             /* "okay"=启用 "disabled"=禁用 */
        default-brightness = <128>;  /* 自定义属性 */
    };
};
```

| 属性 | 含义 |
|---|---|
| `compatible` | 字符串,驱动通过它找到设备。格式 `厂商,型号`,如 `"rockchip,rv1106-uart"` |
| `status` | `"okay"` 启用节点;`"disabled"` 节点存在但不生效(SoC 级 dtsi 里所有外设默认 disabled) |
| `reg` | 硬件寄存器地址,如 `reg = <0xff4d0000 0x100>` = 基址 0xff4d0000,长度 0x100 |
| `label:` | 节点标签,`my_led: my-led { ... }` 之后任何地方可用 `&my_led` 引用它 |

### 2.2 树的分层与覆盖(override)

Luckfox SDK 的设备树分四层,各司其职:

```
rv1106.dtsi                      ← SoC 芯片级: 所有外设节点都定义好, 默认 disabled
rv1106-pinctrl.dtsi              ← 引脚复用定义: 每个引脚的每种功能
rv1106g-luckfox-pico-ultra.dts   ← 板级入口: include 上面两个
rv1106-luckfox-pico-ultra-ipc.dtsi ← 板级外设配置: 启用哪些外设
```

**覆盖语法**:同名节点后写的合并、后写的属性覆盖先写的,最后生效:

```dts
/* rv1106.dtsi 里(先) */
uart3: serial@ff4d0000 {
    status = "disabled";
};

/* 板级 dtsi 里(后,生效) */
&uart3 {
    status = "okay";        /* 覆盖: 启用 */
    pinctrl-0 = <&uart3m0_xfer>;   /* 追加/覆盖属性 */
};
```

### 2.3 phandle:设备树里的"指针"

```dts
pinctrl-0 = <&uart3m0_xfer>;
gpios = <&gpio1 0 0>;
```

`&xxx` 是引用另一个节点。编译后变成数字句柄(phandle)。今天调试时你见过:

```
# adb shell 查看板子实时设备树
pinctrl-0 = <0x4f 0x50>     ← 两个 phandle = 挂了两组引脚(就是 bug 所在)
```

### 2.4 pinctrl 引脚复用(今天的重点,必懂)

**一个物理引脚有多个功能**,比如 GPIO1_A1 可以是普通 GPIO、也可以是 UART3 的 RX。选择哪个功能就是"引脚复用(pinmux)"。

pinctrl 节点先定义好每个功能的引脚组合(pin group):

```dts
/* rv1106-pinctrl.dtsi 里 */
uart3 {
    uart3m0_xfer: uart3m0-xfer {          /* UART3 的 M0 引脚组 */
        rockchip,pins =
            /* uart3_rx_m0 */
            <1 RK_PA1 1 &pcfg_pull_up>,   /* GPIO1 组, A1 引脚, 复用功能 1, 上拉 */
            /* uart3_tx_m0 */
            <1 RK_PA0 1 &pcfg_pull_up>;
    };
};
```

四个字段依次是:

1. **哪个 GPIO 组**:`1` = GPIO1(0=GPIO0, 1=GPIO1 ...)
2. **哪个引脚**:`RK_PA1` = A1(芯片手册里的引脚名,宏定义在 `dt-bindings/pinctrl/rockchip.h`)
3. **复用功能编号**:`1` = 该引脚的第 1 种复用功能(查芯片手册:GPIO1_A1 的功能 1 是 UART3_RX_M0)
4. **电气配置**:上拉/下拉/驱动强度等

设备节点通过 `pinctrl-0` 引用引脚组,内核 pinctrl 子系统就会在驱动启动前把引脚切到对应功能。

> ⚠️ **今天的教训**:一个信号**只能挂一个引脚组**!同时挂 m0+m1,两个 RX 引脚同时连到控制器输入线上互相打架。TX 是输出(两个引脚同时输出相同信号)所以没事,RX 是输入所以死了。以后看到 `pinctrl-0 = <&xxx_m0 &xxx_m1>` 这种写法,直接警惕。

---

## 3. 你的 SDK 里设备树文件在哪

```bash
cd /home/wjc/luckfox-pico/sysdrv/source/kernel/arch/arm/boot/dts
ls rv1106*ultra* rv1106.dtsi rv1106-pinctrl.dtsi
```

| 文件 | 作用 |
|---|---|
| `rv1106.dtsi` | RV1106 芯片级描述(所有外设默认 disabled) |
| `rv1106-pinctrl.dtsi` | 全部引脚复用定义 |
| `rv1106g-luckfox-pico-ultra.dts` | 板级入口(include 关系) |
| `rv1106-luckfox-pico-ultra-ipc.dtsi` | 板级外设启用/配置(你改的就是这个) |

编译链路(你已跑过):

```bash
./build.sh          # SDK 根目录, .BoardConfig.mk 决定板型
# 日志里能看到:
# TARGET_KERNEL_DTS = rv1106g-luckfox-pico-ultra.dts
# DTC arch/arm/boot/dts/rv1106g-luckfox-pico-ultra.dtb
# boot.img 打包进 output/image/update.img
```

---

## 4. 动手实验 1:修改设备树属性并验证(30 分钟)

目标:掌握"改树 → 编译 → 验证"闭环,这是设备树开发的日常。

### 步骤 1:反编译当前 dtb 看现状

```bash
dtc -I dtb -O dts \
  /home/wjc/luckfox-pico/sysdrv/source/objs_kernel/arch/arm/boot/dts/rv1106g-luckfox-pico-ultra.dtb \
  > /tmp/current.dts

grep -A 12 "serial@ff4d0000" /tmp/current.dts   # 看 uart3 当前配置
```

> ⚠️ **今天踩过的坑**:内核构建目录里有 `.rv1106g-luckfox-pico-ultra.dtb.dts.tmp` 这样的中间文件,**它是陈旧的,不要信它**。始终反编译最终的 `.dtb` 文件验证。

### 步骤 2:改板级 dtsi

```bash
vim /home/wjc/luckfox-pico/sysdrv/source/kernel/arch/arm/boot/dts/rv1106-luckfox-pico-ultra-ipc.dtsi
```

随便加一个无害改动练习,比如给 uart3 节点加个自定义属性:

```dts
&uart3 {
    status = "okay";
    pinctrl-0 = <&uart3m0_xfer>;
    my-learning-note = "hello dt";   /* 练习: 自定义属性 */
};
```

### 步骤 3:重新编译内核并验证

```bash
cd /home/wjc/luckfox-pico && ./build.sh kernel
dtc -I dtb -O dts sysdrv/source/objs_kernel/arch/arm/boot/dts/rv1106g-luckfox-pico-ultra.dtb | grep -B2 -A8 "serial@ff4d0000"
# 应该能看到 my-learning-note = "hello dt";
```

### 步骤 4:烧录后看运行时设备树

```bash
./flash_update.sh    # 你熟悉的烧录方式

# 烧录后, 板子运行时(adb 连板子):
adb shell "cat /proc/device-tree/serial@ff4d0000/my-learning-note"
# 输出: hello dt
```

> 补充:`/proc/device-tree` 是**当前生效**的设备树。注意 Luckfox 的 `luckfox-config` 会在开机时用动态 overlay 修改运行时设备树(配置在 `/etc/luckfox.cfg`),所以"静态 dtb 里 disabled"的节点,运行时可能是 okay——今天板子上就是这种情况。排查问题时**永远以 /proc/device-tree 为准**。

---

## 5. 动手实验 2:不写代码,用设备树点亮 LED(30 分钟)

内核自带 `leds-gpio` 驱动,你只需要在设备树里配置,就能得到一个可控的 LED。**这是设备树开发的典型形态:80% 的板级工作就是配置现成驱动。**

### 步骤 1:找一个空闲引脚

```bash
cd /home/wjc/luckfox-pico/sysdrv/source/kernel/arch/arm/boot/dts
# 找没有被人占用的 GPIO(比如查 GPIO1_A2 有没有被用)
grep -rn "RK_PA2" rv1106-luckfox-pico-ultra-ipc.dtsi rv1106g-luckfox-pico-ultra.dts
# 没输出 = 没被占用。再对照 wiki 引脚图确认物理位置
```

### 步骤 2:在板级 dtsi 的根节点下加 LED 节点

```dts
/ {
    my-led {
        compatible = "gpio-leds";       /* 匹配内核自带 leds-gpio 驱动 */
        test-led {
            label = "wjc:test";         /* 会出现在 /sys/class/leds/ 下 */
            gpios = <&gpio1 RK_PA2 GPIO_ACTIVE_LOW>;
            default-state = "off";
        };
    };
};
```

### 步骤 3:编译烧录,应用层控制

```bash
./build.sh kernel && ./flash_update.sh   # 编译+烧录

# 板子上:
ls /sys/class/leds/            # 出现 wjc:test
echo 1 > /sys/class/leds/wjc:test/brightness   # 点亮(LED 接好限流电阻)
echo 0 > /sys/class/leds/wjc:test/brightness   # 熄灭
```

**原理总结**:内核驱动 `drivers/leds/leds-gpio.c` 启动时遍历设备树找 `compatible = "gpio-leds"` 的节点,对每个子节点申请 GPIO 并注册成 LED 设备。**你写的是配置,内核驱动负责干活**——这就是设备树的价值。

---

## 6. 动手实验 3:写你的第一个内核驱动(1 小时)

目标:写一个最小驱动,通过设备树匹配并打印信息。理解 `compatible` 匹配机制。

### 6.1 驱动代码 `my_first_drv.c`

```c
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

/* 设备树节点匹配表: compatible 对上号就调用 probe */
static const struct of_device_id my_match[] = {
    { .compatible = "wjc,my-first-drv" },
    { /* 结尾必须留空 */ },
};
MODULE_DEVICE_TABLE(of, my_match);

/* probe: 内核找到设备后调用; remove: 设备移除/驱动卸载时调用 */
static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const char *name = NULL;
    u32 number = 0;

    /* 从设备树读属性 */
    of_property_read_string(np, "my-name", &name);
    of_property_read_u32(np, "my-number", &number);

    pr_info("my_first_drv: probed! name=%s number=%u\n",
            name ? name : "(null)", number);
    return 0;    /* 返回 0 表示成功; 负数会让内核认为 probe 失败 */
}

static int my_remove(struct platform_device *pdev)
{
    pr_info("my_first_drv: removed\n");
    return 0;
}

static struct platform_driver my_drv = {
    .probe  = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my_first_drv",
        .of_match_table = my_match,
    },
};

/* 宏展开后就是 init/exit 注册 platform_driver */
module_platform_driver(my_drv);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("My first device tree driver");
```

### 6.2 设备树节点(加到板级 dtsi 根节点下)

```dts
/ {
    my-first-dev {
        compatible = "wjc,my-first-drv";   /* 必须和驱动匹配表一致! */
        my-name = "hello-dt";
        my-number = <42>;
    };
};
```

### 6.3 编译成 .ko 模块

```makefile
# Makefile
KDIR := /home/wjc/luckfox-pico/sysdrv/source/kernel
CROSS := /home/wjc/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-
ARCH := arm

obj-m := my_first_drv.o

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

```bash
make                                  # 生成 my_first_drv.ko
adb push my_first_drv.ko /tmp/        # 推到板子(或 scp)
```

### 6.4 板子上加载验证

```bash
# 板子上:
insmod /tmp/my_first_drv.ko
dmesg | tail
# 看到: my_first_drv: probed! name=hello-dt number=42   ← 成功!

ls /sys/bus/platform/drivers/my_first_drv/   # 驱动已注册

rmmod my_first_drv
dmesg | tail   # 看到: my_first_drv: removed
```

### 6.5 知识点总结

| 概念 | 说明 |
|---|---|
| `compatible` 匹配 | 内核把设备树节点的 compatible 和每个注册驱动的 of_match_table 比对,对上就调用该驱动的 probe。**设备树和驱动的接头暗号** |
| `platform_driver` | 最简单的一类驱动框架,适合"挂总线上的外设"(UART、LED、GPIO 设备等) |
| probe/remove | 驱动生命周期:设备存在时 probe,移除时 remove |
| `of_property_read_*` | 驱动读设备树属性的标准 API(`of.h` 里有一整套) |
| 编译方式 | 交叉编译 .ko 模块(M= 外部模块),或把驱动放进内核源码树编进内核 |

### 6.6 常见失败排查

- probe 没被调用 → compatible 拼写不一致(设备树 vs 驱动,注意逗号/大小写)
- insmod 报版本错误 → 模块和内核编译配置不一致(重新 make,别混用工具链)
- probe 返回负数 → 内核会打印 probe failed,检查资源申请是否失败

---

## 7. 调试工具箱(应用开发者也能用的手段)

```bash
# 1. 编译/反编译
dtc -I dts -O dtb input.dts -o output.dtb
dtc -I dtb -O dts input.dtb -o output.dts

# 2. 看板子当前生效的设备树(最权威)
adb shell "ls /proc/device-tree/"
adb shell "cat /proc/device-tree/serial@ff4d0000/status"

# 3. 内核日志里的设备树/引脚信息
dmesg | grep -iE "of_|pinctrl|uart3"
adb shell "dmesg | grep -i pinctrl"

# 4. 运行时引脚复用状态(debugfs, 有的系统要先 mount)
adb shell "mount -t debugfs none /sys/kernel/debug"
adb shell "cat /sys/kernel/debug/pinctrl/*/pinmux-pins | head"

# 5. 应用层直接读寄存器(没 debugfs 时的土办法)
#    devmem 读引脚复用寄存器, 配合芯片手册
```

---

## 8. 案例复盘:今天 UART3 故障的完整排查链

把今天的实战整理成方法论,以后照着走:

| 步骤 | 做了什么 | 结论 |
|---|---|---|
| 1. 现象分析 | 绕行上报正常(不依赖串口),电源/冲洗上报失败(都依赖 ttyS3) | 怀疑串口,方向正确 |
| 2. 工具准备 | 写 `uart_rx_monitor.py`(纯接收)、`uart_tx_sender.py`(纯发送) | 先证明"谁不通" |
| 3. 方向测试 | 电脑→板子:板子收不到;板子→电脑:电脑收得到且 CRC 全对 | **TX 通 RX 断**,硬件接线没问题 |
| 4. 排除干扰 | 确认主程序没在跑(不抢串口) | 排除软件抢占 |
| 5. 查设备树 | 板子 `/proc/device-tree` 显示 uart3 挂了两个 pinctrl phandle(0x4f+0x50) | 定位到 DTS 配置 |
| 6. 看驱动机制 | RV1106 pinctrl 驱动无输入路由表,两个 RX pad 同时连到输入线 | 解释"TX 通 RX 断"的机理 |
| 7. 修复 | pinctrl-0 只保留 `uart3m0_xfer`(与接线一致) | 一行修复 |
| 8. 验证 | 编译→反编译 dtb 确认→烧录→双向收发全通 | 闭环验证 |

**方法论**:应用现象 → 数据链路分析 → 二分法定位(方向/软硬)→ 看运行时状态 → 查配置 → 改 → 编译产物验证 → 实测闭环。

---

## 9. 常见坑速查表

| 现象 | 可能原因 | 排查 |
|---|---|---|
| `/dev/ttyS3` 不存在 | 节点 disabled、或改动没烧进去 | `ls /proc/device-tree/` 看节点;反编译 dtb 看 status |
| 能发不能收(今天的坑) | pinctrl 挂了多组引脚 | 反编译 dtb 查 `pinctrl-0` 是否多个 phandle |
| 驱动不 probe | compatible 不匹配 | `dmesg \| grep -i probe`;对比两侧字符串 |
| 改了 dts 不生效 | 没重编译/看的是陈旧中间文件/烧错分区 | 反编译**最终 .dtb** 验证;确认烧录成功 |
| 引脚没反应 | 引脚被别的节点占用或复用号写错 | grep dts 引脚名;对照芯片手册复用表 |
| 开机外设时好时坏 | luckfox-config 动态 overlay 覆盖了静态配置 | `cat /etc/luckfox.cfg`;以 `/proc/device-tree` 为准 |

---

## 10. 学习路线建议

按这个顺序循序渐进,每个阶段都在你的板子上做出实际效果:

1. **概念**(本文 1-2 章):节点/属性/覆盖/pinctrl,看懂一个板级 dtsi
2. **配置现成驱动**(实验 2):LED、按键、看门狗……只改设备树,不写代码,快速建立信心
3. **最小 platform 驱动**(实验 3):compatible 匹配 + probe 打印,理解驱动框架
4. **字符设备驱动**:open/read/write/ioctl,应用层能真正使用你的驱动
5. **中断与定时器**:按键中断、LED 闪烁
6. **真实外设**:串口/I2C/SPI 传感器驱动,读芯片手册写驱动

推荐资料:

- 内核源码自带文档:`Documentation/devicetree/`(语法与规范)
- 内核驱动范例:`drivers/leds/leds-gpio.c`、`drivers/misc/` 下的小驱动
- [Luckfox Wiki UART 页](https://wiki.luckfox.com/zh/Luckfox-Pico-Ultra/UART/)(引脚图以它为准)
- 宋宝华《Linux 设备驱动开发详解》(经典入门书)

**最重要的习惯**:每次改动都走完整闭环——改树 → 编译 → **反编译 dtb 验证** → 烧录 → `/proc/device-tree` 或实测验证。今天你就是这样把一个跨应用层到设备树的故障挖出来的,这套方法对驱动开发同样适用。
