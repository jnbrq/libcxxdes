# `run_tests.md`

[README](../README.md) | [Documentation index: Project And Development](../docs/index.md#project-and-development)

Run the tests for all configurations with the following command:

```bash
bash ./run_tests.sh test_configs/*.cfg
```

Run tests using only the GNU compiler:

```bash
bash ./run_tests.sh test_configs/gnu*.cfg
```

Run tests using only the Clang compiler:

```bash
bash ./run_tests.sh test_configs/clang*.cfg
```
