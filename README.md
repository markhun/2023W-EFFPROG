# 2023W-EFFPROG - magichex


## Building the program
```
make
```

The executable will be located in `./bin/magichex`

## Testing the program against reference output

```
make test
```

Will execute `magichex` three times with some predefined parameters and compare its
outputs with some reference output. 

The test cases can also be executed one at a time by calling

```
make test1
```
```
make test2
```
```
make test3
```

## Profiling

There are two pre-defined targets to run `perf` on `magichex`

```
make profile
```

```
make profile-with-test
```