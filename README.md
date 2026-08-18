# Turing Machine Simulator

This simple C program takes in a file as input which builds a Turing machine and simulates it.

## Usage

Compile the C file and run it. Give the machine file as a command-line argument. The string input may may be given as an argument or as an answer to the prompt once launched.

```shell
gcc main.c -o main
./main [machine file] <input>
```
Examples
```shell
./main sample.in
./main sample.in 0000
```
### Output

The output will be in the following format:
```
Input: [input]
([state] [char] ->)+ (q_acc|q_rej)
ACCEPT|REJECT
```
where
- `state` is one of `q_0` or any of the user's defined states
- `char` is the current symbol
- `q_acc|q_rej` is the final state of the program
- `ACCEPT|REJECT` is the verdict of the program


## Machine File

The machine file follows roughly the same format as the formal definition of a Turing machine: `M = (Q, S, G, d, q_0, q_acc, q_rej)` where Q, S, G are all finite sets and
1. `Q` is the set of states
2. `S` is the input alphabet not containing the blank symbol, `NULL`
3. `G` is the tape alphabet where `\0` is in `G` and `S` is a subset of `G`
4. `d: Q x G -> Q x G -> {L, R}` is the transition function
5. `q_0` in `Q` is the start state
6. `q_acc` in `Q` is the accept state
7. `q_rej` in `Q` is the reject state, where `q_rej != q_acc`

However, for simplicity, we combine `G` and `S` in the machine file.

### File Format

The machine file is structured as follows:
- The first line contains `tm` to indicate that it is a valid Turing machine file. Comments may be put here after the `tm` to descibe the machine.
- The second line contains the set of states Q (excluding q_0, q_acc, and q_rej), delimited by `,`
- The third line contains the input alphabet S, delimited by `,`
- The fourth line onwards contains the transition functions formatted as `q,g->q,g,m` where `q` is in `Q`, `g` is in `G`, and `m` is either `L` or `R`. Each transition function is separated by a newline.
- There must be an empty newline at the end

### Examples

The [examples](examples) folder has a few valid machine files.
- [1.in](examples/1.in) has a Turing Machine that recognises strings of the form `0^(2^n)`.
- [2.in](examples/2.in) recognises the same binary string `w` delimited by `#` i.e. `w#w`

### Caveats
- The states `q_0`, `q_acc`, `q_rej` are all already included in the machine by default.
- If the machine gets stuck on a transition i.e. given a state and symbol, there is no next state and symbol, it is implicitly taken to go to the `q_rej` state.
- Blank tape symbols are represented in code as `\0`. In the machine file, they are denoted as skipping the symbol. For example, `q_4,->q_1,,R` means "from state `q_4` reading a blank tape symbol, write a blank tape symbol, move the pointer to the right, and go to state `q_1`/.
- The list of states and alphabet is actually not used currently, allowing for "unregistered" states and symbols to be used in the transitions.
- In the current version, the machine file is still sensitive to whitespaces, so follow the format closely.