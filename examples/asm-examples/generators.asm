Disassembly of block at 0x175c310
    0x000001761c10: nop
*-- 0x000001761c11: putfunction @"__CHARLY_MODULE_FUNC", 0x18, true, 1, 4
|   0x000001761c27: halt
*-> 0x000001761c28: nop
*-- 0x000001761c29: putfunction @"create_generator", 0xcd, false, 0, 2
|   0x000001761c3f: setlocal 2, 0
|   0x000001761c48: readlocal 2, 0
|   0x000001761c51: call 0
|   0x000001761c56: setlocal 3, 0
|   0x000001761c5f: readlocal 13, 1
|   0x000001761c68: readlocal 3, 0
|   0x000001761c71: call 0
|   0x000001761c76: call 1
|   0x000001761c7b: pop
|   0x000001761c7c: readlocal 13, 1
|   0x000001761c85: readlocal 3, 0
|   0x000001761c8e: call 0
|   0x000001761c93: call 1
|   0x000001761c98: pop
|   0x000001761c99: readlocal 13, 1
|   0x000001761ca2: readlocal 3, 0
|   0x000001761cab: call 0
|   0x000001761cb0: call 1
|   0x000001761cb5: pop
|   0x000001761cb6: readlocal 13, 1
|   0x000001761cbf: readlocal 3, 0
|   0x000001761cc8: call 0
|   0x000001761ccd: call 1
|   0x000001761cd2: pop
|   0x000001761cd3: readlocal 1, 0
|   0x000001761cdc: return
*-> 0x000001761cdd: nop
*-- 0x000001761cde: putgenerator @"create_generator", 0xdc
|   0x000001761ceb: return
*-> 0x000001761cec: nop
    0x000001761ced: putvalue 0x7ffc000000000000
    0x000001761cf6: setlocal 1, 0
*-> 0x000001761cff: nop
|   0x000001761d00: readlocal 1, 0
|   0x000001761d09: yield
|   0x000001761d0a: pop
|   0x000001761d0b: readlocal 1, 0
|   0x000001761d14: putvalue 0x7ffc000000000001
|   0x000001761d1d: add
|   0x000001761d1e: setlocal 1, 0
*-- 0x000001761d27: branch 0x0000ef
    0x000001761d2c: putvalue 0x7ffb000000000000
    0x000001761d35: return
