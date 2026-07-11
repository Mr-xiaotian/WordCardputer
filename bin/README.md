# 固件文件说明

推荐直接使用：

- `WordCardputer_cardputer_merged.bin`

这是完整合并固件，刷写地址为：

```text
0x0000
```

如果刷机工具不支持 merged 固件，也可以使用分离文件：

- `WordCardputer_cardputer_bootloader.bin` -> `0x0000`
- `WordCardputer_cardputer_partitions.bin` -> `0x8000`
- `WordCardputer_cardputer_app.bin` -> `0x10000`

目标板配置：

- 板卡：`m5stack:esp32:m5stack_cardputer`
- Flash：`8M`
- 分区：`default_8MB`
