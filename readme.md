# Java Compiler

## Open & Run The Project
You need Visual Studio Toolchain for C/C++ for the project.

To open the project, run Visual Studio Developer Command Prompt in project root directory, and type `code .` to bring up Visual Studio Code.

You must run VS Code from Visual Studio Developer Command Prompt, otherwise building will generate errors.

To build & run, simply click run button on top-right.

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

### File Reader
File reader reads a source file into memory in binary format, so the file handle will only stay active during initialization process.

The manipulation of file content will all be directed to the memory in order to speed up control of high precision during parsing.

### Reserved Words
This is the example where hash table is used.

Reserved words are stored in a global array `java_reserved_words`, each element describes a reserved word.
This array is initialized on stack so no need to free it, and it is being used through pointer references.

A symbol table is a hash table for reserved words, each slot in hash table is a `java_symbol`, which is a hash wrapper of `java_reserved_words`.

The hash table is contructed in run-time, and collision is handled by linear probing.

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
