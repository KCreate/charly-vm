Disassembly of block at 0x26fb000
             0x0000026fb260: nop 
         *-- 0x0000026fb261: putfunction @"__CHARLY_MODULE_FUNC", 0x18, true, 1, 3
         |   0x0000026fb277: halt 
         *-> 0x0000026fb278: nop 
             0x0000026fb279: putvalue 0x7ffc000000000000
             0x0000026fb282: setlocal 2, 0
*----------> 0x0000026fb28b: putvalue 0x7ffa000000000000
|  *-------- 0x0000026fb294: branchunless 0x000091
|  |         0x0000026fb299: nop 
|  |         0x0000026fb29a: readlocal 2, 0
|  |         0x0000026fb2a3: putvalue 0x7ffc000000000064
|  |         0x0000026fb2ac: gt 
|  |  *----- 0x0000026fb2ad: branchunless 0x000058
|  |  |      0x0000026fb2b2: nop 
|  |  |  *-- 0x0000026fb2b3: branch 0x000091
|  |  *--+-> 0x0000026fb2b8: readlocal 13, 1
|  |     |   0x0000026fb2c1: readlocal 2, 0
|  |     |   0x0000026fb2ca: call 1
|  |     |   0x0000026fb2cf: pop 
|  |     |   0x0000026fb2d0: readlocal 2, 0
|  |     |   0x0000026fb2d9: putvalue 0x7ffc000000000001
|  |     |   0x0000026fb2e2: add 
|  |     |   0x0000026fb2e3: setlocal 2, 0
*--+-----+-- 0x0000026fb2ec: branch 0x00002b
   *-----*-> 0x0000026fb2f1: readlocal 1, 0
             0x0000026fb2fa: return 
