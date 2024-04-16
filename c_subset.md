# Program Format

Programs are laid out in the following form:

```abnf
<program>        = (<variable-decl>)* (<procedure-decl>)*
<variable-decl>  = <type> <ident> ";"
<procedure-decl> = <type> <ident> " ()" " {" (<statement>)* "}"
<ident>          = <ident-start>(<ident-cont>)*
<type>           = "int" | "int*"
<statement>      = <assign-expr>" ;"
                 | <ident> " ();" ; <- function call
                 | "if(" <expr> "){" (<statement>)* "}"
                 | "while(" <expr> "){" (<statement>)* "}" ; TODO
<assign-expr>    = <ident> " = " <expr>
<expr>           = <unary> (<binop> <unary>)?
<unary>          = "* " <ident>
                 | "& " <ident>
                 | <ident>
                 | <integer>
<binop>          = "+" | "-" | "&" | "|" | "^" | "<<" | ">>" | "==" | "!=" | "<" | "<="
```

### notable "features":

- all declarations MUST be globals, scopes are a convenience
- declarations MUST come before uses
- function names MUST be followed by a space
- functions cannot return a value, even though they have a return type
- `main` MUST NOT return, it MUST enter an infinite loop
- the `()` in a function declaration MUST be preceeded and followed by a space
- only types `int` and `int*` exist (and note that they are 16 bits each)
  - however, types are not checked, the target of a deref expression is cast to a `int*` and dereferenced
- only `<` and `<=` exist. if you want a greater than comparison, swap the argument order
