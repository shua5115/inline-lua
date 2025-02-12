README for Inline Lua, based on Lua 5.1

See INSTALL for installation instructions.
See HISTORY for a summary of changes since the last released version.
See the test directory for some sample programs.

* What is Inline Lua ("inlua")?
  ------------
  Inline Lua ("inlua") inherits its functionality from Lua 5.1, with some big changes to how the language looks and behaves.
  Inlua was inspired by the ternary expression in lua, `condition and true_result or false_result`, specifically how it could replace an if-else in certain scenarios.
  
  ### Syntax
  Inlua changes the syntax of lua significantly to differentiate itself.
  These are the changes to lua's syntax:
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
  |`{ [<expression>]=value }`|`{ .(<expression)=value }`|
  |`return <explist>`|`^^ <explist>`|
  |`break`|`^^^ <expression>`|
  |`do <block> end`|`(<block>)`|
  |`while <cond> do <body> end`|`? <cond> -> (<body>)`|
  |`function(<args>) <body> end`|`[<args>](<body>)`|
  |`func "string literal"` or<br> `func {<table literal>}`|This is ambiguous in inlua,<br>so functions can't be called this way.|
  |`function name(<args>) <body> end`|`name = [<args>](<body>)`|
  |`local function name(<args>) <body> end`|`@name = [<args>](<body>)`|
  |(is illegal)|numbers can contain underscores for visual clarity:<br>`100_000_000`|
  |(is illegal)|return and break statements can be used in place of expressions:<br>`var = errcond & ^^~,"error"; \| okval`|
  |`true`|`()` block that returns no value is true|
  |`false`|`!()` or `~`, nil can be used as false in most cases, but is not a boolean|
  |`for <varlist> in <explist> do <body> end`|For loops are not in the language|

  ### Semantics
  Inlua's makes any expression a statement, and allows blocks to run code and return values.
  These two changes allow ternary expressions to fully replace if-else statements:
  ```
  condition & (true code;) | (false code;)
  ```
  Blocks evaluate to true by default, allowing the ternary expression to work as an if-else consistently.
  However, if a block's last expression is not followed by ';', the block returns the value of that expression:
  ```
  condition & (true_result) | (false_result) -- warning: if true result is false, false result will be returned
  three=(1+2) -- this still works as expected
  ```
  Expression lists can also be returned from blocks:
  ```
  a,b,c=(1,2,3) --> a=1, b=2, c=3
  a,b,c=1,(2,3) --> a=1, b=2, c=3
  a,b,c=(1,2),3 --> a=1, b=3, c=nil
  ```
  Note how expression lists follow the same rules as other multiple-return expressions like a function call or varargs (`...`):
  If it is last in an expression list, it returns multiple values, otherwise it returns only one value.

  In addition to making blocks expressions, while loops are also expressions.
  While loops evaluate to nil by default. When breaking from a while loop, the break statement can supply a value to return.
  ```
  a = ? ~ -> () --> a=nil
  b = ? 1 -> (^^^) --> b=nil
  c = ? 1 -> (^^^7) --> c=7
  ```

  ### Iterators
  Since for loops are not in the language, an iterator library is implemented to replace that functionality:
  ```
  t = {"a", "b", "c"}
  iter(pairs(t)):foreach([k,v](
    print(k, v)
  ))
  ```
  Iterators have a Rust-style api, and are lazily evaluated.
  Each iterator is represented as a table with a `next` function.
  The `next` function is not passed any parameters when called, the iterator's state must be fully contained in upvalues.
  An iterator is complete when `next` returns a nil value as its first return value. After this, it is undefined what the iterator might do.
  To avoid unecessary table allocation, calling methods on an iterator will replace the table's `next` function, but not the table (unless otherwise stated).
  | Method           | Behavior         |
  | ---------------- | ---------------- |
  **Consumers** use values from an iterator, potentially returning a result.
  |`it:consume()`| Consumes the iterator's values until produces no more results. Returns nothing. |
  |`it:foreach(f)`| Calls `f(it.next())` until there are no more results. Returns nothing. |
  |`it:count()`| Returns the number of times the iterator produced a result. |
  |`it:last()`| Returns the last result list produced by the iterator. |
  |`it:nth(n)`| Returns the nth result list produced by the iterator. |
  |`it:collect(t)`| Collects the results of the iterator into a table, returning it when finished.<br>If the table is not provided, a table will be created.<br>An iterator returning single values will store its values in the table's array, starting from 1.<br>An iterator returning two values will store each result as key-value pairs. |
  |`it:all(p)`| Returns if all elements in an iterator satisfy predicate `p`. |
  |`it:any(p)`| Returns if any elements in an iterator satisfy predicate `p`. |
  |`it:find(p)`| Returns the first result list that satisfies predicate `p`. |
  |`it:position(p)`| Returns the index of the first result list that satisfies predicate `p`. |
  |`it:min(lt)`| Returns the minimum value of the iterator. Only considers the first result in a result list.<br>Can provide an optional less-than function for comparing elements.<br>If the next element is equal to the optimal, the next element becomes the optimal.|
  |`it:max(lt)`| Returns the maximum value of the iterator. Only considers the first result in a result list.<br>Can provide an optional less-than function for comparing elements.<br>If the next element is equal to the optimal, the next element becomes the optimal.|
  |`it:compare(it2, comp)`| Lexicographically compares two iterators.<br>An optional comparison function can be provided to dictate how to compare iterator elements.<br>Both this and the comparison function returns 0 when equal, -1 when `it<it2` and 1 when `it>it2`.|
  |`it:fold(acc, f)`| Calls `f(acc, it.next())` until the iterator produces no more results. |
  |`it:reduce(f)`| Stores the first result into an accumulator, `@acc=it.next()`, then calls `it:fold(acc, f)`. |
  |`it:unzip(t1, t2)`| Like collect, but returns two tables.<br>For the ith result: `t1.(i), t2.(i) = it.next()`|
  **Adapters** modify the iterator's `next` function.
  |`it:map(f)`| Calls f on each element in `it`. |
  |`it:chain(it2)`| Produces all of `it`'s values, then all of `it2`'s values. |
  |`it:zip(it2)`| The next function returns values from both it.next and it2.next as pairs until one runs out of values. |
  |`it:filter(p)`| Skips all elements for which predicate `p` is false. |
  |`it:enumerate()`| Returns the count of the current iteration alongside the results of `it.next`. |
  |`it:skip(n)`| Skips the first n elements from the iterator, then continues. |
  |`it:take(n)`| Truncates the iterator to only return n elements before stopping. |
  |`it:inspect(f)`| Calls f on the results of `it.next` on every iteration before returning the results. |

  The following functions are also added as globals to support the use of iterators:

  `iter(f)` Creates a new iterator table with function f as the "next" function.

  `iter(f, state, key)` Creates a new iterator wrapping the stateless iterator function f in a function that manages its state.

  `iter(t)` Expects the table t to have a "next" function. Sets the table's metatable to the iterator metatable.

  `range(start, stop, step)` Returns numbers from start to end, inclusive, stepping by step. Steps after every call. Works like numeric for loop arguments.

  `values(t)` Returns only the values from the table. Works exactly like `pairs`, but without returning the key.

  `ivalues(t)` Returns only the values from the array portion of a table. Works exactly like `ipairs`, but without returning the index.

  

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
