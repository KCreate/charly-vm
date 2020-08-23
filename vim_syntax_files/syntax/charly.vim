" Vim syntax file
" Language:	Charly
" Maintainer:	Leonard Schütz <leni.schuetz@me.com>
" Original File URL:		http://www.fleiner.com/vim/syntax/javascript.vim

if !exists("main_syntax")
  " quit when a syntax file was already loaded
  if exists("b:current_syntax")
    finish
  endif
  let main_syntax = 'charly'
elseif exists("b:current_syntax") && b:current_syntax == "charly"
  finish
endif

let s:cpo_save = &cpo
set cpo&vim


syn keyword charlyiptCommentTodo      TODO FIXME XXX TBD contained
syn match   charlyLineComment         "\/\/.*" contains=@Spell,charlyCommentTodo
syn match   charlyCommentSkip         "^[ \t]*\*\($\|[ \t]\+\)"
syn region  charlyComment	            start="/\*"  end="\*/" contains=@Spell,charlyCommentTodo
syn match   charlySpecial	            "\\\d\d\d\|\\."
syn region  charlyStringD	            start=+"+  skip=+\\\\\|\\"+  end=+"\|$+	contains=charlySpecial,@htmlPreproc
syn region  charlyStringS	            start=+'+  skip=+\\\\\|\\'+  end=+'\|$+	contains=charlySpecial,@htmlPreproc

syn match   charlySpecialCharacter    "'\\.'"
syn match   charlyNumber	            "-\=\<\d\+L\=\>\|0[xX][0-9a-fA-F]\+\>"
syn region  charlyRegexpString        start=+/[^/*]+me=e-1 skip=+\\\\\|\\/+ end=+/[gim]\{0,2\}\s*$+ end=+/[gim]\{0,2\}\s*[;.,)\]}]+me=e-1 contains=@htmlPreproc oneline

syn keyword charlyConditional	        if else unless guard switch match
syn keyword charlyRepeat		          while do loop
syn keyword charlyBranch		          break continue
syn keyword charlyType		            Array Boolean Class Function Generator Number Object String Null Value
syn keyword charlyStatement		        return yield
syn keyword charlyBoolean	      	    true false
syn keyword charlyNull		            null NaN
syn keyword charlyIdentifier      	  arguments let const self super
syn keyword charlyLabel		            case default
syn keyword charlyException		        try catch finally throw
syn keyword charlyGlobal	            Charly
syn keyword charlyReserved		        class const export extends import static ignoreconst property from as new typeof use

syn keyword charlyFunction	          func
syn match	charlyBraces	              "[{}\[\]]"
syn match	charlyParens	              "[()]"

syn sync fromstart
syn sync maxlines=100

if main_syntax == "charly"
  syn sync ccomment charlyComment
endif

" Define the default highlighting.
" Only when an item doesn't have highlighting yet
hi def link charlyComment		          Comment
hi def link charlyLineComment		      Comment
hi def link charlyCommentTodo		      Todo
hi def link charlySpecial		          Special
hi def link charlyStringS		          String
hi def link charlyStringD		          String
hi def link charlyCharacter		        Character
hi def link charlySpecialCharacter	  charlySpecial
hi def link charlyNumber		          Number
hi def link charlyConditional		      Conditional
hi def link charlyRepeat	      	    Repeat
hi def link charlyBranch		          Conditional
hi def link charlyType			          Type
hi def link charlyStatement		        Statement
hi def link charlyFunction		        Function
hi def link charlyBraces		          Function
hi def link charlyError	  	          Error
hi def link charlyParenError		      charlyError
hi def link charlyNull			          Keyword
hi def link charlyBoolean		          Boolean
hi def link charlyRegexpString		    String

hi def link charlyIdentifier		      Identifier
hi def link charlyLabel		            Label
hi def link charlyException		        Exception
"hi def link charlyMessage		         Keyword
hi def link charlyGlobal		          Keyword
hi def link charlyMember		          Keyword
hi def link charlyReserved		        Keyword
hi def link charlyDebug		            Debug
hi def link charlyConstant		        Label


let b:current_syntax = "charly"
if main_syntax == 'charly'
  unlet main_syntax
endif
let &cpo = s:cpo_save
unlet s:cpo_save

" vim: ts=8
