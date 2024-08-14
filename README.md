# MinecartSpeedPlus

**此插件为私人定制 未经允许禁止擅自修改发布**  

## 介绍

双击矿车更改矿车速度

### 功能列表

- [x] 配置套餐 自定义套餐
- [x] 支持计分板经济系统
- [x] 可以快速破坏矿车
- [x] 矿车速度非全局生效
- [x] 支持充能铁轨
- [ ] 修复充能铁轨未充能时有机率触发加速

## 安装

- 使用 Lip 安装

    ```bash
    lip install github.com/xianyubb/MinecartSpeedPlus
    ```

- 下载插件包

    [release](https://github.com/xianyubb/MinecartSpeedPlus/releases)

    > 请注意下载对应 LeviLamina 版本的插件版本

## 配置文件

```json
{
    "version": 2, // 勿动
    "ScoreName": "money", // 计分板名称
    "moveMentList": [
        // 初始化为空数组
        {
            "name": "1", // 按钮显示名称
            "speed": 10, // 该套餐的速度
            "money": 0 // 兑换套餐所需积分
        }
    ]
}
```

## Tips

作者: xianyubb
QQ: 2149656630
