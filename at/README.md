# At is On
When trying to use the old `at` on Windows cmd command shell, I realized that it was deprecated.
It was recommended to use the windows scheduler instead.
For some really simple tasks, this is way too much overhead, and i still want to use something as simle as the old at command
### Solution
So I wrote this simple Program to do exactly that.
As the old `at`is still there and sometimes not so easy to overwrite, since it is part of the windows system, I chose another Name for this. And as `at` is quite short and easy to remember, I chose `on` for this.
`On` for this command sounds quite like "wrong" english, but it does it's job, and so does it's name.
### Usage	
Usage: on Time Command

time format: `hh:mm` or `hh:mm:ss`

Command can be any command you want to execute. In some cases, it may help to include it in double quotes (`"`).
### Examples
* on 12:30 "echo Hello World"
* on 14:20 "ls -atl"