# webserv - CGI capable webserver in C++98

## Supported options

- `-c file`: Use an alterantive configuration file.
- `-l LEVEL`: Specify loglevel. Supported loglevels are `FATAL`/`1`, `ERR`/`ERROR`/`2`, `WARN`/`WARNING`/`3`, `INFO`/`4`, `DEBUG`/`5`, `TRACE`/`6`, `TRACE2`/`7`, etc. Default is `INFO`.
- `-t`: Do not run, just test the configuration file and print confirmation to standard error.
- `-T`: Same as `-t`, but additionally dump the configuration file to standard output.
- `-v`: Print version information.
- `-h`: Print help information.

## Building and running

```sh
make # or make re for clean build
./webserv
```

## Tests (TODO)

```sh
make unit_tests # or reunit_tests for clean build
# And for coverage reports
make llvmcov # or rellvmcov for clean build
make lcov # or relcov for clean build
make gcovr # or regcovr for clean build
```

## Config

Almost a proper subset of the NGINX `http` context. Many directives missing of
course.  For a list of full directives and their NGINX counterparts, see
[default.conf](conf/default.conf). Notable outliers are the `cgi_ext` and
`cgi_dir` directives, as NGINX doesn't support CGI, or the `upload_dir` directives,
since NGINX also doesn't support simple uploading of files out of the box (sensibly
so, the semantics of the `upload_dir` directives are explained here in the wiki).
