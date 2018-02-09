Disassembly of block at 0x976ec0
             0x000000979400: nop 
         *-- 0x000000979401: putfunction @"__CHARLY_MODULE_FUNC", 0x18, true, 1, 6
         |   0x000000979417: halt 
         *-> 0x000000979418: nop 
*----------- 0x000000979419: putfunction @"throw_some_message", 0x11f, false, 1, 2
|            0x00000097942f: setlocal 2, 0
|     *----- 0x000000979438: registercatchtable 0x00005c
|     |      0x00000097943d: nop 
|     |      0x00000097943e: readlocal 2, 0
|     |      0x000000979447: putstring "hello world"
|     |      0x000000979450: call 1
|     |      0x000000979455: pop 
|     |      0x000000979456: popcatchtable 
|     |  *-- 0x000000979457: branch 0x00007e
|     *--+-> 0x00000097945c: setlocal 3, 0
|        |   0x000000979465: nop 
|        |   0x000000979466: readlocal 13, 1
|        |   0x00000097946f: readlocal 3, 0
|        |   0x000000979478: call 1
|        |   0x00000097947d: pop 
|  *-----*-> 0x00000097947e: registercatchtable 0x0000f3
|  |         0x000000979483: nop 
|  |  *----- 0x000000979484: registercatchtable 0x0000a8
|  |  |      0x000000979489: nop 
|  |  |      0x00000097948a: readlocal 2, 0
|  |  |      0x000000979493: putstring "this is an error"
|  |  |      0x00000097949c: call 1
|  |  |      0x0000009794a1: pop 
|  |  |      0x0000009794a2: popcatchtable 
|  |  |  *-- 0x0000009794a3: branch 0x0000d4
|  |  *--+-> 0x0000009794a8: setlocal 4, 0
|  |     |   0x0000009794b1: nop 
|  |     |   0x0000009794b2: readlocal 13, 1
|  |     |   0x0000009794bb: putstring "this is printed every time"
|  |     |   0x0000009794c4: call 1
|  |     |   0x0000009794c9: pop 
|  |     |   0x0000009794ca: readlocal 4, 0
|  |     |   0x0000009794d3: throw 
|  |     *-> 0x0000009794d4: nop 
|  |         0x0000009794d5: readlocal 13, 1
|  |         0x0000009794de: putstring "this is printed every time"
|  |         0x0000009794e7: call 1
|  |         0x0000009794ec: pop 
|  |         0x0000009794ed: popcatchtable 
|  |     *-- 0x0000009794ee: branch 0x000115
|  *-----+-> 0x0000009794f3: setlocal 5, 0
|        |   0x0000009794fc: nop 
|        |   0x0000009794fd: readlocal 13, 1
|        |   0x000000979506: readlocal 5, 0
|        |   0x00000097950f: call 1
|        |   0x000000979514: pop 
|        *-> 0x000000979515: readlocal 1, 0
|            0x00000097951e: return 
*----------> 0x00000097951f: nop 
             0x000000979520: readlocal 1, 0
             0x000000979529: throw 
