README for Inline Lua, based on Lua 5.1

See INSTALL for installation instructions.
See HISTORY for a summary of changes since the last released version.

* What is Inline Lua ("inlua")?
  ------------
  Inline Lua inherits its functionality from Lua 5.1, with some big changes to how the language looks and behaves.
  Inline Lua was inspired by the ternary expression in lua, `condition and true_result or false_result`, specifically how it could replace an if-else in certain scenarios.
  However, in Lua, ternary expressions are limited: the true and false branches must be expressions, so statements like assignments and loops could not be executed. Also, there is an edge case where if the `true_result` evaluated to false, the `false_result` would be evaluated instead.
  To resolve these issues, and to make this language distinct from Lua, the following changes were made:

  * There are no reserved words. All language syntax is symbolic.
  * All expressions are statements. No exceptions.
  * Logic operators are `&,|,!` for "and, or, not", respectively.
  * Not-equals operator is `!=`.
  * Local variables are declared with prefix `@`: `@a, b = 1, 2`
  * Nil is the `~` character
  * Return is `^^`, and can be followed by a list of expressions.
  * Break is `^^^`, and can be followed by a single expression.
  * Function literals are `[arglist](body)`, and are the only way to define functions.
  * Tables are indexed with `.field` or `.(expression)`. The latter replaces square bracket indexing.
  * While loops are `? condition -> (body)`, and are expressions.
    * While loops evaluate to nil by default.
    * If break is used in the loop, a single value can be placed after the break, and the loop will evaluate to that value: `?1->(^^^7) -- loop evaluates to 7`
  * Blocks are `(code)`, and are expressions.
    * Sub-expressions (`(expression)`) no longer exist, with block expressions as the replacement.
    * The last statement in a block, if not followed by a semicolon, will have its value returned, e.g. `3*(print(); 2+1)` returns 9 while `3*(print(); 2+1;)` does not. This allows blocks to take the place of sub-expressions seamlessly while retaining their ability to execute multiple lines of code.
    * An empty block `()`, or any block which returns no values, evaluates to true. This ensures ternary expressions work as expected with blocks: `condition & (true_code;) | (false_code;)`.
    * Blocks can return multiple values when used as the last argument in a function call or return statement: `io.write(1,2,(3,4))` prints "1234".
    * In a block, if the last expression is varargs (`...`) or a function call, that expression will only return one value. The number of return values for a block must be known at compile time.
    * A block in the middle of an expression list will only return one value. This matches the behavior of other multi-return expressions like function calls or varargs.
  * For and repeat-until loops no longer exist. A library of iterator functions is planned as a replacement.
  * Table key assignment with an expression is `{.(expression)=expression}`, instead of `{[expression]=expression}` to distinguish from function definition, and to be consistent with table indexing syntax.
  * Table field assignment is `{.field=value}` to be consistent with the above change, and to further distinguish inlua tables from lua tables.
  * Return and break statements can be used where an expression is expected.
    * This is so ternaries can break or return without needing to enter a block: `@a = v | ^~, "error, missing value"`
  * Lists of expressions on their own, like `f(),g(),h()`, are still expressions. Expression lists are evaluated left-to-right.
  * Functions can only be called with parenthesis, so this syntax is now illegal: `print "arg"` or `print {table}`
    * There are two reasons for this. The first is that all expressions are statements, so there is ambiguity between this as a function call and this as two consecutive statements. The second is that having only one way to call a function increases the consistency and readability of code.
  
  
  Inline Lua uses the same bytecode representation as Lua 5.1, so any bytecode generated with Inline Lua can be loaded into a standard Lua 5.1 VM.

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
