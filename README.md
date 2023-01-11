# Proxy Lab

## Introduction

ICS Proxy Lab, Peking University.

In this lab, you will write a simple HTTP and HTTPS proxy that caches web objects. For the first part of the lab, you will set up the proxy to accept incoming connections, read and parse requests, forward requests to web servers, read the servers’ responses, and forward those responses to the corresponding clients. This first part will involve learning about basic HTTP operation and how to use sockets to write programs that communicate over network connections. In the second part, you will upgrade your proxy to deal with multiple concurrent connections. This will introduce you to dealing with concurrency, a crucial systems concept. In the third part, you will add caching to your proxy using a simple main memory cache of recently accessed web content. In the last part, you will enable your proxy to handle HTTPS requests.

For more information about this lab, please refer to ```proxylab.pdf```.

## Installation

It is recommended to do this lab on Ubuntu 22.04. Before you run autograder, some packages need to be installed. Execute following commands in your Linux machine to install them.
```
sudo apt install net-tools
sudo apt install curl
pip install selenium
wget https://chromedriver.storage.googleapis.com/107.0.5304.62/chromedriver_linux64.zip
unzip chromedriver_linux64.zip
chmod +x chromedriver
sudo mv chromedriver /usr/local/bin/chromedriver
wget https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb
sudo dpkg -i google-chrome-stable_current_amd64.deb
sudo apt update
sudo apt --fix-broken install
```

## Score

This assignment will be graded out of a total of 90 points, which will be awarded based on the following criteria:
- *BasicCorrectness*: 40 points for basic proxy operation
- *Concurrency*: 15 points for handling concurrent requests
- *Cache*: 15 points for a working cache
- *RealPages*: 20 points for passing browser-based tests: total 10 tests, 2pt each, including 1 test for Part IV

My score for this lab is as follows. Note that I made some changes to ```csapp.c``` and ```Makefile``` as well.
| Score | Basic | Concurrency | Caching | Real pages |
| ----- | ----- | ----------- | ------- | ---------- |
| 90    | 40    | 15          | 15      | 20         |

**Note**: Since [https://pkuhelper.pku.edu.cn](https://pkuhelper.pku.edu.cn) was down on January 7th, 2023 and replaced with [https://treehole.pku.edu.cn](https://treehole.pku.edu.cn), if you run my scripts now, you will only get 88 score. However, if your implementation is correct, you should be able to see the following error message:
```
[11:33:50.359] --- Running [real-2048]
[11:33:52.374 info] we got an 8 after 9 moves
[11:33:52.375 debug] We will check if the proxy is still working after 1s
[11:33:53.418 error] TypeError: Failed to fetch
[11:33:53.419] TEST FINISHED. status = failed
```
