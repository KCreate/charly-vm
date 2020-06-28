Disassembly of block at 0x129cb60
          0x00000129cb80: nop
      *-- 0x00000129cb81: putfunction @"main", 0x18, true, 1, 3
      |   0x00000129cb97: halt
      *-> 0x00000129cb98: nop
          0x00000129cb99: putvalue 0x7ffc000000000019
          0x00000129cba2: setlocal 2, 0
          0x00000129cbab: readlocal 2, 0
          0x00000129cbb4: putvalue 0x7ffc000000000014
          0x00000129cbbd: gt
      *-- 0x00000129cbbe: branchunless 0x000061
      |   0x00000129cbc3: nop
      |   0x00000129cbc4: readlocal 13, 1
      |   0x00000129cbcd: putstring "num is bigger than 20"
      |   0x00000129cbd6: call 1
      |   0x00000129cbdb: pop
*-----+-- 0x00000129cbdc: branch 0x0000b1
|     *-> 0x00000129cbe1: nop
|         0x00000129cbe2: readlocal 2, 0
|         0x00000129cbeb: putvalue 0x7ffc00000000000a
|         0x00000129cbf4: gt
|  *----- 0x00000129cbf5: branchunless 0x000098
|  |      0x00000129cbfa: nop
|  |      0x00000129cbfb: readlocal 13, 1
|  |      0x00000129cc04: putstring "num is bigger than 10"
|  |      0x00000129cc0d: call 1
|  |      0x00000129cc12: pop
|  |  *-- 0x00000129cc13: branch 0x0000b1
|  *--+-> 0x00000129cc18: nop
|     |   0x00000129cc19: readlocal 13, 1
|     |   0x00000129cc22: putstring "num is smaller than 10"
|     |   0x00000129cc2b: call 1
|     |   0x00000129cc30: pop
*-----*-> 0x00000129cc31: readlocal 1, 0
          0x00000129cc3a: return
