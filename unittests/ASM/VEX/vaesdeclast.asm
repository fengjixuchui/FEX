%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM1": ["0xD5D56A6A6AD5D56A", "0x6A6AD5D5D56A6AD5", "0x0000000000000000", "0x0000000000000000"],
    "XMM2": ["0x2A2A9595952A2A95", "0x95952A2A2A95952A", "0x0000000000000000", "0x0000000000000000"],
    "XMM3": ["0xD5D56A6B6AD5D56B", "0x6A6AD5D4D56A6AD4", "0x0000000000000000", "0x0000000000000000"],
    "XMM4": ["0x2A2A95956AD5D56A", "0x6A6AD5D42A95952A", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovaps ymm0, [rdx + 32 * 4]

vaesdeclast xmm1, xmm0, [rdx + 32 * 0]
vaesdeclast xmm2, xmm0, [rdx + 32 * 1]
vaesdeclast xmm3, xmm0, [rdx + 32 * 2]
vaesdeclast xmm4, xmm0, [rdx + 32 * 3]

hlt

align 32
.data:
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000

dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF

dq 0x0000000100000001
dq 0x0000000100000001
dq 0x0000000100000001
dq 0x0000000100000001

dq 0xFFFFFFFF00000000
dq 0x00000001FFFFFFFF
dq 0xFFFFFFFF00000000
dq 0x00000001FFFFFFFF

dq 0x0202020202020202
dq 0x0303030303030303
dq 0x0202020202020202
dq 0x0303030303030303
