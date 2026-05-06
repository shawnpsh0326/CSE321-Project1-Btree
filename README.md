# CSE321 Project 1: B-tree Index Structures

This project implements and evaluates three tree-based index structures:
B-tree, B*-tree, and B+tree. The programs use the given `student.csv` dataset
with 100,000 student records.

## Files

- `b_tree.cpp`: B-tree implementation
- `b_star_tree.cpp`: B*-tree implementation
- `b_plus_tree.cpp`: B+tree implementation
- `student.csv`: input dataset

## Dataset

Each student record is loaded into an in-memory array.
The Student ID is used as the key, and the array index is used as the RID
(Record Identifier).

## Environment

- Language: C++
- Compiler: g++
- External libraries: none

## Compile

Run the following commands in the project directory.

```bash
g++ b_tree.cpp -o b_tree
g++ b_star_tree.cpp -o b_star_tree
g++ b_plus_tree.cpp -o b_plus_tree
```

## Run

Run each executable separately.

```bash
./b_tree
./b_star_tree
./b_plus_tree
```

On Windows PowerShell, the executables can be run as:

```powershell
.\b_tree.exe
.\b_star_tree.exe
.\b_plus_tree.exe
```

After running a program, enter the tree order `d`.

Example:

```text
Input order(one integer): 5
```

The order must be at least 3. To stop the program after one experiment, enter
`n` when the program asks whether to continue.

## Experiments

Each program performs the following workloads:

1. Insert all 100,000 records into an empty tree.
2. Search 10,000 randomly selected Student IDs.
3. Execute a range query on Student IDs from `202000000` to `202100000`.
4. Delete 2,000 randomly selected records.

The program prints:

- insertion time
- node utilization
- number of splits
- point search time
- range query result and time
- deletion time

For the report, the experiments were repeated with different tree orders:
`d = 3, 5, 10, 20, 50`.
