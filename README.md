README for Inline Lua, based on Lua 5.1

See INSTALL for installation instructions.
See HISTORY for a summary of changes since the last released version.
See the test directory for some sample programs.

* What is Inline Lua ("inlua")?
  ------------
  Inline Lua ("inlua") inherits its functionality from Lua 5.1, with some big changes to how the language looks and behaves.
  Inlua was inspired by the ternary expression in lua, `condition and true_result or false_result`, and how it could replace if-else in most scenarios.
  
  ### Syntax Changes
  | Lua              | Inlua            |
  | ---------------: | :--------------- |
  |`nil`|`~`|
  |`~=`|`!=`|
  |`and`|`&`|
  |`or`|`\|`|
  |`not`|`!`|
  |`local`|`@`|
  |`t[<expression>]`|`t.(<expression>)`|
  |`{ key=value }`|`{ .key=value }`|
  |`{ [<expression>]=value }`|`{ .(<expression>)=value }`|
  |`return <explist>`|`^^ <explist>`|
  |`break`|`^^^`|
  |(is illegal)|`^^^ <expression>`<br>Break from a loop with a value.|
  |`do <block> end`|`(<block>)`|
  |`while <cond> do <body> end`|`? <cond> -> (<body>)`|
  |`for var=start,stop,<step> do <body> end`|`?? var=start,stop,<step> -> (body)`|
  |`for <varlist> in <explist> do <body> end`|`?? [<varlist>] <explist> -> (body)`|
  |`function(<args>) <body> end`|`[<args>](<body>)`|
  |`function name(<args>) <body> end`|`name = [<args>](<body>)`|
  |`local function name(<args>) <body> end`|`@name = [<args>](<body>)`|
  |`func "string literal"` or<br> `func {<table literal>}`|This is ambiguous in inlua,<br>so functions can't be called this way.|
  |(is illegal)|numbers can contain underscores for visual clarity:<br>`100_000_000`|
  |(is illegal)|return and break statements can be used in place of expressions:<br>`var = errcond & ^^~,"error"; \| okval`|
  |`true`|`()` block that returns no value is true|
  |`false`|`!()` or `~`, nil can be used as false in most cases, but is not a boolean|
  |`true`|`?1->(^^^), ??i=1,10->()` loop with no break value is true|

  ### Semantics
  In Inlua, every expression is a statement and blocks can run code *and* return values.
  These two semantic changes allow ternary expressions to fully replace if-else statements:
  ```
  condition & (true code) | (false code)
  ```
  Blocks and loops evaluate to true by default, allowing the ternary expression to work like if-else.
  However, if a block's last expression is not followed by ';', the block returns the value of that expression:
  ```
  condition & (result_a) | (result_b) -- warning: if result_a is false, result_b result will be returned instead
  ```
  Expression lists can also be returned from blocks:
  ```
  a,b,c=(1,2,3) --> a=1, b=2, c=3
  a,b,c=1,(2,3) --> a=1, b=2, c=3
  a,b,c=(1,2),3 --> a=1, b=3, c=nil
  ```
  Note how expression lists follow the same rules as other multiple-return expressions like a function call or varargs (`...`):
  If it is last in an expression list, it returns multiple values, otherwise it returns only one value.

  Loops evaluate to true by default, like loops. When breaking from a loop, the break statement can supply a value to return.
  Returning a non-true value from a loop can be used to detect when break was used in the loop.
  ```
  a = ? ~ -> () --> a=true
  b = ? 1 -> (^^^) --> b=true
  c = ? 1 -> (^^^7) --> c=7
  d = ?? [k,v] pairs(t) -> (^^^v) --> d = v or, if t is empty, true
  e = ?? i=1,10 -> (print(i)) --> e = nil
  ```

  ### Additions to Standard Library

  `io.subprocess(command)`
  * Creates a new subprocess using the given `command` string.
  * Returns the stdout, stdin, and stderr of the new subprocess, in that order.

* What is Lua?
  ------------
  Lua is a powerful, light-weight programming language designed for extending
  applications. Lua is also frequently used as a general-purpose, stand-alone
  language. Lua is free software.

  For complete information, visit Lua's web site at http://www.lua.org/ .
  For an executive summary, see http://www.lua.org/about.html .

  Lua has been used in many different projects around the world.
  For a short list, see http://www.lua.org/uses.html .

* Availability
  ------------
  Lua is freely available for both academic and commercial purposes.
  See COPYRIGHT and http://www.lua.org/license.html for details.
  Lua can be downloaded at http://www.lua.org/download.html .

* Installation
  ------------
  Lua is implemented in pure ANSI C, and compiles unmodified in all known
  platforms that have an ANSI C compiler. In most Unix-like platforms, simply
  do "make" with a suitable target. See INSTALL for detailed instructions.

* Origin
  ------
  Lua is developed at Lua.org, a laboratory of the Department of Computer
  Science of PUC-Rio (the Pontifical Catholic University of Rio de Janeiro
  in Brazil).
  For more information about the authors, see http://www.lua.org/authors.html .

(end of README)
