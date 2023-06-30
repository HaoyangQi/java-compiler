# Java Compiler

## Branch Naming Convention
```
<class>/<stage>/<description>
```
### class
* dev: feature
* bugfx: bug fix

### stage
* system: architecture design, system-wise support
* syntax: syntax parsing: token, parsing
* context: semantics parsing
* ir: intermediate representation

### description
describe purpose of the feature briefly

## Architecture

### Token

### Lexeme & Hash Table
Certain lexemes have definite form so they can be hashed.
Hash table is a list of lexeme data, and all data from the hash table is a reference.
That is: all lexemes are single and have no copy, hence no deletion.

### Error Determination & Recovery
* Error recovery is implicitly supported by hand-written syntax parser.
* Errors are determined in semantics parsing phase.
As a important feature in top-down parser, backtrace is firstly supported by:
* for every parsing function, it return NULL and reverted accepted tokens as needed
