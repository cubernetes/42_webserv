# Configuration Requirements as per [Subject](webserv-subject.pdf), explained line by line

## 1. choose port and host for each server

    - server.listen
    - default: *:8000
    - multiple: yes
    - https://nginx.org/en/docs/http/ngx_http_core_module.html#listen

## 2. setup server_names or not

    - server.server_name
    - default: ""
    - multiple: yes
    - https://nginx.org/en/docs/http/ngx_http_core_module.html#server_name

## 3. the first server for a host:port will be the default for this host:port (that means it will answer to all the requests that don't belong to an other server)

    - https://nginx.org/en/docs/http/request_processing.html

## 4. setup default error pages

    - {http,server,location}.error_page
    - default: none
    - multiple: yes
    - https://nginx.org/en/docs/http/ngx_http_core_module.html#error_page

## 5. limit client body size

    - {http,server,location}.client_max_body_size
    - default: 1m
    - multiple: no
    - https://nginx.org/en/docs/http/ngx_http_core_module.html#client_max_body_size

## 6. setup routes with one or multiple of the following rules/configuration (routes won't be using regexp)

    - location == route in NGINX-speak
    - server.location
    - default: none
    - multiple: yes
    - https://nginx.org/en/docs/http/ngx_http_core_module.html#location

### 6.1 define a list of accepted http methods for the route

    - location.limit_except
    - default: none
    - multiple: no
    - https://nginx.org/en/docs/http/ngx_http_core_module.html#limit_except

### 6.2 define a http redirection

    - location.return
    - default: none
    - multiple: no // although NGINX allows for multiple
    - https://nginx.org/en/docs/http/ngx_http_rewrite_module.html#return

### 6.3 define a directory or a file from where the file should be searched

    - location.alias
    - default: none
    - multiple: no
    - https://nginx.org/en/docs/http/ngx_http_core_module.html#alias

### 6.4 turn on or off directory listing

    - {http,server,location}.autoindex
    - default: off
    - multiple: no
    - https://nginx.org/en/docs/http/ngx_http_autoindex_module.html#autoindex

### 6.5 set a default file to answer if the request is a directory

    - {http,server,location}.index
    - default: index.html
    - multiple: yes
    - https://nginx.org/en/docs/http/ngx_http_index_module.html#index

### 6.6 execute CGI based on certain file extension (for example .php)

    - no NGINX equivalent
    - {http,server,location}.cgi_ext -> register file extension (1st arg) and optional handler (2nd arg)
    - default: none
    - multiple: yes
    - {http,server,location}.cgi_dir -> whitelist a directory for CGI-use, absolute or relative to root directive
    - default: cgi-bin
    - multiple: no

### 6.7 make it work with POST andd GET methods

    - probably referring to CGI, since a simple static web server doesn't need POST

### 6.8 make the route able to accept uploaded files and configure where they should be saved

    - no NGINX equivalent
    - {http,server,location}.upload_dir -> absolute or relative to root directive
    - default: ""
    - multiple: no
    - always passed as UPLOAD_FILE_PATH to CGI

#### 6.8.1 do you wonder what a CGI is

    - yes

#### 6.8.2 because you won't call the CGI directly, use the full path as PATH_INFO

    - env["PATH_INFO"] = WHO KNOWS IDK, ubuntu_cgi_tester might give a hint
    - means: execve("/bin/python3", {"/bin/python3", "/full/path/cgi-bin/test.py", NULL}, env);
    - but also:
    - means: execve("/full/path/cgi-bin/test.cgi", {"/full/path/cgi-bin/test.cgi", NULL}, env;

#### 6.8.3 just remember that, for chunked request, your server needs to unchunk it, the CGI will expect EOF as end of the body

    - refers the `transfer-encoding: chunked` requests

#### 6.8.4 same things for the output of the CGI. If no content_length is returned from the CGI, EOF will mark the end of the returned data

    - need to collect all stdout from CGI script, then send as response with content-length

#### 6.8.5 your program should call the CGI with the file requested as the first argument

    - see 6.8.2 and 6.6, it depends on how the cgi extension was registered
    - the CGI script should definitely not receive additional arguments, that's not how CGI works

#### 6.8.6 the cgi should be run in the correct directory for relative path file access

    - chdir should be applied before execve

#### 6.8.7 your server should work with one CGI

    - they mean "at least" one, not exactly one
    - we'll use python because easier

## 7. define a root for various file operations (not mentioned in subject)

    - {http,server,location}.root
    - default: html
    - multiple: no
    - https://nginx.org/en/docs/http/ngx_http_core_module.html#root
